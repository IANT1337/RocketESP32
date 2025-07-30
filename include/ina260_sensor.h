#ifndef INA260_SENSOR_H
#define INA260_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// INA260 I2C address (can be 0x40-0x4F depending on A0/A1 pins)
#define INA260_I2C_ADDR 0x40

// INA260 register addresses
#define INA260_CONFIG 0x00
#define INA260_CURRENT 0x01
#define INA260_VOLTAGE 0x02
#define INA260_POWER 0x03
#define INA260_MASK_ENABLE 0x06
#define INA260_ALERT_LIMIT 0x07
#define INA260_MFG_UID 0xFE
#define INA260_DIE_UID 0xFF

// Expected manufacturer and die IDs
#define INA260_MFG_UID_VALUE 0x5449  // "TI" in ASCII
#define INA260_DIE_UID_VALUE 0x2270

// Configuration register bits
#define INA260_CONFIG_RESET 0x8000
#define INA260_CONFIG_AVG_MASK 0x0E00
#define INA260_CONFIG_VBUSCT_MASK 0x01C0
#define INA260_CONFIG_ISHCT_MASK 0x0038
#define INA260_CONFIG_MODE_MASK 0x0007

// Default configuration: continuous mode, 1.1ms conversion time, 1 sample average
#define INA260_CONFIG_DEFAULT 0x6127

// Conversion factors (fixed by INA260 hardware)
// For 11.1V LiPo monitoring (max voltage ~12.6V when fully charged)
#define INA260_CURRENT_LSB 1.25f    // mA per LSB (fixed)
#define INA260_VOLTAGE_LSB 1.25f    // mV per LSB (fixed, supports up to ~81V)
#define INA260_POWER_LSB 10.0f      // mW per LSB (fixed)

struct PowerData {
  float voltage;    // Bus voltage in volts
  float current;    // Current in milliamps
  float power;      // Power in milliwatts
  bool valid;       // Data validity flag
};

class INA260Sensor {
private:
  bool initialized;
  
  bool writeRegister(uint8_t reg, uint16_t value);
  bool readRegister(uint8_t reg, uint16_t& value);
  int16_t twosComplementToInt16(uint16_t value);

public:
  INA260Sensor();
  ~INA260Sensor();
  
  void initialize();
  bool readData(PowerData& data);
  bool readVoltage(float& voltage);
  bool readCurrent(float& current);
  bool readPower(float& power);
  bool isValid();
  void reset();
  bool setConfig(uint16_t config);
};

#endif
