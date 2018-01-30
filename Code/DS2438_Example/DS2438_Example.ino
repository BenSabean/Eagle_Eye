/*
     DS2438TemperatureAndVoltage

     This example demonstrates the use of the DS2438 Library to read temperature and
     voltage from a Dallas Semiconductor DS2438 battery monitor using the Arduino
     OneWire library.

     by Joe Bechter

     (C) 2012, bechter.com

     All files, software, schematics and designs are provided as-is with no warranty.
     All files, software, schematics and designs are for experimental/hobby use.
     Under no circumstances should any part be used for critical systems where safety,
     life or property depends upon it. You are responsible for all use.
     You are free to use, modify, derive or otherwise extend for your own non-commercial purposes provided
         1. No part of this software or design may be used to cause injury or death to humans or animals.
         2. Use is non-commercial.
         3. Credit is given to the author (i.e. portions © bechter.com), and provide a link to the original source.

*/

#include <Arduino.h>
#include <OneWire.h>
#include <DS2438.h>
/* Chip 1 Calibration */
#define S1_COEFFICIENT_A    3.5
#define S1_OFFSET_A       -0.04
#define S1_COEFFICIENT_B  0.018
#define S1_OFFSET_B       -0.04
/* Chip 2 Calibration */
#define S2_COEFFICIENT_A    3.5
#define S2_OFFSET_A       -0.04
#define S2_COEFFICIENT_B  0.018
#define S2_OFFSET_B       -0.04
/* Chip 3 Calibration */
#define S3_COEFFICIENT_A    3.5
#define S3_OFFSET_A       -0.04
#define S3_COEFFICIENT_B  0.018
#define S3_OFFSET_B       -6.93
/* Chip 4 Calibration */
#define S4_COEFFICIENT_A    3.5
#define S4_OFFSET_A       -0.04
#define S4_COEFFICIENT_B  0.018
#define S4_OFFSET_B         0  //17.17

// define the Arduino digital I/O pin to be used for the 1-Wire network here
const uint8_t ONE_WIRE_PIN = 8;

// define the 1-Wire address of the DS2438 battery monitor here (lsb first)
uint8_t chip1_address[] = { 0x26, 0xED, 0x21, 0xCF, 0x01, 0x00, 0x00, 0x3F };
uint8_t chip2_address[] = { 0x26, 0x0F, 0x51, 0xCF, 0x01, 0x00, 0x00, 0x15 };
uint8_t chip3_address[] = { 0x26, 0x1B, 0x34, 0xCF, 0x01, 0x00, 0x00, 0x51 };
uint8_t chip4_address[] = { 0x26, 0xE4, 0x43, 0xCF, 0x01, 0x00, 0x00, 0x3B };

OneWire ow(ONE_WIRE_PIN);

DS2438 chip1(&ow, chip1_address);
DS2438 chip2(&ow, chip2_address);
DS2438 chip3(&ow, chip3_address);
DS2438 chip4(&ow, chip4_address);

void setup()
{
  Serial.begin(9600);
  chip1.begin(S1_COEFFICIENT_A, S1_OFFSET_A, S1_COEFFICIENT_B, S1_OFFSET_B);
  chip2.begin(S2_COEFFICIENT_A, S2_OFFSET_A, S2_COEFFICIENT_B, S2_OFFSET_B);
  chip3.begin(S3_COEFFICIENT_A, S3_OFFSET_A, S3_COEFFICIENT_B, S3_OFFSET_B);
  chip4.begin(S4_COEFFICIENT_A, S4_OFFSET_A, S4_COEFFICIENT_B, S4_OFFSET_B);
}

void loop()
{
  chip1.update();
  chip2.update();
  chip3.update();
  chip4.update();

  Serial.print("chip1\nTemp = ");
  Serial.print(chip1.getTemperature(), 1);
  //Serial.print("C, Data A = ");
  //Serial.print(chip1.getRawData(DS2438_CHA), 2);
  Serial.print(", Current A = ");
  Serial.print(chip1.getCurrent(DS2438_CHA), 2);
  //Serial.print(", Data B = ");
  //Serial.print(chip1.getRawData(DS2438_CHB), 2);
  Serial.print(", Current B = ");
  Serial.println(chip1.getCurrent(DS2438_CHB), 2);


  Serial.print("chip2\nTemp = ");
  Serial.print(chip2.getTemperature(), 1);
  //Serial.print("C, Data A = ");
  //Serial.print(chip2.getRawData(DS2438_CHA), 2);
  Serial.print(", Current A = ");
  Serial.print(chip2.getCurrent(DS2438_CHA), 2);
  //Serial.print(", Data B = ");
  //Serial.print(chip2.getRawData(DS2438_CHB), 2);
  Serial.print(", Current B = ");
  Serial.println(chip2.getCurrent(DS2438_CHB), 2);


  Serial.print("chip3\nTemp = ");
  Serial.print(chip3.getTemperature(), 1);
  //Serial.print("C, Data A = ");
  //Serial.print(chip3.getRawData(DS2438_CHA), 2);
  Serial.print(", Current A = ");
  Serial.print(chip3.getCurrent(DS2438_CHA), 2);
  //Serial.print(", Data B = ");
  //Serial.print(chip3.getRawData(DS2438_CHB), 2);
  Serial.print(", Current B = ");
  Serial.println(chip3.getCurrent(DS2438_CHB), 2);


  Serial.print("chip4\nTemp = ");
  Serial.print(chip4.getTemperature(), 1);
  //Serial.print("C, Data A = ");
  //Serial.print(chip4.getRawData(DS2438_CHA), 2);
  Serial.print(", Current A = ");
  Serial.print(chip4.getCurrent(DS2438_CHA), 2);
  //Serial.print(", Data B = ");
  //Serial.print(chip4.getRawData(DS2438_CHB), 2);
  Serial.print(", Current B = ");
  Serial.println(chip4.getCurrent(DS2438_CHB), 2);

  Serial.print("\nEND\n\n");
  delay(1000);
}
