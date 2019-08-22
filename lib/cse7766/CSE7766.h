// -----------------------------------------------------------------------------
// CSE7766 based power monitor
// Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>
// http://www.chipsea.com/UploadFiles/2017/08/11144342F01B5662.pdf
// -----------------------------------------------------------------------------

#ifndef CSE7766_h
#define CSE7766_h

#include "Arduino.h"
#include "debug.h"
#include <SoftwareSerial.h>

#ifndef CSE7766_PIN
#define CSE7766_PIN                     1       // TX pin from the CSE7766
#endif

#ifndef CSE7766_PIN_INVERSE
#define CSE7766_PIN_INVERSE             0       // Signal is inverted
#endif

#define CSE7766_SYNC_INTERVAL           300     // Safe time between transmissions (ms)
#define CSE7766_BAUDRATE                4800    // UART baudrate

#define CSE7766_V1R                     1.0     // 1mR current resistor
#define CSE7766_V2R                     1.0     // 1M voltage resistor

#define SENSOR_ERROR_OK             0       // No error
#define SENSOR_ERROR_OUT_OF_RANGE   1       // Result out of sensor range
#define SENSOR_ERROR_WARM_UP        2       // Sensor is warming-up
#define SENSOR_ERROR_TIMEOUT        3       // Response from sensor timed out
#define SENSOR_ERROR_UNKNOWN_ID     4       // Sensor did not report a known ID
#define SENSOR_ERROR_CRC            5       // Sensor data corrupted
#define SENSOR_ERROR_I2C            6       // Wrong or locked I2C address
#define SENSOR_ERROR_GPIO_USED      7       // The GPIO is already in use
#define SENSOR_ERROR_CALIBRATION    8       // Calibration error or Not calibrated
#define SENSOR_ERROR_OTHER          99      // Any other error

class CSE7766 {

public:

// ---------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------

  CSE7766();
  virtual ~CSE7766();
  void setRX(unsigned char pin_rx);
  void setInverted(bool inverted);
  unsigned char getRX();
  bool getInverted();
  void expectedCurrent(double expected);
  void expectedVoltage(unsigned int expected);
  void expectedPower(unsigned int expected);
  void setCurrentRatio(double value);
  void setVoltageRatio(double value);
  void setPowerRatio(double value);
  double getCurrentRatio();
  double getVoltageRatio();
  double getPowerRatio();
  void resetRatios();
  void resetEnergy(double value = 0);
  double getCurrent(); //_current
  double getVoltage(); //_voltage
  double getActivePower(); //_active
  double getApparentPower(); // _voltage * _current;
  double getReactivePower();
  double getPowerFactor(); //((_voltage > 0) && (_current > 0)) ? 100 * _active / _voltage / _current : 100;
  double getEnergy(); //_energy

  void begin();
  void handle();

private:

// ---------------------------------------------------------------------
// private
// ---------------------------------------------------------------------

  int _error = 0;
  bool _dirty = true;
  bool _ready = false;
  unsigned int _pin_rx = CSE7766_PIN;
  bool _inverted = CSE7766_PIN_INVERSE;
  SoftwareSerial * _serial = NULL;

  double _active = 0;
  double _voltage = 0;
  double _current = 0;
  double _energy = 0;

  double _ratioV = 1.0;
  double _ratioC = 1.0;
  double _ratioP = 1.0;

  unsigned char _data[24];
  
bool _checksum();
void _process();
void _read();
bool _serial_available();
void _serial_flush();
uint8_t _serial_read();

};
#endif
