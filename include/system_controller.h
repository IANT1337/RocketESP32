#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <Preferences.h>
#include "config.h"
#include "gps_module.h"
#include "pressure_sensor.h"
#include "mpu9250_sensor.h"
#include "ina260_sensor.h"
#include "radio_module.h"
#include "power_manager.h"
#include "wifi_manager.h"
#include "sd_manager.h"

class SystemController {
private:
  SystemMode currentMode;
  unsigned long lastSensorRead;
  unsigned long lastGPSRead;
  unsigned long lastPressureRead;    // Track pressure sensor read time
  unsigned long lastPowerRead;       // Track power sensor read time
  unsigned long lastRadioListen;
  unsigned long lastRadioTx;         // Track last radio transmission time
  unsigned long lastHeartbeat;
  unsigned long maintenanceModeStartTime;
  
  GPSModule gpsModule;         // Stack allocated for better performance
  PressureSensor pressureSensor;
  MPU9250Sensor imuSensor;
  INA260Sensor powerSensor;
  RadioModule radioModule;
  PowerManager powerManager;
  WiFiManager wifiManager;
  SDManager sdManager;
  
  TelemetryData telemetryData;
  
  // Mode persistence
  Preferences preferences;
  
  // Threading support
  TaskHandle_t backgroundTaskHandle;
  TaskHandle_t sensorTaskHandle;
  SemaphoreHandle_t telemetryMutex;
  volatile bool backgroundTaskRunning;
  volatile bool sensorTaskRunning;
  
  // Task synchronization
  TaskHandle_t mainTaskHandle; // For notifying main task of shutdown completion
  
  // Mode transition state machine
  enum ModeTransitionState {
    TRANSITION_IDLE,
    TRANSITION_STARTING,
    TRANSITION_INIT_GPS,
    TRANSITION_INIT_PRESSURE,
    TRANSITION_INIT_IMU,
    TRANSITION_INIT_POWER,
    TRANSITION_RADIO_CONFIG,
    TRANSITION_WIFI_CONFIG,
    TRANSITION_COMPLETE
  };
  
  ModeTransitionState transitionState;
  SystemMode pendingMode;
  unsigned long transitionStartTime;
  
  // Performance monitoring
  struct PerformanceMetrics {
    unsigned long sensorReadTime;
    unsigned long radioTxTime;
    unsigned long sdWriteTime;
    unsigned long maxSensorReadTime;
    unsigned long maxRadioTxTime;
    unsigned long maxSdWriteTime;
  };
  
  PerformanceMetrics perfMetrics;
  
  void updateModeTransition(); // Non-blocking mode transition handler
  void completeModeTransition();
  void updateSensors();
  void checkRadioCommands();
  void pulseCameraPin();
  void sendTelemetry();
  void handleMaintenanceMode();
  void handleFlightMode();
  void handleSleepMode();
  
  // Mode persistence functions
  void savePersistentMode(SystemMode mode);
  SystemMode loadPersistentMode();
  
  // Performance monitoring
  void updatePerformanceMetrics(unsigned long duration, unsigned long* metric, unsigned long* maxMetric);
  void logPerformanceWarning(const char* operation, unsigned long duration);
  
  // Background task functions
  static void backgroundTask(void* parameter);
  void runBackgroundTasks();
  
  // Sensor task functions
  static void sensorTask(void* parameter);
  void runSensorTasks();

public:
  SystemController();
  ~SystemController();
  
  void initialize();
  void update();
  SystemMode getCurrentMode() const { return currentMode; }
  void setMode(SystemMode mode);
  
  // Performance metrics access
  const PerformanceMetrics& getPerformanceMetrics() const { return perfMetrics; }
  void resetPerformanceMetrics();
  
  // SD card access methods
  bool isSDCardAvailable() const;
  void flushSDCardData();
  void listSDCardFiles();
  String getSDCardStatus() const;
  String getDetailedSDCardStatus() const;
  
  // Thread-safe telemetry access
  TelemetryData getTelemetryDataCopy() const;
};

#endif
