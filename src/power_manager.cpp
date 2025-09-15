#include "power_manager.h"

PowerManager::PowerManager() : 
  sensorsEnabled(false), 
  radioEnabled(false),
  initialized(false) {
}

PowerManager::~PowerManager() {
}

void PowerManager::initialize() {
  Serial.println("Initializing power manager...");
  
  // Configure power control pins as outputs
  //pinMode(SENSOR_POWER_PIN, OUTPUT);
  //pinMode(RADIO_POWER_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  
  // Start with everything disabled to save power
  disableSensors();
  enableRadio(); // Radio should always be available for commands
  
  // Status LED on to indicate system is running
  digitalWrite(STATUS_LED_PIN, HIGH);
  
  initialized = true;
  Serial.println("Power manager initialized");
}

void PowerManager::enableSensors() {
  if (!initialized) return;
  
  if (!sensorsEnabled) {
    Serial.println("Enabling sensors");
    //digitalWrite(SENSOR_POWER_PIN, HIGH);
    sensorsEnabled = true;
    
    // Wait for sensors to stabilize (reduced delay)
    //delay(200);
  }
}

void PowerManager::disableSensors() {
  if (!initialized) return;
  
  if (sensorsEnabled) {
    Serial.println("Disabling sensors");
    //digitalWrite(SENSOR_POWER_PIN, LOW);
    sensorsEnabled = false;
  }
}

void PowerManager::enableRadio() {
  if (!initialized) return;
  
  if (!radioEnabled) {
    Serial.println("Enabling radio");
    //digitalWrite(RADIO_POWER_PIN, HIGH);
    radioEnabled = true;
    
    // Wait for radio to stabilize (reduced delay)
    delay(200);
  }
}

void PowerManager::disableRadio() {
  if (!initialized) return;
  
  if (radioEnabled) {
    Serial.println("Disabling radio");
    //digitalWrite(RADIO_POWER_PIN, LOW);
    radioEnabled = false;
  }
}
