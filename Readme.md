
# DIY Advanced Star Tracker with Arduino and NEMA 17 (A4988)

> ***Author:** Malhar Chakraborty (OuttaSyllabus)*

> ***Date:** 18/03/2022*

> ***Started Advanced version:** Oct 2021 (See the Release notes)*

> ***Started Barn door tracker Arduino project:** *Dec 2020* [GitHub Link](https://github.com/malhar-c/Star-Tracker-ver.-11.x.x)*

________
> **Features:**
>  - 0 Manual mechanical operation.
>  - Auto calibration after startup and end of track using limit switches this results in higher total track time by using the whole
> length of the threaded rod.
>  - Auto correct tangent error overtime (using variable speed stepper drive) calculation is done using length from the hinge to threaded rod
> and the pitch of the threaded rod used.
>  - Low noise operation (using 16th micro-stepping)
>  - Non-blocking stepper driving using timer interrupts (library)
>  - Lots of customization options for future.


## Electronics:

 1. Arduino Nano
 2. A4988 Stepper Driver
 3. NEMA 17 Stepper (1.8 step angle) Motor
 4. 4x4 Matrix Membrane keypad with I2c module (LCD backpack)
 5. 1602 LCD with I2c backpack
 6. Analog Temperature sensors (thermistors)
 7. Fan (12V)
 8. MOSFET (IRFP150 or any n-channel mosfet will work) for Fan control
 9. KA7805 (5v Voltage regulator for Arduino voltage supply)
 10. Power source: 18650 Li ion cells (3s) with 3s BMS
 11. Capacitive Touch sensor for shake free operation of the tracker. `Note: this didn't work for me as everytime the modules stop working for some reason, but would be a very nice addition.`
 12. Limit switches (for upper and lower limit detection and calibration)


> **Note:** I've used a diode as temperature sensor as I ran out of thermistors and I was rushing to finish the initial prototype, it worked and I left it as it is, probably have to replace in future, as it's not as accurate and it depends on a ref voltage, which surprisingly in my case varies when I use the external power source, and when I use USB to power the Arduino only (it's not usable when used with USB as temp readings are incorrect).

## Mechanical construction and Hardware:

### Barndoor construction

> Watch the YouTube video for details about the construction and working :- [Coming soon @OuttaSyllabus](https://www.youtube.com/c/OuttaSyllabus)
> **NOTE:** Here I've listed the dimensions I've used in my design, you can vary everything as long as functionality is met.

 - Ply wood planks (about 11 inches long and) x2 `Note: I've used some lying around pieces of ply woods, any hardwood should work as long as you can work with it. xD`
 - Hinges x2 to make the barn door (2 inches)

### Tracking and mounting
> Link(s) I followed for construction:
> [Swivel mount design](https://www.nutsvolts.com/magazine/article/january2015_Wierenga)
>
> [f/138 – Daniel Berrangé](https://fstop138.berrange.com/2014/01/building-an-barn-door-mount-part-1-arduino-stepper-motor-control/)
>
> [Barn door tracker Calculator](https://blarg.co.uk/astronomy/barn-door-tracker-calculator)

1. PVC pipes cut to the breadth of the planks (1 inch dia) x2
2. PVC pipe holders (1 inch dia) x4

> **Note:** To this PVC pipe the motor and threaded rod mechanism will be attached such that it will swivel as the barn door opens with time.

4. Threaded rod (1mm pitch 5mm dia) `any dimensions will work just remember we have to couple this to the shaft of NEMA 17 and we need to know the pitch precisely because based on that only the tracking algorithm will work.`
5. Tee nut and washers.
6. NEMA 17 clamp `we have to attach the NEMA 17 to one of those PVC pipes so pick a Clamp which will work in this situation, I've just used a L bracket as nothing was available except of that. For more details watch the YT video @OuttaSyllabus`
7. NEMA 17 Shaft coupler.
8. Ball Head with suitable mounting hardware (to mount the ball had on top of the plank which will hold the camera).
9. Extra tripod quick release plate `You probably can skip this. I ordered one extra QR plate and permanently attached it to the barn door with two part resin epoxy adhesive (or whatever it's called, I've used Araldite) as my tracker was quite heavy and I though it would be more secure with this approach, you just need some sort of arrangement to mount it on top of a tripod without any wiggle room.`

### Electronics/Controls housing

> 3D Printed enclosure, printed in ABS+ (The A4988 heats up easily to around 50C without proper cooling, the ABS can survive up to 80-90 deg C direct contact with the heatsink, thus enough headroom.)

![](https://raw.githubusercontent.com/malhar-c/Advanced-Arduino-Star-Tracker-OuttaSyllabus/master/Images/enclosure_base.PNG)

![](https://raw.githubusercontent.com/malhar-c/Advanced-Arduino-Star-Tracker-OuttaSyllabus/master/Images/enclosure_lid.PNG)

## Exact components used (Links):

 1. Arduino: [Nano CH340 Chip Board without USB cable compatible with Arduino (Soldered)](https://robu.in/product/arduino-nano-board-r3-with-ch340-chip-wo-usb-cable-solderedarduino-nano-r3-wo-usb-cable-soldered/)
 2. A4988: [A4988 driver Stepper Motor Driver- Good Quality](https://robu.in/product/a4988-driver-stepper-motor-driver/)
 3. Stepper Motor shaft coupler: [Aluminum Flexible Shaft Coupling 5mm x 5mm](https://robu.in/product/aluminium-flexible-shaft-coupling-5mm-x-5mm/)
 4. 4x4 Membrane keypad: [# 4?4 Matrix Keypad Membrane Switch for Arduino, ARM and other MCU](https://robu.in/product/4x4-matrix-keypad-membrane-switch-arduino-arm-mcu/)
 5. 1602 LCD : [LCD1602 Parallel LCD Display Yellow Backlight](https://robu.in/product/serial-lcd1602-iic-i2c-yellow-backlight/)
 6. I2c backpack : [IIC/I2C Serial Interface Adapter Module](https://robu.in/product/iici2c-serial-interface-adapter-module/)
 7. Analog Temperature sensors (thermistors) : [Analog Temperature Sensor Module](https://robu.in/product/analog-temperature-module/)
 8. Fan (12V) : [12V 3010 Cooling Fan for 3D Printer](https://robu.in/product/12v-3010-cooling-fan-for-3d-printer/)
 9. KA7805 : [KA7805 Linear Voltage Regulator (Pack of 3 Ics)](https://robu.in/product/ka7805-linear-voltage-regulator-pack-of-3-ics/)
 10. 3s BMS: [3 Series 20A 18650 Lithium Battery Protection Board 11.1V 12V 12.6V](https://robu.in/product/3-series-20a-18650-lithium-battery-protection-board-11-1v-12v-12-6v/)
 11. Capacitive touch sensor: [Digital Sensor TTP223B Module Capacitive Touch Switch](https://robu.in/product/digital-sensor-ttp223b-module-capacitive-touch-switch/)
 12.

## Pin configurations

### Stepper Motor Driver (A4988)

> **Note:** These configurations will be directly done to the library file. Follow this link for more details about the library used: [Library for A4988 stepper motor driver using timer interrupt](https://www.programming-electronics-diy.xyz/2017/10/library-for-a4988-stepper-motor-driver.html)

> You have to define these pins using port register manipulation to increase read/write speeds. Read more about *Arduino port manipulation* [here](https://electronoobs.com/eng_arduino_tut130.php)

| A4988 Pin |  Arduino PIN|
|--|--|
| Step | Digital 9 |
| Dir  |  Digital 8  |
| Sleep | Digital 13 |
| MS1 | Digital 12 |
| MS2 | Digital 11 |
| MS3 | Digital 10 |

### Other accessories
| Accessory PIN |  Arduino PIN|
|--|--|
| A4988 Thermistor | Analog 7 |
| KA7805 Diode temp sensor  |  Analog 2  |
| Fan MOSFET (Gate) | Digital 3 |
| Lower Limit Switch (Pulled up) | Digital 4 |
| Upper Limit Switch (Pulled up) | Digital 2 |
| Touch switch | Digital 7 |

> **NOTE:** LCD and matrix keypad both are used using LCD i2c backpacks (just because they are cheap) but these backpacks have a minor problem which resulted in one of the rows of my keypad not working (just throwing random inputs when pressed) that didn't bother me though as I didn't need all 16 inputs, and this is easily fixable by going with a proper i2c module (which just happens to be 3-4 times more expensive in my area than the LCD backpack).

> Watch this for info: [Want to use a Keypad in your Arduino projects but running out of Pins?](https://youtu.be/n9Bq1kHYsJk)
