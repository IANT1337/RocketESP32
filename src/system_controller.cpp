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
  imuSensor = new MPU9250Sensor();
  powerSensor = new INA260Sensor();
  radioModule = new RadioModule();
  powerManager = new PowerManager();
  wifiManager = new WiFiManager();
  
  // Initialize telemetry data
  memset(&telemetryData, 0, sizeof(TelemetryData));
}

SystemController::~SystemController() {
  delete gpsModule;
  delete pressureSensor;
  delete imuSensor;
  delete powerSensor;
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
  
  wifiManager->initialize();
  yield(); // Feed watchdog
  powerSensor->initialize();
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
      gpsModule->initialize();
      yield(); // Feed watchdog
      delay(250); // Small delay
      pressureSensor->initialize();
      yield(); // Feed watchdog
      delay(250); // Small delay
      imuSensor->initialize();
      yield(); // Feed watchdog
      delay(250); // Small delay
      powerSensor->initialize();
      yield(); // Feed watchdog
      delay(250); // Small delay
      radioModule->setHighPower();
      wifiManager->powerOff(); // Turn off WiFi during flight for power saving
      break;
    case MODE_MAINTENANCE:
      powerManager->enableSensors();
      gpsModule->initialize();
      yield(); // Feed watchdog
      delay(250); // Small delay
      pressureSensor->initialize();
      yield(); // Feed watchdog
      delay(250); // Small delay
      imuSensor->initialize();
      yield(); // Feed watchdog
      delay(250); // Small delay
      powerSensor->initialize();
      yield(); // Feed watchdog
      delay(250); // Small delay
      radioModule->setLowPower();
      wifiManager->powerOn(); // Turn on WiFi for maintenance
      wifiManager->connect(WIFI_SSID, WIFI_PASSWORD);
      break;
    case MODE_SLEEP:
      powerManager->disableSensors();
      powerSensor->initialize();
      yield(); // Feed watchdog
      delay(250); // Small delay
      radioModule->setLowPower();
      wifiManager->powerOff(); // Completely turn off WiFi to save power
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
    // Only broadcast to WiFi if we're in maintenance mode (WiFi is on)
    if (currentMode == MODE_MAINTENANCE && wifiManager->isValid()) {
      wifiManager->broadcastData(telemetryData);
    }
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
  
  // Read IMU data every sensor read (faster rate)
  IMUData imuData;
  if (imuSensor->readData(imuData)) {
    telemetryData.accel_x = imuData.accel_x;
    telemetryData.accel_y = imuData.accel_y;
    telemetryData.accel_z = imuData.accel_z;
    telemetryData.gyro_x = imuData.gyro_x;
    telemetryData.gyro_y = imuData.gyro_y;
    telemetryData.gyro_z = imuData.gyro_z;
    telemetryData.mag_x = imuData.mag_x;
    telemetryData.mag_y = imuData.mag_y;
    telemetryData.mag_z = imuData.mag_z;
    telemetryData.imu_temperature = imuData.temperature;
    telemetryData.imu_valid = imuData.valid;
  } else {
    // Clear IMU data if read failed
    telemetryData.accel_x = telemetryData.accel_y = telemetryData.accel_z = 0.0f;
    telemetryData.gyro_x = telemetryData.gyro_y = telemetryData.gyro_z = 0.0f;
    telemetryData.mag_x = telemetryData.mag_y = telemetryData.mag_z = 0.0f;
    telemetryData.imu_temperature = 0.0f;
    telemetryData.imu_valid = false;
  }
  
  // Read power data every sensor read (faster rate)
  PowerData powerData;
  if (powerSensor->readData(powerData)) {
    telemetryData.bus_voltage = powerData.voltage;
    telemetryData.current = powerData.current;
    telemetryData.power = powerData.power;
    telemetryData.power_valid = powerData.valid;
  } else {
    // Clear power data if read failed
    telemetryData.bus_voltage = 0.0f;
    telemetryData.current = 0.0f;
    telemetryData.power = 0.0f;
    telemetryData.power_valid = false;
  }
  
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
    radioModule->sendAcknowledgment("Received command: " + command);
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
