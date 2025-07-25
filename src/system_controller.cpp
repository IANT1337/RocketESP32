#include "system_controller.h"

SystemController::SystemController() : 
  currentMode(MODE_SLEEP),
  lastSensorRead(0),
  lastGPSRead(0),
  lastRadioListen(0),
  lastHeartbeat(0),
  maintenanceModeStartTime(0) {
  
  // Initialize module pointers
  gpsModule = new GPSModule();
  pressureSensor = new PressureSensor();
  radioModule = new RadioModule();
  powerManager = new PowerManager();
  wifiManager = new WiFiManager();
  
  // Initialize telemetry data
  memset(&telemetryData, 0, sizeof(TelemetryData));
}

SystemController::~SystemController() {
  delete gpsModule;
  delete pressureSensor;
  delete radioModule;
  delete powerManager;
  delete wifiManager;
}

void SystemController::initialize() {
  Serial.println("Initializing system controller...");
  
  // Initialize power manager first
  powerManager->initialize();
  yield(); // Feed watchdog
  delay(100); // Small delay between initializations
  
  // Initialize communication modules with watchdog feeding
  radioModule->initialize();
  yield(); // Feed watchdog
  delay(100); // Small delay
  
  gpsModule->initialize();
  yield(); // Feed watchdog
  delay(100); // Small delay
  
  pressureSensor->initialize();
  yield(); // Feed watchdog
  delay(100); // Small delay
  
  wifiManager->initialize();
  yield(); // Feed watchdog
  
  // Start in sleep mode
  setMode(MODE_SLEEP);
  
  Serial.println("System controller initialized");
}

void SystemController::update() {
  unsigned long currentTime = millis();
  
  // Check for radio commands periodically
  if (currentTime - lastRadioListen >= RADIO_LISTEN_INTERVAL) {
    checkRadioCommands();
    lastRadioListen = currentTime;
  }
  
  // Handle current mode
  switch (currentMode) {
    case MODE_SLEEP:
      handleSleepMode();
      break;
    case MODE_FLIGHT:
      handleFlightMode();
      break;
    case MODE_MAINTENANCE:
      handleMaintenanceMode();
      break;
  }
  
  // Send heartbeat
  if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    Serial.print("System running in mode: ");
    Serial.println(currentMode);
    lastHeartbeat = currentTime;
    updateSensors();
    sendTelemetry();
    lastSensorRead = currentTime;
  }
}

void SystemController::setMode(SystemMode mode) {
  if (mode != currentMode) {
    Serial.print("Mode transition: ");
    Serial.print(currentMode);
    Serial.print(" -> ");
    Serial.println(mode);
    
    handleModeTransition(mode);
    currentMode = mode;
    
    if (mode == MODE_MAINTENANCE) {
      maintenanceModeStartTime = millis();
    }
  }
}

void SystemController::handleModeTransition(SystemMode newMode) {
  // Handle power management transitions
  switch (newMode) {
    case MODE_FLIGHT:
      powerManager->enableSensors();
      radioModule->setHighPower();
      break;
    case MODE_MAINTENANCE:
      powerManager->enableSensors();
      radioModule->setLowPower();
      wifiManager->connect(WIFI_SSID, WIFI_PASSWORD);
      break;
    case MODE_SLEEP:
      powerManager->disableSensors();
      radioModule->setLowPower();
      wifiManager->disconnect();
      break;
  }
  
}

void SystemController::handleFlightMode() {
  unsigned long currentTime = millis();
  
  // Read sensors and send telemetry regularly
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
    updateSensors();
    sendTelemetry();
    lastSensorRead = currentTime;
  }
}

void SystemController::handleMaintenanceMode() {
  unsigned long currentTime = millis();
  
  // Read sensors and send telemetry
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
    updateSensors();
    sendTelemetry();
    wifiManager->broadcastData(telemetryData);
    lastSensorRead = currentTime;
  }
  
  // Auto-exit maintenance mode after timeout
  if (currentTime - maintenanceModeStartTime >= MAINTENANCE_TIMEOUT) {
    Serial.println("Maintenance mode timeout, returning to sleep");
    setMode(MODE_SLEEP);
  }
}

void SystemController::handleSleepMode() {
  // In sleep mode, only listen for radio commands
  // Power management is handled in mode transitions
}

void SystemController::updateSensors() {
  unsigned long currentTime = millis();
  
  // Read GPS data only once per second
  if (currentTime - lastGPSRead >= GPS_READ_INTERVAL) {
    telemetryData.gps_valid = gpsModule->readData(
      telemetryData.latitude, 
      telemetryData.longitude, 
      telemetryData.altitude_gps
    );
    lastGPSRead = currentTime;
  }
  
  // Read pressure data every sensor read (faster rate)
  telemetryData.pressure_valid = pressureSensor->readData(
    telemetryData.pressure, 
    telemetryData.altitude_pressure
  );
  
  // Read radio RSSI
  //telemetryData.rssi = radioModule->getRSSI();
  
  // Update timestamp and mode
  telemetryData.timestamp = millis();
  telemetryData.mode = currentMode;
}

void SystemController::checkRadioCommands() {
  String command = radioModule->receiveCommand();
  
  if (command.length() > 0) {
    Serial.print("Received command: ");
    Serial.println(command);
    
    if (command == CMD_FLIGHT_MODE) {
      setMode(MODE_FLIGHT);
    } else if (command == CMD_SLEEP_MODE) {
      setMode(MODE_SLEEP);
    } else if (command == CMD_MAINTENANCE_MODE) {
      setMode(MODE_MAINTENANCE);
    }
  }
}

void SystemController::sendTelemetry() {
  radioModule->sendTelemetry(telemetryData);
}
