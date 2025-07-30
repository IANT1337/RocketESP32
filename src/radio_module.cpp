#include "radio_module.h"

RadioModule::RadioModule() : initialized(false), highPowerMode(false), lastRSSIQuery(0), cachedRSSI(-999) {
  radioSerial = new HardwareSerial(2);
  void sendATCommand(String command, bool waitResponse);
}

RadioModule::~RadioModule() {
  delete radioSerial;
}

void RadioModule::initialize() {
  Serial.println("Initializing radio module...");
  
  // Initialize radio serial communication
  radioSerial->begin(RADIO_BAUD_RATE, SERIAL_8N1, RADIO_SERIAL_RX_PIN, RADIO_SERIAL_TX_PIN);
  
  // Brief delay for serial to stabilize
  delay(500);
  
  // Clear any existing data in the buffer
  while (radioSerial->available()) {
    radioSerial->read();
  }
  
  // Try to enter AT command mode with shorter timeout
  sendATCommand("+++", false); // Send without terminator
  delay(1100); // AT command mode requires > 1 second guard time
  
  // Test communication with shorter timeout
  sendATCommand("ATI");
  delay(5);
  String response = readATResponse(1000); // Reduced from 2000ms
  
  if (response.indexOf("RFD900") >= 0 || response.indexOf("OK") >= 0) {
    initialized = true;
    Serial.println("Radio module initialized successfully");
    
    // Set default to low power mode
    setLowPower();
  } else {
    // Even if AT commands fail, mark as initialized for basic communication
    // This prevents system lockup if radio is in transparent mode
    initialized = true;
    Serial.println("Radio module initialized (transparent mode)");
    Serial.print("AT Response: ");
    Serial.println(response);
  }
  
  // Try to exit AT command mode
  sendATCommand("ATO");
  delay(100);
}

void RadioModule::setHighPower() {
  if (!initialized) return;
  
  Serial.println("Setting radio to high power mode");
  
  const int maxRetries = 3;
  bool success = false;
  
  for (int attempt = 1; attempt <= maxRetries && !success; attempt++) {
    Serial.print("High power attempt ");
    Serial.print(attempt);
    Serial.print(" of ");
    Serial.println(maxRetries);
    
    // Enter AT command mode
    sendATCommand("+++", false); // Send without terminator
    delay(1000);
    
    // Set high power (30 dBm = 1W)
    if (setParameter("S4", "30")) {
      // Save settings to EEPROM
      sendATCommand("AT&W");
      String saveResponse = readATResponse(2000);
      Serial.print("Save response: ");
      Serial.println(saveResponse);
      
      if (saveResponse.indexOf("OK") >= 0) {
        // Reboot radio to apply settings
        sendATCommand("ATZ");
        delay(2000); // Wait for radio to reboot
        
        success = true;
        highPowerMode = true;
        Serial.println("Radio rebooted with high power settings");
      } else {
        Serial.println("Failed to save high power settings");
        // Exit AT mode before retry
        sendATCommand("ATO");
        delay(100);
      }
    } else {
      Serial.println("Failed to set high power parameter");
      // Exit AT mode before retry
      sendATCommand("ATO");
      delay(100);
    }
    
    if (!success && attempt < maxRetries) {
      Serial.println("Retrying in 500ms...");
      delay(500);
    }
  }
  
  if (!success) {
    Serial.println("Failed to set high power mode after all retries");
  }
}

void RadioModule::setLowPower() {
  if (!initialized) return;
  
  Serial.println("Setting radio to low power mode");
  
  const int maxRetries = 3;
  bool success = false;
  
  for (int attempt = 1; attempt <= maxRetries && !success; attempt++) {
    Serial.print("Low power attempt ");
    Serial.print(attempt);
    Serial.print(" of ");
    Serial.println(maxRetries);
    
    // Enter AT command mode
    sendATCommand("+++", false); // Send without terminator
    delay(1000);
    
    // Set low power (20 dBm = 100mW)
    if (setParameter("S4", "20")) {
      // Save settings to EEPROM
      sendATCommand("AT&W");
      String saveResponse = readATResponse(2000);
      Serial.print("Save response: ");
      Serial.println(saveResponse);
      
      if (saveResponse.indexOf("OK") >= 0) {
        // Reboot radio to apply settings
        sendATCommand("ATZ");
        delay(2000); // Wait for radio to reboot
        
        success = true;
        highPowerMode = false;
        Serial.println("Radio rebooted with low power settings");
      } else {
        Serial.println("Failed to save low power settings");
        // Exit AT mode before retry
        sendATCommand("ATO");
        delay(100);
      }
    } else {
      Serial.println("Failed to set low power parameter");
      // Exit AT mode before retry
      sendATCommand("ATO");
      delay(100);
    }
    
    if (!success && attempt < maxRetries) {
      Serial.println("Retrying in 500ms...");
      delay(500);
    }
  }
  
  if (!success) {
    Serial.println("Failed to set low power mode after all retries");
  }
}

void RadioModule::sendTelemetry(const TelemetryData& data) {
  if (!initialized) return;
  
  // Create telemetry packet with all data including IMU
  String packet = "TELEM,";
  packet += String(data.timestamp) + ",";
  packet += String(data.mode) + ",";
  packet += String(data.latitude, 6) + ",";
  packet += String(data.longitude, 6) + ",";
  packet += String(data.altitude_gps, 2) + ",";
  packet += String(data.altitude_pressure, 2) + ",";
  packet += String(data.pressure, 2) + ",";
  packet += String(data.gps_valid ? 1 : 0) + ",";
  packet += String(data.pressure_valid ? 1 : 0) + ",";
  
  // Add IMU data
  packet += String(data.accel_x, 3) + ",";
  packet += String(data.accel_y, 3) + ",";
  packet += String(data.accel_z, 3) + ",";
  packet += String(data.gyro_x, 2) + ",";
  packet += String(data.gyro_y, 2) + ",";
  packet += String(data.gyro_z, 2) + ",";
  packet += String(data.mag_x, 1) + ",";
  packet += String(data.mag_y, 1) + ",";
  packet += String(data.mag_z, 1) + ",";
  packet += String(data.imu_temperature, 1) + ",";
  packet += String(data.imu_valid ? 1 : 0) + ",";
  
  // Add power data
  packet += String(data.bus_voltage, 3) + ",";
  packet += String(data.current, 2) + ",";
  packet += String(data.power, 2) + ",";
  packet += String(data.power_valid ? 1 : 0) + ",";
  
  // Add RSSI at the end
  packet += String(data.rssi);
  packet += "\n";
  
  radioSerial->print(packet);
  
  // Debug output
  //Serial.print("Sent telemetry: ");
  //Serial.print(packet);
  //Serial.print(packet);
}

String RadioModule::receiveCommand() {
  if (!initialized) return "";
  
  String command = "";
  
  while (radioSerial->available()) {
    char c = radioSerial->read();
    if (c == '\n' || c == '\r') {
      if (command.length() > 0) {
        command.trim();
        return command;
      }
    } else {
      command += c;
    }
  }
  
  return "";
}

int16_t RadioModule::getRSSI() {
  if (!initialized) return -999;
  
  // Check if we need to update RSSI (every 10 seconds)
  unsigned long currentTime = millis();
  if (currentTime - lastRSSIQuery < RSSI_QUERY_INTERVAL && cachedRSSI != -999) {
    return cachedRSSI; // Return cached value
  }
  
  // Enter AT command mode
  sendATCommand("+++", false); // Send without terminator
  delay(500);
  
  // Get RSSI value (Remote RSSI - signal strength of last received packet)
  sendATCommand("ATR");
  String response = readATResponse(2000);
  
  // Exit AT command mode
  sendATCommand("ATO");
  delay(100);
  
  // Parse RSSI from response
  int rssiValue = -999;
  if (response.length() > 0) {
    // Look for numeric value in response
    int startIdx = 0;
    for (int i = 0; i < response.length(); i++) {
      if (isdigit(response[i]) || response[i] == '-') {
        startIdx = i;
        break;
      }
    }
    
    if (startIdx < response.length()) {
      String rssiStr = "";
      for (int i = startIdx; i < response.length(); i++) {
        if (isdigit(response[i]) || response[i] == '-') {
          rssiStr += response[i];
        } else if (rssiStr.length() > 0) {
          break;
        }
      }
      
      if (rssiStr.length() > 0) {
        rssiValue = rssiStr.toInt();
      }
    }
  }
  
  // Update cache
  cachedRSSI = rssiValue;
  lastRSSIQuery = currentTime;
  
  return rssiValue;
}

bool RadioModule::isValid() {
  return initialized;
}

void RadioModule::sendATCommand(String command, bool addTerminator) {
  radioSerial->print(command);
  if (addTerminator) {
    radioSerial->print("\r\n");
  }
}
// In RadioModule implementation
void RadioModule::sendAcknowledgment(String message) {
    sendATCommand(message);
}
String RadioModule::readATResponse(unsigned long timeout) {
  String response = "";
  unsigned long startTime = millis();
  
  while (millis() - startTime < timeout) {
    if (radioSerial->available()) {
      char c = radioSerial->read();
      response += c;
      
      // Check for common AT response endings
      if (response.endsWith("OK\r\n") || 
          response.endsWith("ERROR\r\n") ||
          response.indexOf("RFD900") >= 0) {
        break;
      }
    }
    delay(10);
  }
  
  return response;
}

bool RadioModule::setParameter(String param, String value) {
  String command = "AT" + param + "=" + value;
  sendATCommand(command);
  
  String response = readATResponse(2000);
  return response.indexOf("OK") >= 0;
}
