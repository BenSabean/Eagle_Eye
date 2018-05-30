/*
 *   DS2438.cpp
 *
 *   by Joe Bechter
 *
 *   (C) 2012, bechter.com
 *
 *   All files, software, schematics and designs are provided as-is with no warranty.
 *   All files, software, schematics and designs are for experimental/hobby use.
 *   Under no circumstances should any part be used for critical systems where safety,
 *   life or property depends upon it. You are responsible for all use.
 *   You are free to use, modify, derive or otherwise extend for your own non-commercial purposes provided
 *       1. No part of this software or design may be used to cause injury or death to humans or animals.
 *       2. Use is non-commercial.
 *       3. Credit is given to the author (i.e. portions © bechter.com), and provide a link to the original source.
 *
 */

#include "DS2438.h"

DS2438::DS2438(OneWire *ow, uint8_t *address) 
{
    _ow = ow;
    _address = address;
};

void DS2438::begin(float coefficientA, float offsetA, float coefficientB, float offsetB)
{
	uint8_t data[9];
	/* Initial condition for variables */
	_temperature = 0;
	_dataA = 0.0;
	_dataB = 0.0;
	_coefficientA = coefficientA;
	_offsetA = offsetA;
	_coefficientB = coefficientB;
	_offsetB = offsetB;

	/* Setting up status register for voltage, temp and current */
	if (!readPageZero(data))	// Reading the register
		return;
	data[0] = data[0] & 0xf7;	// Set bit4 (AD) to 0 - selecting VAD instead of VDD
	writePageZero(data);		// Writing data back 

}

void DS2438::update() 
{
    uint8_t data[9];

	/* Request Temp, Voltage and Current + delays */
	startConversion();
	delay(10);
	/* Read full 8 bytes of data from registers */
	if (!readPageZero(data))	// Reading the register
		return;
	/* Convert and Store Temperature */
    _temperature = (double)(((((int16_t)data[2]) << 8) | (data[1] & 0x0ff)) >> 3) * 0.03125;
	/* Convert and Store Voltage from Channel 1 */
    _dataA = (((data[4] << 8) & 0x00300) | (data[3] & 0x0ff)) / 100.0;
	/* Convert and Store Voltage from Channel 2 */
	_dataB = (((data[6] << 8) & 0x00300) | (data[5] & 0x0ff));
}

/*
	Set approriate bits in the Status Register [0x00]
	Send Start Conversion command
	Conversion Delay
*/
void DS2438::startConversion()
{
	uint8_t data[9];

    _ow->reset();
    _ow->select(_address);
	/* Star Temperature Conversion */
    _ow->write(DS2438_TEMPERATURE_CONVERSION_COMMAND, 0);//0x44
    delay(DS2438_TEMPERATURE_DELAY);
    _ow->reset();
    _ow->select(_address);
	/* Start Voltage Conversion */
    _ow->write(DS2438_VOLTAGE_CONVERSION_COMMAND, 0);// 0xB4
    delay(DS2438_VOLTAGE_CONVERSION_DELAY);
}


void DS2438::writePageZero(uint8_t *data) 
{
    _ow->reset();
    _ow->select(_address);
    _ow->write(DS2438_WRITE_SCRATCHPAD_COMMAND, 0);	// 0x4E
    _ow->write(DS2438_PAGE_0, 0);					// 0x00
    for (int i = 0; i < 8; i++)
        _ow->write(data[i], 0);
    _ow->reset();
    _ow->select(_address);
    _ow->write(DS2438_COPY_SCRATCHPAD_COMMAND, 0);	// 0x48
    _ow->write(DS2438_PAGE_0, 0);					// 0x00
}

boolean DS2438::readPageZero(uint8_t *data) 
{
    _ow->reset();						
    _ow->select(_address);		
    _ow->write(DS2438_RECALL_MEMORY_COMMAND, 0);	// 0xB8
    _ow->write(DS2438_PAGE_0, 0);					// 0x00
    _ow->reset();
    _ow->select(_address);						
    _ow->write(DS2438_READ_SCRATCHPAD_COMMAND, 0);	// 0xBE
    _ow->write(DS2438_PAGE_0, 0);					// 0x00
    for (int i = 0; i < 9; i++)
        data[i] = _ow->read();						

    return _ow->crc8(data, 8) == data[8];			// Last byte CRC check
}

double DS2438::getTemperature()
{
	return _temperature;
}

float DS2438::getRawData(bool channel)
{
	if (channel == DS2438_CHA)
		return _dataA;
	else if (channel == DS2438_CHB)
		return _dataB;
}

/* 
	Applying callibration and return current values
*/
float DS2438::getCurrent(bool channel)
{
	float currentA = 0;
	float currentB = 0;

	if (channel == DS2438_CHA)
	{
		currentA = (_dataA * _coefficientA) + _offsetA;
		if (currentA < 0 || currentA > 30)
			return 0;
		else 
			return currentA;
	}
	if (channel == DS2438_CHB)
	{
		currentB = (_dataB * _coefficientB) + _offsetB;
		if (currentB < 0 || currentB > 30)
			return 0;
		else
			return currentB;
	}
	return 0;
}



