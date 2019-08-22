// -----------------------------------------------------------------------------
// CSE7766 based power monitor
// Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>
// http://www.chipsea.com/UploadFiles/2017/08/11144342F01B5662.pdf
// -----------------------------------------------------------------------------

#include "CSE7766.h"

// Constructor
  CSE7766::CSE7766() {
  }
// Destructor
  CSE7766::~CSE7766() {
      if (_serial) delete _serial;
      //end();
  }


  void CSE7766::setRX(unsigned char pin_rx) {
      if (_pin_rx == pin_rx) return;
      _pin_rx = pin_rx;
      _dirty = true;
  }

  void CSE7766::setInverted(bool inverted) {
      if (_inverted == inverted) return;
      _inverted = inverted;
      _dirty = true;
  }


  unsigned char CSE7766::getRX() {
      return _pin_rx;
  }

  bool CSE7766::getInverted() {
      return _inverted;
  }


  void CSE7766::expectedCurrent(double expected) {
      if ((expected > 0) && (_current > 0)) {
          _ratioC = _ratioC * (expected / _current);
      }
  }

  void CSE7766::expectedVoltage(unsigned int expected) {
      if ((expected > 0) && (_voltage > 0)) {
          _ratioV = _ratioV * (expected / _voltage);
      }
  }

  void CSE7766::expectedPower(unsigned int expected) {
      if ((expected > 0) && (_active > 0)) {
          _ratioP = _ratioP * (expected / _active);
      }
  }

  void CSE7766::setCurrentRatio(double value) {
      _ratioC = value;
  };

  void CSE7766::setVoltageRatio(double value) {
      _ratioV = value;
  };

  void CSE7766::setPowerRatio(double value) {
      _ratioP = value;
  };

  double CSE7766::getCurrentRatio() {
      return _ratioC;
  };

  double CSE7766::getVoltageRatio() {
      return _ratioV;
  };

  double CSE7766::getPowerRatio() {
      return _ratioP;
  };

  void CSE7766::resetRatios() {
      _ratioC = _ratioV = _ratioP = 1.0;
  }

  void CSE7766::resetEnergy(double value) {
      _energy = value;
  }

  double CSE7766::getCurrent() {
      return _current;
  }

  double CSE7766::getVoltage() {
      return _voltage;
  }

  double CSE7766::getActivePower() {
      return _active;
  }

  double CSE7766::getApparentPower() {
      return _voltage * _current;
  }

  double CSE7766::getReactivePower() {
      double active = getActivePower();
      double apparent = getApparentPower();
      if (apparent > active) {
          return sqrt(apparent * apparent - active * active);
      } else {
          return 0;
      }
  }

  double CSE7766::getPowerFactor() {
        return ((_voltage > 0) && (_current > 0)) ? 100 * _active / _voltage / _current : 100;
    }

  double CSE7766::getEnergy() {
      return _energy;
  }
  
  void CSE7766::begin() {

      if (!_dirty) return;

      if (_serial) delete _serial;

      if (1 == _pin_rx) {
          Serial.begin(CSE7766_BAUDRATE);
      } else {
          _serial = new SoftwareSerial(_pin_rx, SW_SERIAL_UNUSED_PIN, _inverted, 32);
          _serial->enableIntTx(false);
          _serial->begin(CSE7766_BAUDRATE);
      }

      _ready = true;
      _dirty = false;

  }

  void CSE7766::handle() {

      if (!_ready) return;
      _read();

  }


  
  // ---------------------------------------------------------------------
  // private
  // ---------------------------------------------------------------------

  /**
   * "
   * Checksum is the sum of all data
   * except for packet header and packet tail lowering by 8bit (...)
   * "
   * @return bool
   */
  bool CSE7766::_checksum() {
      unsigned char checksum = 0;
      for (unsigned char i = 2; i < 23; i++) {
          checksum += _data[i];
      }
      return checksum == _data[23];
  }

  void CSE7766::_process() {

      // Sample data:
      // 55 5A 02 E9 50 00 03 31 00 3E 9E 00 0D 30 4F 44 F8 00 12 65 F1 81 76 72 (w/ load)
      // F2 5A 02 E9 50 00 03 2B 00 3E 9E 02 D7 7C 4F 44 F8 CF A5 5D E1 B3 2A B4 (w/o load)

      #if SENSOR_DEBUG
          DEBUG_MSG("[SENSOR] CSE7766: _process: ");
          for (byte i=0; i<24; i++) DEBUG_MSG("%02X ", _data[i]);
          DEBUG_MSG("\n");
      #endif

      // Checksum
      if (!_checksum()) {
          _error = SENSOR_ERROR_CRC;
          #if SENSOR_DEBUG
              DEBUG_MSG("[SENSOR] CSE7766: Checksum error\n");
          #endif
          return;
      }

      // Calibration
      if (0xAA == _data[0]) {
          _error = SENSOR_ERROR_CALIBRATION;
          #if SENSOR_DEBUG
              DEBUG_MSG("[SENSOR] CSE7766: Chip not calibrated\n");
          #endif
          return;
      }

      if ((_data[0] & 0xFC) > 0xF0) {
          _error = SENSOR_ERROR_OTHER;
          #if SENSOR_DEBUG
              if (0xF1 == _data[0] & 0xF1) DEBUG_MSG("[SENSOR] CSE7766: Abnormal coefficient storage area\n");
              if (0xF2 == _data[0] & 0xF2) DEBUG_MSG("[SENSOR] CSE7766: Power cycle exceeded range\n");
              if (0xF4 == _data[0] & 0xF4) DEBUG_MSG("[SENSOR] CSE7766: Current cycle exceeded range\n");
              if (0xF8 == _data[0] & 0xF8) DEBUG_MSG("[SENSOR] CSE7766: Voltage cycle exceeded range\n");
          #endif
          return;
      }

      // Calibration coefficients
      unsigned long _coefV = (_data[2]  << 16 | _data[3]  << 8 | _data[4] );              // 190770
      unsigned long _coefC = (_data[8]  << 16 | _data[9]  << 8 | _data[10]);              // 16030
      unsigned long _coefP = (_data[14] << 16 | _data[15] << 8 | _data[16]);              // 5195000

      // Adj: this looks like a sampling report
      uint8_t adj = _data[20];                                                            // F1 11110001

      // Calculate voltage
      _voltage = 0;
      if ((adj & 0x40) == 0x40) {
          unsigned long voltage_cycle = _data[5] << 16 | _data[6] << 8 | _data[7];        // 817
          _voltage = _ratioV * _coefV / voltage_cycle / CSE7766_V2R;                      // 190700 / 817 = 233.41
      }

      // Calculate power
      _active = 0;
      if ((adj & 0x10) == 0x10) {
          if ((_data[0] & 0xF2) != 0xF2) {
              unsigned long power_cycle = _data[17] << 16 | _data[18] << 8 | _data[19];   // 4709
              _active = _ratioP * _coefP / power_cycle / CSE7766_V1R / CSE7766_V2R;       // 5195000 / 4709 = 1103.20
          }
      }

      // Calculate current
      _current = 0;
      if ((adj & 0x20) == 0x20) {
          if (_active > 0) {
              unsigned long current_cycle = _data[11] << 16 | _data[12] << 8 | _data[13]; // 3376
              _current = _ratioC * _coefC / current_cycle / CSE7766_V1R;                  // 16030 / 3376 = 4.75
          }
      }

      // Calculate energy
      unsigned int difference;
      static unsigned int cf_pulses_last = 0;
      unsigned int cf_pulses = _data[21] << 8 | _data[22];
      if (0 == cf_pulses_last) cf_pulses_last = cf_pulses;
      if (cf_pulses < cf_pulses_last) {
          difference = cf_pulses + (0xFFFF - cf_pulses_last) + 1;
      } else {
          difference = cf_pulses - cf_pulses_last;
      }
      _energy += difference * (float) _coefP / 1000000.0;
      cf_pulses_last = cf_pulses;

  }

  void CSE7766::_read() {

      _error = SENSOR_ERROR_OK;

      static unsigned char index = 0;
      static unsigned long last = millis();

      while (_serial_available()) {

          // A 24 bytes message takes ~55ms to go through at 4800 bps
          // Reset counter if more than 1000ms have passed since last byte.
          if (millis() - last > CSE7766_SYNC_INTERVAL) index = 0;
          last = millis();

          uint8_t byte = _serial_read();

          // first byte must be 0x55 or 0xF?
          if (0 == index) {
              if ((0x55 != byte) && (byte < 0xF0)) {
                  continue;
              }

          // second byte must be 0x5A
          } else if (1 == index) {
              if (0x5A != byte) {
                  index = 0;
                  continue;
              }
          }

          _data[index++] = byte;
          if (index > 23) {
              _serial_flush();
              break;
          }

      }

      // Process packet
      if (24 == index) {
          _process();
          index = 0;
      }

  }


  bool CSE7766::_serial_available() {
      if (1 == _pin_rx) {
          return Serial.available();
      } else {
          return _serial->available();
      }
  }

  void CSE7766::_serial_flush() {
      if (1 == _pin_rx) {
          return Serial.flush();
      } else {
          return _serial->flush();
      }
  }

  uint8_t CSE7766::_serial_read() {
      if (1 == _pin_rx) {
          return Serial.read();
      } else {
          return _serial->read();
      }
  }

