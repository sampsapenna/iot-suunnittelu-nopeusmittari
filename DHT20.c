//
//    FILE: DHT20.cpp
//  AUTHOR: Rob Tillaart
// VERSION: 0.2.0
// PURPOSE: Arduino library for DHT20 I2C temperature and humidity sensor.
//
// HISTORY: see changelog.md





#include "DHT20.h"



//  set DHT20_WIRE_TIME_OUT to 0 to disable.
//  note this timeout is commented in code below.
#define DHT20_WIRE_TIME_OUT         250000    //  microseconds

const uint8_t DHT20_ADDRESS = 0x38;




//vaihda construktori pois
void DHT20_init(struct DHT20 *sens){

  //  reset() ?
  sens->_temperature = 0;
  sens->_humidity    = 0;
  sens->_humOffset   = 0;
  sens->_tempOffset  = 0;
  sens->_status      = 0;
  sens->_lastRequest = 0;
  sens->_lastRead    = 0;
}


//tämä tuntuu tarpeettomalta i2c hommassa. i2c init kutsutaan mainissa sitten kun se tarvitaan kutsua
bool  begin(struct DHT20 *sens)
{
    //200k magic num is gotten from original github page
  //_wire->begin();
  //  _wire->setWireTimeout(DHT20_WIRE_TIME_OUT, true);
  return i2c_init(GROVE_I2C_INST,200*1000)>0;
}




bool  isConnected(struct DHT20 *sens)
{
    return true;
    //tämä on pieni hutaisu en tiedä voiko konnektio pudota tuosta noin vaa 
    /*
  _wire->beginTransmission(DHT20_ADDRESS);
  int rv = _wire->endTransmission();
  return rv == 0;
    */
}


//  See datasheet 7.4 Sensor Reading Process, point 1
//  use with care.
uint8_t  resetSensor(struct DHT20 *sens)
{
  uint8_t count = 255;
  if ((readStatus() & 0x18) != 0x18)
  {
    count++;
    if (sens->_resetRegister(0x1B)) count++;
    if (sens->_resetRegister(0x1C)) count++;
    if (sens->_resetRegister(0x1E)) count++;
    sleep_ms(10);
  }
  return count;
}


////////////////////////////////////////////////
//
//  READ THE SENSOR
//
//lue1 bit?
int  read(struct DHT20 *sens)
{
  //  do not read to fast
  if (to_us_since_boot(get_absolute_time())/1000) - sens->_lastRead < 1000)
  {
    return DHT20_ERROR_LASTREAD;
  }

  int status = requestData(struct DHT20 *sens);
  if (status < 0) return status;
  //  wait for measurement ready
  while (isMeasuring(struct DHT20 *sens))
  {
    yield();
    //wtf is this
  }
  //  read the measurement
  status = readData(struct DHT20 *sens);
  if (status < 0) return status;

  //  convert it to meaningful data
  return convert(struct DHT20 *sens);
}


// this seems important, page 10 from datasheet
int  requestData(struct DHT20 *sens)
{
  //  reset sensor if needed.
  resetSensor(struct DHT20 *sens);
  
    //lievästi tarpeeton
    uint8_t mes1 = 0xAC;
    uint8_t mes2 = 0x33;
    uint8_t mes3 = 0x00;
  
    i2c_write_blocking(GROVE_I2C_INST,DHT20_ADDRESS,&mes1,1,false);
    i2c_write_blocking(GROVE_I2C_INST,DHT20_ADDRESS,&mes2,1,false);
    i2c_write_blocking(GROVE_I2C_INST,DHT20_ADDRESS,&mes3,1,false);

  //  GET CONNECTION
  /*
  _wire->beginTransmission(DHT20_ADDRESS);
  _wire->write(0xAC);
  _wire->write(0x33);
  _wire->write(0x00);
  int rv = _wire->endTransmission();
  */






/*
should return one of these
0: success.
1: data too long to fit in transmit buffer.
2: received NACK on transmit of address.
3: received NACK on transmit of data.
4: other error.
5: timeout
*/
    //en tiedä miten tehdä errorit paremmin
    int rv=0;




  _lastRequest = to_us_since_boot(get_absolute_time())/1000;
  return rv;
}





int  readData(struct DHT20 *sens)
{
  //  GET DATA
  const uint8_t length = 7;
  //int bytes = _wire->requestFrom(DHT20_ADDRESS, length);
  uint8_t bytes;

    int32_t ret=i2c_read_blocking(GROVE_I2C_INST,DHT20_ADDRESS,*bytes,1,false);
    if (ret=0){
        bytes=0;
    }else{
      i2c_read_blocking(GROVE_I2C_INST,DHT20_ADDRESS,*sens->_bits,length,false);
      //laita &sens._bits 
    }

    /*
//  if (bytes == 0)     return DHT20_ERROR_CONNECT;
 // if (bytes < length) return DHT20_MISSING_BYTES;

  //bool allZero = true;
    

    //wire.h lukee yksi kerrallaan mutta meidän ei tarvitse
  for (int i = 0; i < bytes; i++)
  {
    _bits[i] = _wire->read();
    //  if (_bits[i] < 0x10) Serial.print(0);
    //  Serial.print(_bits[i], HEX);
    //  Serial.print(" ");
    allZero = allZero && (_bits[i] == 0);
  }

    Serial.println();
  if (allZero) return DHT20_ERROR_BYTES_ALL_ZERO;

    */
  sens->_lastRead = to_us_since_boot(get_absolute_time())/1000;
  return bytes;
}


int  convert(struct DHT20 *sens)
{
  //  CONVERT AND STORE
  sens->_status      = sens->_bits[0];
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
  uint8_t sens->_crc = _crc8(sens->_bits, 6);
  //  Serial.print(_crc, HEX);
  //  Serial.print("\t");
  //  Serial.println(_bits[6], HEX);
  if (sens->_crc != sens->_bits[6]) return DHT20_ERROR_CHECKSUM;

  return DHT20_OK;
}


////////////////////////////////////////////////
//
//  TEMPERATURE & HUMIDITY & OFFSET
//
float  getHumidity(struct DHT20 *sens)
{
  return sens->_humidity + sens->_humOffset;
};


float  getTemperature(struct DHT20 *sens)
{
  return sens->_temperature + sens->_tempOffset;
};


void  setHumOffset(struct DHT20 *sens,float offset)
{
  sens->_humOffset  = offset;
};


void  setTempOffset(struct DHT20 *sens,float offset)
{
  sens->_tempOffset = offset;
};


float  getHumOffset(struct DHT20 *sens)
{
  return sens->_humOffset;
};


float  getTempOffset(struct DHT20 *sens)
{
  return sens->_tempOffset;
};


////////////////////////////////////////////////
//
//  STATUS
//
uint8_t  readStatus(struct DHT20 *sens)
{
    /*
  _wire->beginTransmission(DHT20_ADDRESS);
  _wire->write(0x71);
  _wire->endTransmission();
    */
   uint8_t bytes;
   uint8_t mes =0x71;
    i2c_write_blocking(GROVE_I2C_INST,DHT20_ADDRESS,&mes,1,false);

  sleep_ms(1);  //  needed to stabilize timing
  //_wire->requestFrom(DHT20_ADDRESS, (uint8_t)1);
    i2c_read_blocking(GROVE_I2C_INST,DHT20_ADDRESS,*bytes,1,false);
  //en tiedä millä vaihtaa
  sleep_ms(1);  //  needed to stabilize timing



  //return (uint8_t) _wire->read();
  return (uint8_t) i2c_read_blocking(GROVE_I2C_INST,DHT20_ADDRESS,*bytes,1,false);
  //returns bytes gotten should work


}


bool  isCalibrated(struct DHT20 *sens)
{
  return (readStatus() & 0x08) == 0x08;
}


bool  isMeasuring(struct DHT20 *sens)
{
  return (readStatus() & 0x80) == 0x80;
}


bool  isIdle(struct DHT20 *sens)
{
  return (readStatus() & 0x80) == 0x00;
}


int  internalStatus(struct DHT20 *sens)
{
  return sens->_status;
};


////////////////////////////////////////////////
//
//  TIMING
//
uint32_t  lastRead(struct DHT20 *sens)
{
  return sens->_lastRead;
};


uint32_t  lastRequest(struct DHT20 *sens)
{
  return sens->_lastRequest;
};


////////////////////////////////////////////////
//
//  PRIVATE
//
uint8_t  _crc8(uint8_t *ptr, uint8_t len)
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
//    18 seems to be status register
//    other values unknown.
bool  _resetRegister(uint8_t reg)
{
  uint8_t value[3];
  //vv en halunnut antaa suoraa &0x00 parametriksi
  uint8_t nill =0x00;

    /*
  _wire->beginTransmission(DHT20_ADDRESS);
  _wire->write(reg);
  _wire->write(0x00);
  _wire->write(0x00);
  if (_wire->endTransmission() != 0) return false;
    */
    i2c_write_blocking(GROVE_I2C_INST,DHT20_ADDRESS,&reg,1,false);
    i2c_write_blocking(GROVE_I2C_INST,DHT20_ADDRESS,&nill,1,false);
    i2c_write_blocking(GROVE_I2C_INST,DHT20_ADDRESS,&nill,1,false);

  sleep_ms(5);
    uint8_t dummy[4];

  //int bytes = _wire->requestFrom(DHT20_ADDRESS, (uint8_t)3); 
    int bytes = i2c_read_blocking(GROVE_I2C_INST,DHT20_ADDRESS,&dummy,3,false);



  /*
  tietääkseni ei tarvitse
  for (int i = 0; i < bytes; i++)
  {
    value[i] = _wire->read();
    //  Serial.println(value[i], HEX);
  }
  */

        i2c_read_blocking(GROVE_I2C_INST,DHT20_ADDRESS,&value,bytes,false);


        sleep_ms(10);

uint8_t val =0xB0 | reg;
/*
  _wire->beginTransmission(DHT20_ADDRESS);
  _wire->write(0xB0 | reg);
  _wire->write(value[1]);
  _wire->write(value[2]);
  if (_wire->endTransmission() != 0) return false;
*/

    i2c_write_blocking(GROVE_I2C_INST,DHT20_ADDRESS,&val,1,false);
    i2c_write_blocking(GROVE_I2C_INST,DHT20_ADDRESS,&value[1],1,false);
    if (i2c_write_blocking(GROVE_I2C_INST,DHT20_ADDRESS,&value[2],1,false)==PICO_ERROR_GENERIC){
        return false;
    }
    //scuffed versio error handlesta mikä oli 



  sleep_ms(5);
  return true;
}


// -- END OF FILE --
