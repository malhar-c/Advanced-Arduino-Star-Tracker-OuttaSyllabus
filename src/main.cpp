#include <arduino.h>
#include <Keypad_I2C.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Wire.h>

//stepper control with timer interrupts implementation
//all settings are done in library (pinouts, microstepping threasholds, etc)
#include <avr/io.h>
#include "A4988-stepperDriver.h"
#include <util/delay.h>

#define max_steps 12500 //25000

//siderial tracking calculations [all in mm and minutes]
#define SIDEREAL_DAY_MINUTES      1436.06818333  //1436.06818333 minutes
#define THREAD_PITCH              1   //1mm pitch
#define SWIVEL_MOUNT_DISTANCE     21  //21mm
#define BASE_RADIUS_AT_0_DEG      256   //radius when the planks are at 0 degree = 256mm

float theta = 0;
float psi = 0;
float correction = 0;

//I2C devices
#define I2CADDR 0x26 //keypad
#define I2CLCDADDR 0x27 //lcd

//Thermistor/Diode & Fan PINs
#define A4988_Thermistor A7
#define KA7805_Diode_Sensor  A2 // used to bias the diode  anode
#define fanPin 3

//Stopper switches
#define Lower_Limit 4
#define Upper_Limit 2

//TouchPad
#define Touch_pad 7
#define Touchpad_enable 0   //  0=false 1=true

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

char hexaKeys[ROWS][COLS] =
{
  {'0','*','7','4'},
  {'4','0','8','5'},
  {'6','#','9','6'},
  {'C','D','C','B'}
};

byte rowPins[ROWS] = {4, 5, 6, 7};
byte colPins[COLS] = {3, 2, 1, 0};

//initialize an instance of class NewKeypad
Keypad_I2C customKeypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2CADDR);

//iniliatize lcd
LiquidCrystal_I2C lcd(I2CLCDADDR,16,2);   //16 02 LCD

//Temperaure calculation variables
int Vo;
float R1 = 10000;
float logR2, R2, T, Tc;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
//diode temp cal
const int t0 = 20.3;        // calibration temp
const float vf0 = 573.44;   // measured forward voltage
float dtemp;
float dtemp_avg;
float t;
float greater_temp = 0;  //stores the higher temp value among the components

//other Variables
bool low_lim_flag = 0;
bool up_lim_flag = 0;
bool calib_flag = false; //once the calibation is done this should be true
bool track_flag = false;
bool manual_flag = false;
bool fan_man_flag = false;
long StepCount = 0;
bool Touch_1 = 0;
bool LCD_backlight_flag = true;
short fan_speed = 0;
unsigned long curr_temp_disp_millis = 0;
unsigned long prev_temp_disp_millis = 0;
short temp_disp_state = 0;
unsigned long curr_millis = 0;
unsigned long prev_millis = 0;
bool fan_auto_on = 0;
unsigned long arduino_runtime_millis = 0;
float solar_time = 0;
float sidereal_time = 0;
const float sidereal_conv = 1.0/0.99726958;


//siderial tracking variables
static long startPositionUSteps;
static long startPositionSecs;
static long startWallClockSecs;

static long targetWallClockSecs;
static long targetPositionSecs;
static long targetPositionUSteps;

//LCD functions
void starting_text();
void calibrating_text();
void ready_text();
void temp_display();
void Track_text();
void turn_on_backlight();


//temp control functions
void update_temp();

//Star tracker functions
void Calibrate();
void Track();
void Track_end();
void Rewind();
void update_everything();
void Touch_pad_refresh();
void update_limit_switches(); //only limit switches or update eveyrithing
void manual_mode();
void runfan(int);
void temp_Fan_speed_control();
void Full_cycle();

short cal_sidereal(int);     //all the sidereal tracking calculations will be done here
float stepCount_to_seconds(long);

void setup()
{
  pinMode(Lower_Limit, INPUT_PULLUP);
  pinMode(Upper_Limit, INPUT_PULLUP);
  pinMode(fanPin, OUTPUT);
  pinMode(Touch_pad, INPUT);
  pinMode(KA7805_Diode_Sensor, INPUT_PULLUP); // set the  pin IN with npull up to bias the diode

  A4988_stepperSetup();

  Wire.begin();
  lcd.init();

  //show on the LCD
  starting_text();
  customKeypad.begin();
  Serial.begin(115200);

}

char customKey = customKeypad.getKey();

void loop()
{
  arduino_runtime_millis = millis();
  update_everything();
  Serial.println("inside loop");
  //ready_text();
  temp_display();
  temp_Fan_speed_control();
  runfan(fan_speed);

  if (Touch_1 && !track_flag)
  {
    goto track_init;
  }

  //bypass for debugging
  while(!calib_flag)
  {
    calibrating_text();
    Calibrate();
  }

  while(track_flag)
  {
    Track();
  }

  while(manual_flag)
  {
    manual_mode();
  }

  //turn_on_backlight();

  customKey = customKeypad.getKey();
  switch(customKey)
  {
    case 'B':
    track_init: //for the touch control to directly trigger track
    Serial.println("inside case B");
    //delay(1000);
    lcd.clear();
    lcd.home();
    lcd.print("B. Start Track");
    lcd.setCursor(0,1);
    lcd.print("*. Back");
    // delay(1000);
    Touch_pad_refresh();
    customKey = customKeypad.getKey();
    while(customKey != 'B' || customKey != '*')
    {
      Touch_pad_refresh();
      customKey = customKeypad.getKey();
      if (customKey == '*')
      {
        ready_text();
        break;
      }
      if (customKey == 'B' || Touch_1)
      {
        track_flag = true;
        break;
      }
    }
    break;

    case 'C':
    lcd.clear();
    lcd.home();
    lcd.print("C. Rewind & Calib");
    lcd.setCursor(0,1);
    lcd.print("*. Back");
    customKey = customKeypad.getKey();
    while(customKey != 'C' || customKey != '*')
    {
      customKey = customKeypad.getKey();
      if (customKey == '*')
      {
        ready_text();
        break;
      }

      if (customKey == 'C')
      {
        calib_flag = false;
        break;
      }
    }
    break;

    case 'D':
    lcd.clear();
    lcd.home();
    lcd.print("D. Manual Conrol");
    lcd.setCursor(0,1);
    lcd.print("*. Back");
    Touch_pad_refresh();
    customKey = customKeypad.getKey();
    while(customKey != 'D' || customKey != '*')
    {
      Touch_pad_refresh();
      customKey = customKeypad.getKey();
      if (customKey == '*')
      {
        ready_text();
        break;
      }

      if (customKey == 'D')
      {
        manual_flag = true;
        break;
      }
    }
    break;

    //fan on off
    case '5':
    lcd.clear();
    lcd.home();
    if(fan_speed != 100)
    {
      fan_man_flag = true;
      fan_speed = 100;
      lcd.print("Fan: Man");
    }
    else
    {
      fan_man_flag = false;
      fan_speed = 0;
      lcd.print("Fan: Auto");
    }
    lcd.setCursor(0,1);
    temp_display();
    break;

    //reduce fan speed
    case '4':
    lcd.clear();
    lcd.home();
    fan_speed-=20;
    fan_man_flag = true;
    if(fan_speed <= 0)
    {
      fan_man_flag = false;
      fan_speed = 0;
    }
    lcd.print("Fan: Man (");
    lcd.print(fan_speed);
    lcd.print("%)");
    lcd.setCursor(0,1);
    temp_display();
    break;

    //increase fan speed
    case '6':
    lcd.clear();
    lcd.home();
    fan_speed+=20;
    fan_man_flag = true;
    if(fan_speed >= 100)
    {
      fan_speed = 100;
    }
    lcd.print(" Fan: Man (");
    lcd.print(fan_speed);
    lcd.print("%)");
    lcd.setCursor(0,1);
    temp_display();
    break;

    //backlight on/off
    case '8':
    {
      if(LCD_backlight_flag){
        LCD_backlight_flag = false;
      }
      else{
        LCD_backlight_flag = true;
      }
      break;
    }

    //full cycle (plank up and down)
    case '#':
    lcd.clear();
    lcd.home();
    lcd.print("#. Continue");
    lcd.setCursor(0,1);
    lcd.print("*. Back");
    Touch_pad_refresh();
    customKey = customKeypad.getKey();
    while(customKey != '#' || customKey != '*')
    {
      Touch_pad_refresh();
      customKey = customKeypad.getKey();
      if (customKey == '*')
      {
        ready_text();
        break;
      }

      if (customKey == '#')
      {
        Full_cycle();
        break;
      }
    }
    break;
  }
}

void Full_cycle()
{
  lcd.clear();
  temp_display();
  temp_Fan_speed_control();
  runfan(fan_speed);
  update_everything();
  //Serial.println("outside do");

  A4988_stepperWakeUp();
  //run motor to open plank
  A4988_stepperDirection(TURN_RIGHT);
  do
  {
    // Serial.println("inside do");

    A4988_stepMicroseconds(1, 1000);
    A4988_isBusy();
    StepCount+=1;
    update_limit_switches();
  }while(up_lim_flag != 1 && StepCount < max_steps);
  A4988_isBusy();
  A4988_stepperSleep();
  lcd.setCursor(1, 0);
  lcd.print("Total: ");
  lcd.print(StepCount);
  _delay_ms(10000);
  calib_flag = false;
}

void turn_on_backlight()
{
  if(LCD_backlight_flag)
  {
    lcd.backlight();
  }
  else
  {
    lcd.noBacklight();
  }
}

void starting_text()
{
  lcd.clear();
  //lcd.noBacklight();
  lcd.setCursor(2,0);
  lcd.print("Star Tracker");
  lcd.setCursor(4,1);
  lcd.print("ADVANCED");
}

void calibrating_text()
{
  lcd.clear();
  //lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Calibrating...");
  lcd.blink_on();
}

void ready_text()
{
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("READY!");
}

void Track_text()
{
  lcd.setCursor(1, 0);
  lcd.print("Tracking ");
  lcd.print(StepCount);
}

void temp_display()
{
  update_temp();
  curr_temp_disp_millis = millis();

  if(curr_temp_disp_millis - prev_temp_disp_millis >= 3000) //3000 msec timeout
  {
    prev_temp_disp_millis = curr_temp_disp_millis;

    //state 0 - display A4998 temp
    //state 1 - display KA7805 temp
    if(temp_disp_state == 0)
    {
      temp_disp_state = 1;
      lcd.setCursor(0, 1);
      lcd.print(" KA7805 - ");
      lcd.print(int(t));
      lcd.print(" ");
      lcd.print((char)223);
      lcd.print("C");
    }
    else
    {
      temp_disp_state = 0;
      lcd.setCursor(0, 1);
      lcd.print("  A4988 - ");
      lcd.print(int(Tc));
      lcd.print((char)223);
      lcd.print("C ");
    }
  }
}


void update_temp()
{
  //A4988 temp calculation
  Vo = analogRead(A4988_Thermistor);
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  Tc = T - 273.15;

  //KA7805 temp calculation
  dtemp_avg = 0;
  for (int i = 0; i < 10; i++)
  {
    float vf = analogRead(KA7805_Diode_Sensor) * (4976.30 / 1023.000);
    dtemp = (vf - vf0) * 0.4545454;
    dtemp_avg = dtemp_avg + dtemp;
  }
  t = t0 - dtemp_avg / 10;
  if (t > (int)42)
  {
    LCD_backlight_flag = false;
  }

}

void temp_Fan_speed_control()
{
  //only if manual fan control is off
  if (!fan_man_flag)
  {
    //calculation of the greater temp among A4988 and KA7805
    if(Tc > t){
      greater_temp = Tc;
    }
    else if(t > Tc){
      greater_temp = t;
    }
    else{
      greater_temp = Tc;
    }

    //Setting fan speed for higher temp component
    if(greater_temp >= 37.00)
    {
      fan_auto_on = 1;
      //fan_speed = greater_temp;
      float speedf;
      speedf = (greater_temp-37.00) * (100.00-50.00) / (45.00-37.00) + 50.00;
      fan_speed = (int)speedf;
      // fan_speed = map(greater_temp, 37.00, 45.00, 50, 100);
    }
    else if (greater_temp < 33.00)
    {
      fan_speed = 0;
      fan_auto_on = 0;
    }

    if (fan_auto_on && greater_temp < 37.00)
    {
      float speedf;
      speedf = (greater_temp-33.00) * (100.00-30.00) / (45.00-33.00) + 30.00;
      fan_speed = (int)speedf;
    }
  }
}

void Calibrate()
{
  calib_flag = true;
  A4988_stepperWakeUp();
  //run motor to close plank
  A4988_stepperDirection(TURN_LEFT);
  // _delay_ms(500);
  do
  {
    A4988_stepMicroseconds(1, 1000);
    update_limit_switches();
  } while(low_lim_flag != 1);

  A4988_isBusy();
  _delay_ms(100);

  //run the motor to open the plank
  A4988_stepperDirection(TURN_RIGHT);
  do
  {
    A4988_stepMicroseconds(1, 1000);
    update_limit_switches();
  } while(low_lim_flag == 1);
  A4988_isBusy();
  _delay_ms(100);
  A4988_stepperSleep();
  // // [NO NEED] run motor to open plank a bit more for buffer
  // Set_RPM(150);
  // stepper.setMicrostep(1);
  // stepper.rotate(180);
  StepCount = 0;
  // stepper.disable();
  // delay(200);
  ready_text();
  temp_display();
}

void update_limit_switches()
{
  if(digitalRead(Lower_Limit) == 0){
    low_lim_flag = 1;
  }

  else{
    low_lim_flag = 0;
  }

  if(digitalRead(Upper_Limit) == 0){
    up_lim_flag = 1;
  }

  else{
    up_lim_flag = 0;
  }
}

void Touch_pad_refresh()
{
  if(Touchpad_enable)
  {
    Touch_1 = digitalRead(Touch_pad);
    // Serial.print("TouchPad debug status: ");
    // Serial.println(digitalRead(Touch_pad));
  }

  else
  {
    Touch_1 = 0;
  }
}

void update_everything()
{
  lcd.noBlink();
  turn_on_backlight();
  update_limit_switches();
  update_temp();
  Touch_pad_refresh();
}

void manual_mode()
{
  manual_flag = false;
  lcd.clear();
  lcd.noBlink();
  lcd.print("7. Open plank");
  lcd.setCursor(0,1);
  lcd.print("9. Close Plank    ");
  bool Open_toggle_state = false;
  bool Close_toggle_state = false;
  do
  {
    update_limit_switches();
    customKey = customKeypad.getKey();
    switch (customKey)
    {
      case '7':
      A4988_stepperWakeUp();
      if(Open_toggle_state == false){
        Open_toggle_state = true;
      }else{
        Open_toggle_state = false;
      }
      break;

      case '9':
      A4988_stepperWakeUp();
      //run motor to close plank
      if(Close_toggle_state == false){
        Close_toggle_state = true;
      }else{
        Close_toggle_state = false;
      }
      break;

      case '*':
      //delay(500);
      lcd.clear();
      lcd.print("Manual M Paused");
      lcd.setCursor(0, 1);
      lcd.print("#. Resm");
      lcd.print(" *. Cancl");
      // delay(500);
      do
      {
        customKey = customKeypad.getKey();
        if(customKey == '#')
        {
          lcd.clear();
          break;
        }
        else if (customKey == '*')
        {
          ready_text();
          temp_display();
          lcd.blink();
          A4988_stepperSleep();
          return;
        }
      } while(1);
    }
    if(Open_toggle_state)
    {
      //run motor to open plank
      A4988_stepperDirection(TURN_RIGHT);
      A4988_stepMicroseconds(1, 1000);
      A4988_isBusy();
      StepCount+=1;
      update_limit_switches();
      if(up_lim_flag || StepCount >= max_steps)
      {
        Open_toggle_state = false;
        lcd.clear();
        lcd.home();
        lcd.print("Upper Limit       ");
        //do something to tell the limit is reached
        _delay_ms(1000);
        update_limit_switches();
        lcd.clear();
        lcd.print("7. Open plank");
        lcd.setCursor(0,1);
        lcd.print("9. Close Plank    ");
      }
    }

    if(Close_toggle_state)
    {
      //run motor to close plank
      A4988_stepperDirection(TURN_LEFT);
      A4988_stepMicroseconds(1, 1000);
      A4988_isBusy();
      StepCount-=1;
      update_limit_switches();
      if(low_lim_flag)
      {
        Close_toggle_state = false;
        lcd.clear();
        lcd.home();
        lcd.print("Lower Limit       ");
        //do something to tell the limit is reached
        _delay_ms(1000);
        update_limit_switches();
        lcd.clear();
        lcd.print("7. Open plank");
        lcd.setCursor(0,1);
        lcd.print("9. Close Plank    ");
      }
    }
  } while(1);
}

void Track()
{
  float track_runtime = stepCount_to_seconds(StepCount);
  // Serial.print("TRACK RUNTIME: ");
  // Serial.println(track_runtime);
  track_flag = false;
  lcd.clear();
  lcd.print("Tracking Init");
  lcd.noBlink();
  LCD_backlight_flag = true;
  delay(1000);
  for(int i=0; i<4; i++)
  {
    delay(250);
    lcd.print(".");
  }
  //delay(500);
  LCD_backlight_flag = false;
  temp_display();
  lcd.clear();
  Serial.println("track triggered");
  //tracking do
  do
  {
    solar_time = (float(millis() - arduino_runtime_millis)/1000) + track_runtime;
    sidereal_time = solar_time * sidereal_conv;
    update_everything();
    Track_text();
    temp_display();
    temp_Fan_speed_control();
    runfan(fan_speed);
    //update_limit_switches();
    //Touch_pad_refresh();
    customKey = customKeypad.getKey();
    //Serial.print(Touch_1);
    if (customKey == '*')// || Touch_1)
    {
      LCD_backlight_flag = true;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("Tracking Paused");
      lcd.setCursor(0, 1);
      lcd.print("#. Resm");
      lcd.print(" *. Cancl");
      // delay(500);
      Touch_pad_refresh();
      customKey = customKeypad.getKey();
      do
      {
        Touch_pad_refresh();
        update_everything();
        temp_Fan_speed_control();
        runfan(fan_speed);
        customKey = customKeypad.getKey();
        if(customKey == '#' || Touch_1)
        {
          lcd.clear();
          LCD_backlight_flag = false;
          break;
        }
        else if (customKey == '*')
        {
          A4988_stepperSleep();
          ready_text();
          temp_display();
          lcd.blink();
          return;
        }
      } while(1);
    }
    else if(customKey == '8')
    {
      if (LCD_backlight_flag)
      {
        LCD_backlight_flag = false;
      }
      else{
        LCD_backlight_flag = true;
      }
    }

    short delay_bet_steps_ms = cal_sidereal(sidereal_time);
    A4988_stepperWakeUp();
    //run motor to open plank
    A4988_stepperDirection(TURN_RIGHT);
    A4988_stepMilliseconds(1, delay_bet_steps_ms);
    // Serial.print(" DELAY BETWEEN Steps: ");
    // Serial.println(delay_bet_steps_ms);
    A4988_isBusy();
    StepCount+=1;

  }while(up_lim_flag != 1 && StepCount <= max_steps);
  Track_end();
}

void Track_end()
{
  bool lcd_blink_state = 0;
  A4988_stepperSleep();
  lcd.clear();
  lcd.print("Upper Limit");
  lcd.setCursor(0,1);
  lcd.print("C. Rewind & Calibrate");
  do
  {
    curr_millis = millis();
    if(curr_millis - prev_millis >= 1000)
    {
      prev_millis = curr_millis;
      if(!lcd_blink_state)
      {
        lcd.backlight();
        lcd_blink_state = 1;
      }
      else
      {
        lcd.noBacklight();
        lcd_blink_state = 0;
      }
    }

    // lcd.backlight();
    // lcd_blink_state = 1;
    // Stepper_beep();
    // delay(100);
    // lcd.noBacklight();
    // lcd_blink_state = 0;
    // delay(900);
    Touch_pad_refresh();
    customKey = customKeypad.getKey();
    Serial.print(Touch_1);
    //Serial.print(customKey);
    if(customKey == 'C' || Touch_1)
    {
      Serial.println("inside calibrate init");
      Serial.print(Touch_1);
      delay(500);
      calib_flag = false;
      Serial.print(Touch_1);
      break;
    }
  } while(customKey != '*' || !Touch_1);
  ready_text();
  temp_display();
}

//map(value, fromLow, fromHigh, toLow, toHigh)
void runfan(int speed)
{
  analogWrite(fanPin, map(speed, 0, 100, 100, 255));
  // Serial.print("Fan Speed Debug: ");
  // Serial.println(speed);
  if(speed == 0)
  {
    analogWrite(fanPin, 0);
  }
}

//Function for Siderial tracking calculations
short cal_sidereal(int track_sidereal_runtime)
{
  short delay_steps_ms = 0;
  float Corrected_radius = 0;
  float Speed_in_RPM = 0;
  // Serial.print("Siderial RUNTIME: ");
  // Serial.print(track_sidereal_runtime);
  theta = (((float)track_sidereal_runtime)/3600) * (PI/12); //for max 1 hour
  // Serial.print(" Theta: ");
  // Serial.print(theta, 4);
  psi = (PI - theta)/2;
  // Serial.print(" PSI: ");
  // Serial.print(psi, 4);
  correction = SWIVEL_MOUNT_DISTANCE * tan((PI/2)-psi);
  // Serial.print(" Correction: ");
  // Serial.print(correction, 2);
  Corrected_radius = BASE_RADIUS_AT_0_DEG - correction;  //tangent error corrected bradius
  // Serial.print(" Corrected R: ");
  // Serial.print(Corrected_radius);
  Speed_in_RPM = Corrected_radius * ((2*PI)/SIDEREAL_DAY_MINUTES) * THREAD_PITCH;
  delay_steps_ms = Speed_in_RPM*300;
  // Serial.print(" RPM : ");
  // Serial.print(Speed_in_RPM);
  return (delay_steps_ms);
}

float stepCount_to_seconds(long steps)
{
  int rotations = steps/200;
  float angle_rad = atan((float)rotations/BASE_RADIUS_AT_0_DEG);
  float angle_deg = angle_rad * (180/PI);
  float seconds = 240 * (angle_deg);
  // Serial.println(angle_rad, 6);
  // Serial.println(angle_deg);
  // Serial.println(seconds);
  // _delay_ms(5000);
  return (seconds);
}
