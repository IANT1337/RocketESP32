#ifndef MPU9250_SENSOR_H
#define MPU9250_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// MPU9250 I2C address
#define MPU9250_I2C_ADDR 0x68

// MPU9250 register addresses
#define MPU9250_WHO_AM_I 0x75
#define MPU9250_PWR_MGMT_1 0x6B
#define MPU9250_PWR_MGMT_2 0x6C
#define MPU9250_CONFIG 0x1A
#define MPU9250_GYRO_CONFIG 0x1B
#define MPU9250_ACCEL_CONFIG 0x1C
#define MPU9250_ACCEL_CONFIG2 0x1D
#define MPU9250_INT_PIN_CFG 0x37
#define MPU9250_USER_CTRL 0x6A

// Data registers
#define MPU9250_ACCEL_XOUT_H 0x3B
#define MPU9250_GYRO_XOUT_H 0x43
#define MPU9250_TEMP_OUT_H 0x41

// Magnetometer (AK8963) registers
#define AK8963_I2C_ADDR 0x0C
#define AK8963_WHO_AM_I 0x00
#define AK8963_CNTL1 0x0A
#define AK8963_XOUT_L 0x03

// Expected WHO_AM_I values
#define MPU9250_WHO_AM_I_VALUE 0x71
#define AK8963_WHO_AM_I_VALUE 0x48

// Scale factors
#define ACCEL_SCALE_2G 16384.0f
#define GYRO_SCALE_250DPS 131.0f
#define MAG_SCALE 0.6f  // µT per LSB for ±4800µT range

struct IMUData {
  // Accelerometer data (g)
  float accel_x;
  float accel_y;
  float accel_z;
  
  // Gyroscope data (degrees/second)
  float gyro_x;
  float gyro_y;
  float gyro_z;
  
  // Magnetometer data (µT)
  float mag_x;
  float mag_y;
  float mag_z;
  
  // Temperature (°C)
  float temperature;
  
  bool valid;
};

class MPU9250Sensor {
private:
  bool initialized;
  bool magnetometerInitialized;
  
  bool writeRegister(uint8_t address, uint8_t reg, uint8_t value);
  bool readRegister(uint8_t address, uint8_t reg, uint8_t& value);
  bool readRegisters(uint8_t address, uint8_t reg, uint8_t* buffer, uint8_t count);
  
  bool initializeMPU9250();
  bool initializeMagnetometer();
  
  bool readAccelerometer(float& x, float& y, float& z);
  bool readGyroscope(float& x, float& y, float& z);
  bool readMagnetometer(float& x, float& y, float& z);
  bool readTemperature(float& temp);

public:
  MPU9250Sensor();
  ~MPU9250Sensor();
  
  void initialize();
  bool readData(IMUData& data);
  bool isValid();
};

#endif
