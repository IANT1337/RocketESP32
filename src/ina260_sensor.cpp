#include "ina260_sensor.h"

INA260Sensor::INA260Sensor() : initialized(false) {
}

INA260Sensor::~INA260Sensor() {
}

void INA260Sensor::initialize() {
  Serial.println("Initializing INA260 power sensor...");
  

  
  const int maxRetries = 3;
  bool success = false;
  
  for (int attempt = 1; attempt <= maxRetries && !success; attempt++) {
    Serial.print("INA260 initialization attempt ");
    Serial.print(attempt);
    Serial.print(" of ");
    Serial.println(maxRetries);
    
    // Initialize I2C if not already done (shared with pressure and IMU sensors)
    Wire.begin(PRESSURE_SDA_PIN, PRESSURE_SCL_PIN);
    Wire.setClock(I2C_FREQUENCY);
  
    delay(100); // Allow sensor to stabilize

    // Check if INA260 is present by reading manufacturer ID
    uint16_t mfgId;
    if (!readRegister(INA260_MFG_UID, mfgId)) {
      Serial.println("Failed to read INA260 manufacturer ID");
      if (attempt < maxRetries) {
        Serial.println("Retrying in 500ms...");
        delay(500);
      }
      continue;
    }
    
    if (mfgId != INA260_MFG_UID_VALUE) {
      Serial.print("INA260 manufacturer ID mismatch. Expected: 0x");
      Serial.print(INA260_MFG_UID_VALUE, HEX);
      Serial.print(", Got: 0x");
      Serial.println(mfgId, HEX);
      if (attempt < maxRetries) {
        Serial.println("Retrying in 500ms...");
        delay(500);
      }
      continue;
    }
    
    // Check die ID as well
    uint16_t dieId;
    if (!readRegister(INA260_DIE_UID, dieId)) {
      Serial.println("Failed to read INA260 die ID");
      if (attempt < maxRetries) {
        Serial.println("Retrying in 500ms...");
        delay(500);
      }
      continue;
    }
    
    if (dieId != INA260_DIE_UID_VALUE) {
      Serial.print("INA260 die ID mismatch. Expected: 0x");
      Serial.print(INA260_DIE_UID_VALUE, HEX);
      Serial.print(", Got: 0x");
      Serial.println(dieId, HEX);
      if (attempt < maxRetries) {
        Serial.println("Retrying in 500ms...");
        delay(500);
      }
      continue;
    }
    
    // Reset the device
    if (!writeRegister(INA260_CONFIG, INA260_CONFIG_RESET)) {
      Serial.println("Failed to reset INA260");
      if (attempt < maxRetries) {
        Serial.println("Retrying in 500ms...");
        delay(500);
      }
      continue;
    }
    
    delay(50); // Wait for reset to complete
    
    // Configure the device
    if (!setConfig(INA260_CONFIG_DEFAULT)) {
      Serial.println("Failed to configure INA260");
      if (attempt < maxRetries) {
        Serial.println("Retrying in 500ms...");
        delay(500);
      }
      continue;
    }
    
    success = true;
    initialized = true;
    Serial.println("INA260 power sensor initialized successfully");
  }
  
  if (!success) {
    Serial.println("Failed to initialize INA260 after all retries");
  }
}

bool INA260Sensor::readData(PowerData& data) {
  if (!initialized) {
    data.valid = false;
    return false;
  }
  
  bool success = true;
  
  // Read voltage
  if (!readVoltage(data.voltage)) {
    success = false;
  }
  
  // Read current
  if (!readCurrent(data.current)) {
    success = false;
  }
  
  // Read power
  if (!readPower(data.power)) {
    success = false;
  }
  
  data.valid = success;
  return success;
}

bool INA260Sensor::readVoltage(float& voltage) {
  uint16_t rawVoltage;
  if (!readRegister(INA260_VOLTAGE, rawVoltage)) {
    return false;
  }
  
  // Convert to volts (LSB = 1.25mV)
  voltage = (rawVoltage * INA260_VOLTAGE_LSB) / 1000.0f;
  return true;
}

bool INA260Sensor::readCurrent(float& current) {
  uint16_t rawCurrent;
  if (!readRegister(INA260_CURRENT, rawCurrent)) {
    return false;
  }
  
  // Convert to milliamps (LSB = 1.25mA, signed)
  int16_t signedCurrent = twosComplementToInt16(rawCurrent);
  current = signedCurrent * INA260_CURRENT_LSB;
  return true;
}

bool INA260Sensor::readPower(float& power) {
  uint16_t rawPower;
  if (!readRegister(INA260_POWER, rawPower)) {
    return false;
  }
  
  // Convert to milliwatts (LSB = 10mW)
  power = rawPower * INA260_POWER_LSB;
  return true;
}

bool INA260Sensor::setConfig(uint16_t config) {
  return writeRegister(INA260_CONFIG, config);
}

void INA260Sensor::reset() {
  writeRegister(INA260_CONFIG, INA260_CONFIG_RESET);
  delay(50);
}

bool INA260Sensor::isValid() {
  return initialized;
}

bool INA260Sensor::writeRegister(uint8_t reg, uint16_t value) {
  Wire.beginTransmission(INA260_I2C_ADDR);
  Wire.write(reg);
  Wire.write((value >> 8) & 0xFF); // MSB first
  Wire.write(value & 0xFF);        // LSB second
  return Wire.endTransmission() == 0;
}

bool INA260Sensor::readRegister(uint8_t reg, uint16_t& value) {
  Wire.beginTransmission(INA260_I2C_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  
  Wire.requestFrom(INA260_I2C_ADDR, (uint8_t)2);
  if (Wire.available() >= 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    value = (msb << 8) | lsb;
    return true;
  }
  
  return false;
}

int16_t INA260Sensor::twosComplementToInt16(uint16_t value) {
  if (value & 0x8000) {
    // Negative number in two's complement
    return -((~value + 1) & 0xFFFF);
  } else {
    // Positive number
    return (int16_t)value;
  }
}
