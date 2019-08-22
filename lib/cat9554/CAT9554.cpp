#include <Arduino.h>
#include <Wire.h>
#include "CAT9554.h"

#define CAT9554_ADDRESS                 0x20

#define CAT9554_REG_INP                 0
#define CAT9554_REG_OUT                 1
#define CAT9554_REG_POL                 2
#define CAT9554_REG_CTRL                3

uint8_t pinNum2bitNum[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

/***************************************************************************
 *
 *  Writes 8-bits to the specified destination register
 *
 **************************************************************************/
static void writeRegister(uint8_t i2cAddress, uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(i2cAddress);
  Wire.write((uint8_t)reg);
  Wire.write((uint8_t)value);
  Wire.endTransmission();
}

/***************************************************************************
 *
 * Reads 8-bits from the specified source register
 *
 **************************************************************************/
static uint16_t readRegister(uint8_t i2cAddress, uint8_t reg)
{
  Wire.beginTransmission(i2cAddress);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(i2cAddress, (uint8_t)1);
  return Wire.read();
}

/***************************************************************************
 *
 * Constructor for the CAT9554Class class, not much here yet
 *
 **************************************************************************/
CAT9554Class::CAT9554Class(void) { }

/***************************************************************************
 *
 * Begin method. This method must be called before using this library
 * either directly if the class is initializing the Wire library or by
 * calling this library's function begin(sda, scl) in which case that
 * function will call this one.
 *
 **************************************************************************/
void CAT9554Class::begin(void)
{
  // Read out default values from the registers to the shadow variables.
  m_inp = readRegister(CAT9554_ADDRESS, CAT9554_REG_INP);
  m_out = readRegister(CAT9554_ADDRESS, CAT9554_REG_OUT);
  m_pol = readRegister(CAT9554_ADDRESS, CAT9554_REG_POL);
  m_ctrl = readRegister(CAT9554_ADDRESS, CAT9554_REG_CTRL);
}

#if defined(ARDUINO_ARCH_ESP8266)
/***************************************************************************
 *
 * Convenience method for ESP8266 systems such as the ESP210.
 *
 **************************************************************************/
void CAT9554Class::begin(uint8_t sda, uint8_t scl)
{
  Wire.begin(sda, scl);
  begin();
}
#endif

/***************************************************************************
 *
 * Sets the desired pin mode
 *
 **************************************************************************/
boolean CAT9554Class::pinMode(uint8_t pin, uint8_t mode)
{
  // Make sure the pin number is OK
  if (pin >= sizeof pinNum2bitNum) {
    return false;
  }

  // Calculate the new control register value
  if (mode == OUTPUT) {
    m_ctrl &= ~pinNum2bitNum[pin];
  } else if (mode == INPUT) {
    m_ctrl |= pinNum2bitNum[pin];
  } else {
    return false;
  }

  writeRegister(CAT9554_ADDRESS, CAT9554_REG_CTRL, m_ctrl);

  return true;
}


/***************************************************************************
 *
 * Sets the desired pin polarity. This can be used to invert inverse
 * hardware logic.
 *
 **************************************************************************/
boolean CAT9554Class::pinPolarity(uint8_t pin, uint8_t polarity)
{
  // Make sure pin number is OK
  if (pin >= sizeof pinNum2bitNum) {
    return false;
  }

  if (polarity == INVERTED) {
    m_pol |= pinNum2bitNum[pin];
  } else if (polarity == NORMAL) {
    m_pol &= ~pinNum2bitNum[pin];
  } else {
    return false;
  }

  writeRegister(CAT9554_ADDRESS, CAT9554_REG_POL, m_pol);

  return true;
}

/***************************************************************************
 *
 * Write digital value to pin
 *
 **************************************************************************/
boolean CAT9554Class::digitalWrite(uint8_t pin, boolean val)
{
  // Make sure pin number is OK
  if (pin >= sizeof pinNum2bitNum) {
    return false;
  }

  if (val == HIGH) {
    m_out |= pinNum2bitNum[pin];
  } else {
    m_out &= ~pinNum2bitNum[pin];
  }

  writeRegister(CAT9554_ADDRESS, CAT9554_REG_OUT, m_out);
}

/***************************************************************************
 *
 * Read digital value from pin.
 * Note, so far this function will fail silently if the pin parameter is
 * incorrectly specified.
 *
 **************************************************************************/
boolean CAT9554Class::digitalRead(uint8_t pin)
{
  return (readRegister(CAT9554_ADDRESS, CAT9554_REG_INP) & pinNum2bitNum[pin] != 0);
}

CAT9554Class CAT9554;

