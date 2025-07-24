#include "radio_module.h"

RadioModule::RadioModule() : initialized(false), highPowerMode(false) {
  radioSerial = new HardwareSerial(2);
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
  sendATCommand("+++");
  delay(1100); // AT command mode requires > 1 second guard time
  
  // Test communication with shorter timeout
  sendATCommand("ATI");
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
  
  // Enter AT command mode
  sendATCommand("+++");
  delay(1000);
  
  // Set high power (30 dBm = 1W)
  setParameter("S3", "30");
  
  // Exit AT command mode
  sendATCommand("ATO");
  delay(500);
  
  highPowerMode = true;
}

void RadioModule::setLowPower() {
  if (!initialized) return;
  
  Serial.println("Setting radio to low power mode");
  
  // Enter AT command mode
  sendATCommand("+++");
  delay(1000);
  
  // Set low power (20 dBm = 100mW)
  setParameter("S3", "20");
  
  // Exit AT command mode
  sendATCommand("ATO");
  delay(500);
  
  highPowerMode = false;
}

void RadioModule::sendTelemetry(const TelemetryData& data) {
  if (!initialized) return;
  
  // Create telemetry packet
  String packet = "TELEM,";
  packet += String(data.timestamp) + ",";
  packet += String(data.mode) + ",";
  packet += String(data.latitude, 6) + ",";
  packet += String(data.longitude, 6) + ",";
  packet += String(data.altitude_gps, 2) + ",";
  packet += String(data.altitude_pressure, 2) + ",";
  packet += String(data.pressure, 2) + ",";
  packet += String(data.gps_valid ? 1 : 0) + ",";
  packet += String(data.pressure_valid ? 1 : 0);
  packet += "\n";
  
  radioSerial->print(packet);
  
  // Debug output
  Serial.print("Sent telemetry: ");
  Serial.print(packet);
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

bool RadioModule::isValid() {
  return initialized;
}

void RadioModule::sendATCommand(String command) {
  radioSerial->print(command);
  radioSerial->print("\r\n");
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
