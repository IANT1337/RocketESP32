#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <Arduino.h>
#include "config.h"
#include "gps_module.h"
#include "pressure_sensor.h"
#include "mpu9250_sensor.h"
#include "ina260_sensor.h"
#include "radio_module.h"
#include "power_manager.h"
#include "wifi_manager.h"

class SystemController {
private:
  SystemMode currentMode;
  unsigned long lastSensorRead;
  unsigned long lastGPSRead;
  unsigned long lastRadioListen;
  unsigned long lastHeartbeat;
  unsigned long maintenanceModeStartTime;
  
  GPSModule* gpsModule;
  PressureSensor* pressureSensor;
  MPU9250Sensor* imuSensor;
  INA260Sensor* powerSensor;
  RadioModule* radioModule;
  PowerManager* powerManager;
  WiFiManager* wifiManager;
  
  TelemetryData telemetryData;
  
  void handleModeTransition(SystemMode newMode);
  void updateSensors();
  void checkRadioCommands();
  void sendTelemetry();
  void handleMaintenanceMode();
  void handleFlightMode();
  void handleSleepMode();

public:
  SystemController();
  ~SystemController();
  
  void initialize();
  void update();
  SystemMode getCurrentMode() const { return currentMode; }
  void setMode(SystemMode mode);
};

#endif
