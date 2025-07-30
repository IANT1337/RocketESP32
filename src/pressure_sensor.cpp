#include "pressure_sensor.h"
#include <math.h>

PressureSensor::PressureSensor() : initialized(false), seaLevelPressure(1013.25) {
}

PressureSensor::~PressureSensor() {
}

void PressureSensor::initialize() {
  Serial.println("Initializing pressure sensor...");
  
  // Initialize I2C
  Wire.begin(PRESSURE_SDA_PIN, PRESSURE_SCL_PIN);
  Wire.setClock(I2C_FREQUENCY);
  
  // Wait for sensor to stabilize
  delay(100);
  
  const int maxRetries = 3;
  bool success = false;
  
  for (int attempt = 1; attempt <= maxRetries && !success; attempt++) {
    Serial.print("Pressure sensor initialization attempt ");
    Serial.print(attempt);
    Serial.print(" of ");
    Serial.println(maxRetries);
    
    // Test communication
    Wire.beginTransmission(MPRLS_I2C_ADDR);
    if (Wire.endTransmission() == 0) {
      success = true;
      initialized = true;
      Serial.println("Pressure sensor initialized successfully");
    } else {
      Serial.println("Failed to communicate with pressure sensor");
      if (attempt < maxRetries) {
        Serial.println("Retrying in 500ms...");
        delay(500);
      }
    }
  }
  
  if (!success) {
    Serial.println("Failed to initialize pressure sensor after all retries");
    initialized = false;
  }
}

bool PressureSensor::readData(float& pressure, float& altitude) {
  if (!initialized) {
    return false;
  }
  
  uint32_t rawPressure, rawTemperature;
  
  if (!readRawData(rawPressure, rawTemperature)) {
    return false;
  }
  
  // Convert raw pressure to hPa
  // MPRLS sensor range: 0-25 PSI (0-1724.1 hPa)
  // 24-bit resolution: 0 to 16777215
  pressure = ((float)rawPressure / 16777215.0) * 1724.1;
  
  // Calculate altitude
  altitude = calculateAltitude(pressure);
  
  return true;
}

bool PressureSensor::isValid() {
  return initialized;
}

bool PressureSensor::readRawData(uint32_t& pressure, uint32_t& temperature) {
  // Start conversion
  Wire.beginTransmission(MPRLS_I2C_ADDR);
  Wire.write(0xAA);  // Start conversion command
  Wire.write(0x00);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  
  // Wait for conversion to complete
  delay(10);
  
  // Check status and read data
  uint8_t status;
  uint8_t data[7];
  
  Wire.requestFrom(MPRLS_I2C_ADDR, 7);
  if (Wire.available() != 7) {
    return false;
  }
  
  for (int i = 0; i < 7; i++) {
    data[i] = Wire.read();
  }
  
  status = data[0];
  
  // Check for errors
  if (status & MPRLS_STATUS_FAILED) {
    Serial.println("Pressure sensor: Failed status");
    return false;
  }
  
  if (status & MPRLS_STATUS_SATURATED) {
    Serial.println("Pressure sensor: Saturated status");
    return false;
  }
  
  if (status & MPRLS_STATUS_BUSY) {
    Serial.println("Pressure sensor: Still busy");
    return false;
  }
  
  // Extract pressure and temperature data
  pressure = ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | data[3];
  temperature = ((uint32_t)data[4] << 16) | ((uint32_t)data[5] << 8) | data[6];
  
  return true;
}

float PressureSensor::calculateAltitude(float pressure) {
  // Barometric formula for altitude calculation
  // h = 44330 * (1 - (P/P0)^(1/5.255))
  // where P0 is sea level pressure and P is measured pressure
  
  if (pressure <= 0) {
    return 0.0;
  }
  
  float altitude = 44330.0 * (1.0 - pow(pressure / seaLevelPressure, 0.1903));
  return altitude;
}
