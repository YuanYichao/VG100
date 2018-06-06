# Unfinished VG100 Project 1

## code
### openmv part (cm dictionary)
now this part of codes no longer needs template. It uses the density of the blob found in a picture to judge the direction of arrow

### arduino part (ardu dictionary)
the functions of tuning primarily is finished

the functions of moving are about to be finished

## doc text

### arduino mega
The MCU of the car, also serves as a power source. It controls the movement of the car, receives and handles infomation from sensors and control the behaviors of the screen and keypad.
#### How to connect
connect VIN on the arduino mega with the anode of the battery, and one of the GND to the cathode of the battery. Then connect 5V and 3.3V to two longer rows of pin headers on the circuit board and another GND to the shorter rows of pin headers.

### OpenMV
The sensor that detects the direction of the arrows during the game.
#### How to connect
##### signal wires
RX to TXN, and TX to RXN
##### power wires
VIN to 5V and GND to GND

### HC-SR04
The sensors that detect the distance.
#### How to connect
##### signal wires
For the sensor that is put at the head of the car: connect trig and echo to 24 and 25

For the sensor that is put at the rhs of the car: the one that located near the head should be connected to 26 and 27, for trig and echo respective. Similarly connect the other one's to 28, 29

Then connect the sensors on the lhs to 30-33 respectively.
##### power wires
VCC to 5V and GND to GND

### l298n
The part that drives the car. 'A' part is used to control the rhs and 'B' part for lhs
#### How to connect
##### signal wires
ENA and ENB connects to 2 and 3. And pin 1-4 connect to 36-39 on arduino

### Screen
The first function of the screen is outputing debug infomation, which is very useful when the car is not functioning well. With that infomation, you can refer to the "trouble shooting" part to repair or adjust your car. The second function of it is that it provides the user interface. Since our car is designed to be configured with some variables so that it can behave well in a different pathway, with a screen the whole process will be more user friendly.

#### How to connect
##### signal wires
connect lcd 2004's RS pin to 50, EN pin to 52 and pins from D4 to D7 to pins of 53, 51, 49 and 47 on arduino respectively. 
##### power wires
VSS, K to GND, VDD to 5V and A to 3.3V

### Keypad
The Keypad provide a way to input into the car MCU

#### HOW to connect
##### signal wires
connect the pins that represent rows to 38, 40, 42 and 44 respectively

connect the pins that represent cols to 39, 41, 43 and 45 respectively

### moter
These two moters keeps the car moving
#### How to connect 
connect the anode and cathode respectively to l298n