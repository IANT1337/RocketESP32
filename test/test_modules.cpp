#include <Arduino.h>
#include "config.h"
#include "gps_module.h"
#include "pressure_sensor.h"
#include "radio_module.h"
#include "power_manager.h"
#include "wifi_manager.h"

// Test instances
GPSModule gps;
PressureSensor pressure;
RadioModule radio;
PowerManager power;
WiFiManager wifi;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== Rocket Flight Computer Module Test ===");
  
  // Test power manager
  Serial.println("\n--- Testing Power Manager ---");
  power.initialize();
  power.enableSensors();
  delay(1000);
  
  // Test GPS module
  Serial.println("\n--- Testing GPS Module ---");
  gps.initialize();
  
  // Test pressure sensor
  Serial.println("\n--- Testing Pressure Sensor ---");
  pressure.initialize();
  
  // Test radio module
  Serial.println("\n--- Testing Radio Module ---");
  radio.initialize();
  
  // Test WiFi manager
  Serial.println("\n--- Testing WiFi Manager ---");
  wifi.initialize();
  
  Serial.println("\n=== Module Initialization Complete ===");
  Serial.println("Status Summary:");
  Serial.println("Power Manager: " + String(power.isValid() ? "OK" : "FAILED"));
  Serial.println("GPS Module: " + String(gps.isValid() ? "OK" : "FAILED"));
  Serial.println("Pressure Sensor: " + String(pressure.isValid() ? "OK" : "FAILED"));
  Serial.println("Radio Module: " + String(radio.isValid() ? "OK" : "FAILED"));
  Serial.println("WiFi Manager: " + String(wifi.isValid() ? "OK" : "FAILED"));
}

void loop() {
  static unsigned long lastTest = 0;
  
  if (millis() - lastTest >= 5000) {
    Serial.println("\n--- Sensor Test ---");
    
    // Test GPS reading
    float lat, lon, alt_gps;
    if (gps.readData(lat, lon, alt_gps)) {
      Serial.println("GPS: Lat=" + String(lat, 6) + ", Lon=" + String(lon, 6) + ", Alt=" + String(alt_gps, 2));
    } else {
      Serial.println("GPS: No valid data");
    }
    
    // Test pressure reading
    float pressure_val, alt_pressure;
    if (pressure.readData(pressure_val, alt_pressure)) {
      Serial.println("Pressure: " + String(pressure_val, 2) + " hPa, Alt=" + String(alt_pressure, 2) + " m");
    } else {
      Serial.println("Pressure: No valid data");
    }
    
    // Test radio communication
    String command = radio.receiveCommand();
    if (command.length() > 0) {
      Serial.println("Radio command received: " + command);
    }
    
    // Create test telemetry
    TelemetryData testData;
    testData.timestamp = millis();
    testData.mode = MODE_MAINTENANCE;
    testData.latitude = lat;
    testData.longitude = lon;
    testData.altitude_gps = alt_gps;
    testData.altitude_pressure = alt_pressure;
    testData.pressure = pressure_val;
    testData.gps_valid = true;
    testData.pressure_valid = true;
    
    // Send test telemetry
    radio.sendTelemetry(testData);
    
    lastTest = millis();
  }
  
  delay(100);
}
