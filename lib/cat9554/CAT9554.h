#ifndef __GUARD_CAT9554_H__
#define __GUARD_CAT9554_H__

#include <Arduino.h>
#include <Wire.h>

#define NORMAL                          0
#define INVERTED                        1

class CAT9554Class
{
protected:
 public:
  CAT9554Class();
  void    begin(void);
#if defined(ARDUINO_ARCH_ESP8266)
  void    begin(uint8_t sda, uint8_t scl);
#endif
  boolean pinMode(uint8_t pin, uint8_t mode);
  boolean pinPolarity(uint8_t pin, uint8_t polarity);
  boolean digitalWrite(uint8_t pin, boolean val);
  boolean digitalRead(uint8_t pin);

 private:
  uint8_t m_inp;
  uint8_t m_out;
  uint8_t m_pol;
  uint8_t m_ctrl;
};

extern CAT9554Class CAT9554;

#endif //ifndef __GUARD_CAT9554_H__

// EOF
