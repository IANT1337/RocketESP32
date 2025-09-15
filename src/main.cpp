#include <Arduino.h>
#include "config.h"
#include "system_controller.h"
#include "gps_module.h"
#include "pressure_sensor.h"
#include "radio_module.h"
#include "power_manager.h"
#include "wifi_manager.h"
#include "esp_task_wdt.h"

SystemController systemController;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Rocket Flight Computer Starting...");
  
  // Configure watchdog timer for 30 seconds
  // ESP32-S3 compatible watchdog initialization
  esp_task_wdt_init(30, true);
  esp_task_wdt_add(NULL);
  
  // Initialize system controller
  systemController.initialize();
  
  // Feed watchdog after initialization complete
  esp_task_wdt_reset();
  
  Serial.println("System initialized. Starting main loop...");
}

void loop() {
  // Feed watchdog timer
  esp_task_wdt_reset();
  
  // Main system loop - delegates to system controller
  systemController.update();
  
  // Small delay to prevent excessive CPU usage
  delay(10);
}
