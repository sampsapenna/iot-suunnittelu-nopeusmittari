#ifndef DHT20_LIBRARY_DEFINED
#define DHT20_LIBRARY_DEFINED

//
//    FILE: DHT20.h
//  AUTHOR: Rob Tillaart
// PURPOSE: Arduino library for DHT20 I2C temperature and humidity sensor.
// VERSION: 0.2.0
// HISTORY: See DHT20.cpp
//     URL: https://github.com/RobTillaart/DHT20
//

//  Always check datasheet - front view
//
//          +--------------+
//  VDD ----| 1            |
//  SDA ----| 2    DHT20   |
//  GND ----| 3            |
//  SCL ----| 4            |
//          +--------------+


//#include "Arduino.h"
//#include "Wire.h"



#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

#define DHT20_OK 0
#define DHT20_ERROR_CHECKSUM -10
#define DHT20_ERROR_CONNECT -11
#define DHT20_MISSING_BYTES -12
#define DHT20_ERROR_BYTES_ALL_ZERO -13
#define DHT20_ERROR_READ_TIMEOUT -14
#define DHT20_ERROR_LASTREAD -15


#ifdef USE_I2C_1
#define I2C_INST i2c1
#else
#define I2C_INST i2c0
#endif


typedef struct DHT20 {
  float    _humidity;
  float    _temperature;
  float    _humOffset;
  float    _tempOffset;

  uint8_t  _status;
  uint32_t _lastRequest;
  uint32_t _lastRead;
  uint8_t  _bits[7];
  uint8_t  _crc;
} DHT20;

/*
Initialize the DHT sensor
*/
void DHT20_init(struct DHT20 *sens);

/*
Private function for calculating checksum value
*/
static uint8_t _crc8(uint8_t *ptr, uint8_t len);

/*
Reset the register
*/
bool _resetRegister(uint8_t reg);


// Request the data
int requestData(struct DHT20 *sens);
//  read the raw data.
int readData(struct DHT20 *sens);
//  converts raw databits to temperature and humidity.
int convert(struct DHT20 *sens);


//  blocking read call to read + convert data
int read(struct DHT20 *sens);
//  access the converted temperature & humidity
float getHumidity(struct DHT20 *sens);
float getTemperature(struct DHT20 *sens);


//  OFFSET  1st order adjustments
void setHumOffset(struct DHT20 *sens, float offset);
void setTempOffset(struct DHT20 *sens,float offset);
float getHumOffset(struct DHT20 *sens);
float getTempOffset(struct DHT20 *sens);


//  READ STATUS
uint8_t readStatus(struct DHT20 *sens);
//  3 wrapper functions around readStatus()
bool isCalibrated(struct DHT20 *sens);
bool isMeasuring(struct DHT20 *sens);
bool isIdle(struct DHT20 *sens);
//  status from last read()
int internalStatus(struct DHT20 *sens);


//  TIMING
uint32_t lastRead(struct DHT20 *sens);
uint32_t lastRequest(struct DHT20 *sens);


//  RESET  (new since 0.1.4)
//  use with care 
//  returns number of registers reset => must be 3
//  3     = OK
//  0,1,2 = error.
//  255   = no reset needed.
//  See datasheet 7.4 Sensor Reading Process, point 1
//  use with care 
uint8_t  resetSensor(struct DHT20 *sens);
#endif
