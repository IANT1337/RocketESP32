#include "mpu9250_sensor.h"

MPU9250Sensor::MPU9250Sensor() : initialized(false), magnetometerInitialized(false) {
}

MPU9250Sensor::~MPU9250Sensor() {
}

void MPU9250Sensor::initialize() {
  Serial.println("Initializing MPU9250 sensor...");
  
  // Initialize I2C if not already done
  Wire.begin(PRESSURE_SDA_PIN, PRESSURE_SCL_PIN);
  Wire.setClock(I2C_FREQUENCY);
  
  delay(100); // Allow sensor to stabilize
  
  const int maxRetries = 3;
  bool success = false;
  
  for (int attempt = 1; attempt <= maxRetries && !success; attempt++) {
    Serial.print("MPU9250 initialization attempt ");
    Serial.print(attempt);
    Serial.print(" of ");
    Serial.println(maxRetries);
    
    // Check if MPU9250 is present
    uint8_t whoAmI;
    if (!readRegister(MPU9250_I2C_ADDR, MPU9250_WHO_AM_I, whoAmI)) {
      Serial.println("Failed to read MPU9250 WHO_AM_I register");
      if (attempt < maxRetries) {
        Serial.println("Retrying in 500ms...");
        delay(500);
      }
      continue;
    }
    
    if (whoAmI != MPU9250_WHO_AM_I_VALUE) {
      Serial.print("MPU9250 WHO_AM_I mismatch. Expected: 0x");
      Serial.print(MPU9250_WHO_AM_I_VALUE, HEX);
      Serial.print(", Got: 0x");
      Serial.println(whoAmI, HEX);
      if (attempt < maxRetries) {
        Serial.println("Retrying in 500ms...");
        delay(500);
      }
      continue;
    }
    
    // Initialize MPU9250
    if (!initializeMPU9250()) {
      Serial.println("Failed to initialize MPU9250");
      if (attempt < maxRetries) {
        Serial.println("Retrying in 500ms...");
        delay(500);
      }
      continue;
    }
    
    // Initialize magnetometer (don't fail entirely if this fails)
    if (!initializeMagnetometer()) {
      Serial.println("Failed to initialize magnetometer (continuing without it)");
    }
    
    success = true;
    initialized = true;
    Serial.println("MPU9250 sensor initialized successfully");
  }
  
  if (!success) {
    Serial.println("Failed to initialize MPU9250 after all retries");
  }
}

bool MPU9250Sensor::initializeMPU9250() {
  // Reset device
  if (!writeRegister(MPU9250_I2C_ADDR, MPU9250_PWR_MGMT_1, 0x80)) {
    return false;
  }
  delay(100);
  
  // Wake up device and set clock source to gyro X
  if (!writeRegister(MPU9250_I2C_ADDR, MPU9250_PWR_MGMT_1, 0x01)) {
    return false;
  }
  delay(100);
  
  // Enable all sensors
  if (!writeRegister(MPU9250_I2C_ADDR, MPU9250_PWR_MGMT_2, 0x00)) {
    return false;
  }
  
  // Configure gyroscope (±250°/s)
  if (!writeRegister(MPU9250_I2C_ADDR, MPU9250_GYRO_CONFIG, 0x00)) {
    return false;
  }
  
  // Configure accelerometer (±2g)
  if (!writeRegister(MPU9250_I2C_ADDR, MPU9250_ACCEL_CONFIG, 0x00)) {
    return false;
  }
  
  // Set accelerometer sample rate (1kHz)
  if (!writeRegister(MPU9250_I2C_ADDR, MPU9250_ACCEL_CONFIG2, 0x00)) {
    return false;
  }
  
  // Configure DLPF (Digital Low Pass Filter)
  if (!writeRegister(MPU9250_I2C_ADDR, MPU9250_CONFIG, 0x03)) {
    return false;
  }
  
  // Enable I2C bypass mode for magnetometer access
  if (!writeRegister(MPU9250_I2C_ADDR, MPU9250_INT_PIN_CFG, 0x02)) {
    return false;
  }
  
  return true;
}

bool MPU9250Sensor::initializeMagnetometer() {
  // Check if magnetometer is present
  uint8_t whoAmI;
  if (!readRegister(AK8963_I2C_ADDR, AK8963_WHO_AM_I, whoAmI)) {
    return false;
  }
  
  if (whoAmI != AK8963_WHO_AM_I_VALUE) {
    Serial.print("AK8963 WHO_AM_I mismatch. Expected: 0x");
    Serial.print(AK8963_WHO_AM_I_VALUE, HEX);
    Serial.print(", Got: 0x");
    Serial.println(whoAmI, HEX);
    return false;
  }
  
  // Set magnetometer to continuous measurement mode (100Hz)
  if (!writeRegister(AK8963_I2C_ADDR, AK8963_CNTL1, 0x16)) {
    return false;
  }
  
  delay(10);
  magnetometerInitialized = true;
  return true;
}

bool MPU9250Sensor::readData(IMUData& data) {
  if (!initialized) {
    data.valid = false;
    return false;
  }
  
  bool success = true;
  
  // Read accelerometer data
  if (!readAccelerometer(data.accel_x, data.accel_y, data.accel_z)) {
    success = false;
  }
  
  // Read gyroscope data
  if (!readGyroscope(data.gyro_x, data.gyro_y, data.gyro_z)) {
    success = false;
  }
  
  // Read temperature
  if (!readTemperature(data.temperature)) {
    success = false;
  }
  
  // Read magnetometer data (if available)
  if (magnetometerInitialized) {
    if (!readMagnetometer(data.mag_x, data.mag_y, data.mag_z)) {
      // Don't fail entirely if magnetometer fails
      data.mag_x = data.mag_y = data.mag_z = 0.0f;
    }
  } else {
    data.mag_x = data.mag_y = data.mag_z = 0.0f;
  }
  
  data.valid = success;
  return success;
}

bool MPU9250Sensor::readAccelerometer(float& x, float& y, float& z) {
  uint8_t buffer[6];
  if (!readRegisters(MPU9250_I2C_ADDR, MPU9250_ACCEL_XOUT_H, buffer, 6)) {
    return false;
  }
  
  int16_t raw_x = (buffer[0] << 8) | buffer[1];
  int16_t raw_y = (buffer[2] << 8) | buffer[3];
  int16_t raw_z = (buffer[4] << 8) | buffer[5];
  
  x = raw_x / ACCEL_SCALE_2G;
  y = raw_y / ACCEL_SCALE_2G;
  z = raw_z / ACCEL_SCALE_2G;
  
  return true;
}

bool MPU9250Sensor::readGyroscope(float& x, float& y, float& z) {
  uint8_t buffer[6];
  if (!readRegisters(MPU9250_I2C_ADDR, MPU9250_GYRO_XOUT_H, buffer, 6)) {
    return false;
  }
  
  int16_t raw_x = (buffer[0] << 8) | buffer[1];
  int16_t raw_y = (buffer[2] << 8) | buffer[3];
  int16_t raw_z = (buffer[4] << 8) | buffer[5];
  
  x = raw_x / GYRO_SCALE_250DPS;
  y = raw_y / GYRO_SCALE_250DPS;
  z = raw_z / GYRO_SCALE_250DPS;
  
  return true;
}

bool MPU9250Sensor::readMagnetometer(float& x, float& y, float& z) {
  if (!magnetometerInitialized) {
    return false;
  }
  
  uint8_t buffer[7]; // 6 data bytes + 1 status byte
  if (!readRegisters(AK8963_I2C_ADDR, AK8963_XOUT_L, buffer, 7)) {
    return false;
  }
  
  // Check if data is ready and not overflow
  uint8_t status = buffer[6];
  if (!(status & 0x01) || (status & 0x08)) {
    return false;
  }
  
  int16_t raw_x = (buffer[1] << 8) | buffer[0];
  int16_t raw_y = (buffer[3] << 8) | buffer[2];
  int16_t raw_z = (buffer[5] << 8) | buffer[4];
  
  x = raw_x * MAG_SCALE;
  y = raw_y * MAG_SCALE;
  z = raw_z * MAG_SCALE;
  
  return true;
}

bool MPU9250Sensor::readTemperature(float& temp) {
  uint8_t buffer[2];
  if (!readRegisters(MPU9250_I2C_ADDR, MPU9250_TEMP_OUT_H, buffer, 2)) {
    return false;
  }
  
  int16_t raw_temp = (buffer[0] << 8) | buffer[1];
  temp = (raw_temp / 333.87f) + 21.0f; // Convert to Celsius
  
  return true;
}

bool MPU9250Sensor::writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool MPU9250Sensor::readRegister(uint8_t address, uint8_t reg, uint8_t& value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  
  Wire.requestFrom(address, (uint8_t)1);
  if (Wire.available()) {
    value = Wire.read();
    return true;
  }
  
  return false;
}

bool MPU9250Sensor::readRegisters(uint8_t address, uint8_t reg, uint8_t* buffer, uint8_t count) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  
  Wire.requestFrom(address, count);
  if (Wire.available() >= count) {
    for (uint8_t i = 0; i < count; i++) {
      buffer[i] = Wire.read();
    }
    return true;
  }
  
  return false;
}

bool MPU9250Sensor::isValid() {
  return initialized;
}
