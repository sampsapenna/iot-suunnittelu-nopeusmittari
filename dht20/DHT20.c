// FILE: DHT20.c
// ORIGINAL AUTHOR: Rob Tillaart
// EXTENDED BY Kimi Malkamäki, Sampsa Penna 2022
// VERSION: 0.1.0
// PURPOSE: Arduino library for DHT20 I2C temperature and humidity sensor, ported for Raspberry Pi Pico SDK


#include "DHT20.h"


//  set DHT20_WIRE_TIME_OUT to 0 to disable.
//  note this timeout is commented in code below.
#define DHT20_WIRE_TIME_OUT         250000    //  microseconds


// src https://www.adafruit.com/product/5183
const uint8_t DHT20_ADDRESS = 0x38;


void DHT20_init(struct DHT20 *sens){
  sens->_temperature = 0;
  sens->_humidity    = 0;
  sens->_humOffset   = 0;
  sens->_tempOffset  = 0;
  sens->_status      = 0;
  sens->_lastRequest = 0;
  sens->_lastRead    = 0;
}


//  See datasheet 7.4 Sensor Reading Process, point 1
//  use with care.
uint8_t  resetSensor(struct DHT20 *sens)
{
  uint8_t count = 255;
  if ((readStatus(sens) & 0x18) != 0x18)
  {
    count++;
    if (_resetRegister(0x1B)) count++;
    if (_resetRegister(0x1C)) count++;
    if (_resetRegister(0x1E)) count++;
    sleep_ms(10);
  }
  return count;
}


////////////////////////////////////////////////
//
//  READ THE SENSOR
//
int  read(struct DHT20 *sens)
{
  int status;
  //  do not read to fast
  if (
    to_us_since_boot(get_absolute_time()) / 1000 - sens->_lastRead < 1000
  ) {
    return DHT20_ERROR_LASTREAD;
  }

  status = requestData(sens);
  if (status < 0) {
    return status;
  };
  sleep_ms(80);
  status = readData(sens);
  if (status < 0) {
    return status;
  };

  //  convert it to meaningful data
  return convert(sens);
}


// this seems important, page 10 from datasheet
int requestData(struct DHT20 *sens) {
  // reset sensor if needed.
  resetSensor(sens);
  
  uint8_t mes1 = 0xAC;
  uint8_t mes2 = 0x33;
  uint8_t mes3 = 0x00;
  
  i2c_write_blocking(I2C_INST,DHT20_ADDRESS,&mes1,1,false);
  i2c_write_blocking(I2C_INST,DHT20_ADDRESS,&mes2,1,false);
  i2c_write_blocking(I2C_INST,DHT20_ADDRESS,&mes3,1,false);
  sens->_lastRequest = to_us_since_boot(get_absolute_time()) / 1000;
  return 0;
}


int readData(struct DHT20 *sens)
{
  //  GET DATA
  const uint8_t length = 7;

  int bytes = i2c_read_blocking(
      I2C_INST,
      DHT20_ADDRESS,
      sens->_bits,
      length,
      false
  );

  bool allZero = true;
  for (int i = 0; i < length; i++) {
    if (sens->_bits[i] > 0) {
      allZero = false;
      break;
    }
  }
  if (allZero) {
    return DHT20_ERROR_BYTES_ALL_ZERO;
  }
  sens->_lastRead = to_us_since_boot(get_absolute_time()) / 1000;
  return bytes;
}


int  convert(struct DHT20 *sens)
{
  //  CONVERT AND STORE
  sens->_status = sens->_bits[0];
  uint32_t raw = sens->_bits[1];
  raw <<= 8;
  raw += sens-> _bits[2];
  raw <<= 4;
  raw += (sens->_bits[3] >> 4);
  sens->_humidity = raw * 9.5367431640625e-5;   // ==> / 1048576.0 * 100%;

  raw = (sens->_bits[3] & 0x0F);
  raw <<= 8;
  raw += sens->_bits[4];
  raw <<= 8;
  raw += sens->_bits[5];
  sens->_temperature = raw * 1.9073486328125e-4 - 50;  //  ==> / 1048576.0 * 200 - 50;

  //  TEST CHECKSUM
  sens->_crc = _crc8(sens->_bits, 6);
  if (sens->_crc != sens->_bits[6]) {
    return DHT20_ERROR_CHECKSUM;
  }

  return DHT20_OK;
}


////////////////////////////////////////////////
//
//  TEMPERATURE & HUMIDITY & OFFSET
//
float getHumidity(struct DHT20 *sens)
{
  return sens->_humidity + sens->_humOffset;
};


float getTemperature(struct DHT20 *sens)
{
  return sens->_temperature + sens->_tempOffset;
};


void setHumOffset(struct DHT20 *sens,float offset)
{
  sens->_humOffset  = offset;
};


void setTempOffset(struct DHT20 *sens,float offset)
{
  sens->_tempOffset = offset;
};


float getHumOffset(struct DHT20 *sens)
{
  return sens->_humOffset;
};


float getTempOffset(struct DHT20 *sens)
{
  return sens->_tempOffset;
};


////////////////////////////////////////////////
//
//  STATUS
//
uint8_t readStatus(struct DHT20 *sens)
{
   uint8_t bytes;
   uint8_t mes = 0x71;
  i2c_write_blocking(I2C_INST,DHT20_ADDRESS,&mes,1,false);

  sleep_ms(1);  //  needed to stabilize timing

  i2c_read_blocking(
    I2C_INST,
    DHT20_ADDRESS,
    &bytes,
    1,
    false
  );
  return bytes;
}


bool isCalibrated(struct DHT20 *sens)
{
  return (readStatus(sens) & 0x08) == 0x08;
}


bool isMeasuring(struct DHT20 *sens)
{
  return (readStatus(sens) & 0x80) == 0x80;
}


bool isIdle(struct DHT20 *sens)
{
  return (readStatus(sens) & 0x80) == 0x00;
}


int internalStatus(struct DHT20 *sens)
{
  return sens->_status;
};


////////////////////////////////////////////////
//
//  TIMING
//
uint32_t lastRead(struct DHT20 *sens)
{
  return sens->_lastRead;
};


uint32_t lastRequest(struct DHT20 *sens)
{
  return sens->_lastRequest;
};


////////////////////////////////////////////////
//
//  PRIVATE
//
static uint8_t _crc8(uint8_t *ptr, uint8_t len)
{
  uint8_t crc = 0xFF;
  while(len--)
  {
    crc ^= *ptr++;
    for (uint8_t i = 0; i < 8; i++)
    {
      if (crc & 0x80)
      {
        crc <<= 1;
        crc ^= 0x31;
      }
      else
      {
        crc <<= 1;
      }
    }
  }
  return crc;
}


//  Code based on demo code sent by www.aosong.com
//  no further documentation.
//  0x1B returned 18, 0, 4
//  0x1C returned 18, 65, 0
//  0x1E returned 18, 8, 0
//  18 seems to be status register
//  other values unknown.
bool _resetRegister(uint8_t reg)
{
  uint8_t value[3];
  uint8_t nill = 0x00;
  i2c_write_blocking(
    I2C_INST,
    DHT20_ADDRESS,
    &reg,
    1,
    false
  );
  i2c_write_blocking(
    I2C_INST,
    DHT20_ADDRESS,
    &nill,
    1,
    false
  );
  i2c_write_blocking(
    I2C_INST,
    DHT20_ADDRESS,
    &nill,
    1,
    false
  );

  sleep_ms(5);

  i2c_read_blocking(
    I2C_INST,
    DHT20_ADDRESS,
    value,
    3,
    false
  );
  sleep_ms(10);

  uint8_t val = 0xB0 | reg;
  i2c_write_blocking(
    I2C_INST,
    DHT20_ADDRESS,
    &val,
    1,
    false
  );
  i2c_write_blocking(
    I2C_INST,
    DHT20_ADDRESS,
    &value[1],
    1,
    false
  );
  i2c_write_blocking(
    I2C_INST,
    DHT20_ADDRESS,
    &value[2],
    1,
    false
  );
  sleep_ms(5);
  return true;
}
