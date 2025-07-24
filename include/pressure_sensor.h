#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

#define MPRLS_I2C_ADDR 0x18
#define MPRLS_STATUS_POWERED (0x40)
#define MPRLS_STATUS_BUSY (0x20)
#define MPRLS_STATUS_FAILED (0x04)
#define MPRLS_STATUS_SATURATED (0x01)

class PressureSensor {
private:
  bool initialized;
  float seaLevelPressure; // hPa, for altitude calculation
  
  bool readRawData(uint32_t& pressure, uint32_t& temperature);
  float calculateAltitude(float pressure);

public:
  PressureSensor();
  ~PressureSensor();
  
  void initialize();
  bool readData(float& pressure, float& altitude);
  void setSeaLevelPressure(float pressure) { seaLevelPressure = pressure; }
  bool isValid();
};

#endif
