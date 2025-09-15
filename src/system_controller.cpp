#include "system_controller.h"

SystemController::SystemController() : 
  currentMode(MODE_SLEEP),
  lastSensorRead(0),
  lastGPSRead(0),
  lastPressureRead(0),
  lastPowerRead(0),
  lastRadioListen(0),
  lastRadioTx(0),
  lastHeartbeat(0),
  maintenanceModeStartTime(0),
  backgroundTaskHandle(NULL),
  sensorTaskHandle(NULL),
  telemetryMutex(NULL),
  backgroundTaskRunning(false),
  sensorTaskRunning(false),
  mainTaskHandle(NULL),
  transitionState(TRANSITION_IDLE),
  pendingMode(MODE_SLEEP),
  transitionStartTime(0),
  // Initialize modules with member initializer list (stack allocated)
  gpsModule(),
  pressureSensor(),
  imuSensor(),
  powerSensor(),
  radioModule(),
  powerManager(),
  wifiManager(),
  sdManager() {
  
  // Store main task handle for synchronization
  mainTaskHandle = xTaskGetCurrentTaskHandle();
  
  // Initialize telemetry data
  memset(&telemetryData, 0, sizeof(TelemetryData));
  
  // Initialize performance metrics
  memset(&perfMetrics, 0, sizeof(PerformanceMetrics));
  
  // Create mutex for telemetry data access
  telemetryMutex = xSemaphoreCreateMutex();
}

SystemController::~SystemController() {
  // Stop background task with proper synchronization
  if (backgroundTaskHandle != NULL) {
    backgroundTaskRunning = false;
    // Wait for task to acknowledge shutdown (with timeout)
    uint32_t notificationValue = 0;
    if (xTaskNotifyWait(0, ULONG_MAX, &notificationValue, pdMS_TO_TICKS(500)) == pdTRUE) {
      Serial.println("Background task shutdown acknowledged");
    } else {
      Serial.println("Background task shutdown timeout - forcing deletion");
    }
    
    // Ensure task is actually deleted
    if (eTaskGetState(backgroundTaskHandle) != eDeleted) {
      vTaskDelete(backgroundTaskHandle);
    }
    backgroundTaskHandle = NULL;
  }
  
  // Stop sensor task with proper synchronization
  if (sensorTaskHandle != NULL) {
    sensorTaskRunning = false;
    // Wait for task to acknowledge shutdown (with timeout)
    uint32_t notificationValue = 0;
    if (xTaskNotifyWait(0, ULONG_MAX, &notificationValue, pdMS_TO_TICKS(500)) == pdTRUE) {
      Serial.println("Sensor task shutdown acknowledged");
    } else {
      Serial.println("Sensor task shutdown timeout - forcing deletion");
    }
    
    // Ensure task is actually deleted
    if (eTaskGetState(sensorTaskHandle) != eDeleted) {
      vTaskDelete(sensorTaskHandle);
    }
    sensorTaskHandle = NULL;
  }
  
  // Delete mutex
  if (telemetryMutex != NULL) {
    vSemaphoreDelete(telemetryMutex);
    telemetryMutex = NULL;
  }
  
  // No need to delete modules - they're stack allocated and will be destroyed automatically
}

void SystemController::initialize() {
  Serial.println("Initializing system controller...");
  Serial.print("Board: ");
  Serial.println(BOARD_NAME);
  Serial.print("MCU: ");
  Serial.println(MCU_TYPE);
  
  // Initialize camera power pin
  pinMode(CAMERA_POWER_PIN, OUTPUT);
  digitalWrite(CAMERA_POWER_PIN, LOW); // Start with camera off (will be set by mode)
  
  // Initialize power manager first
  powerManager.initialize();
  yield(); // Feed watchdog
  delay(100); // Small delay between initializations
  
  // Initialize communication modules with watchdog feeding
  radioModule.initialize();
  yield(); // Feed watchdog
  delay(100); // Small delay
  
  wifiManager.initialize();
  yield(); // Feed watchdog
  powerSensor.initialize();
  
  // Set system controller reference in WiFi manager for SD card status
  wifiManager.setSystemController(this);
  
  // Initialize SD card storage
  sdManager.initialize();
  yield(); // Feed watchdog
  delay(100); // Small delay
  
  // Start in sleep mode
  setMode(MODE_SLEEP);
  
  // Create and start background task
  backgroundTaskRunning = true;
  BaseType_t taskCreated = xTaskCreatePinnedToCore(
    backgroundTask,                    // Task function
    "BackgroundTask",                  // Task name
    BACKGROUND_TASK_STACK_SIZE,       // Stack size
    this,                             // Parameter (this SystemController instance)
    BACKGROUND_TASK_PRIORITY,         // Priority
    &backgroundTaskHandle,            // Task handle
    BACKGROUND_TASK_CORE              // Core to run on
  );
  
  if (taskCreated == pdPASS) {
    Serial.println("Background task created successfully");
  } else {
    Serial.println("Failed to create background task");
  }
  
  // Create and start sensor task
  sensorTaskRunning = true;
  BaseType_t sensorTaskCreated = xTaskCreatePinnedToCore(
    sensorTask,                       // Task function
    "SensorTask",                     // Task name
    SENSOR_TASK_STACK_SIZE,          // Stack size
    this,                            // Parameter (this SystemController instance)
    SENSOR_TASK_PRIORITY,            // Priority
    &sensorTaskHandle,               // Task handle
    SENSOR_TASK_CORE                 // Core to run on
  );
  
  if (sensorTaskCreated == pdPASS) {
    Serial.println("Sensor task created successfully");
  } else {
    Serial.println("Failed to create sensor task");
  }
  
  Serial.println("System controller initialized");
}

void SystemController::update() {
  unsigned long currentTime = millis();

  // Handle non-blocking mode transitions
  updateModeTransition();

  // Check for radio commands periodically (critical for flight safety)
  if (currentTime - lastRadioListen >= RADIO_LISTEN_INTERVAL) {
    unsigned long radioStart = micros();
    checkRadioCommands();
    unsigned long radioTime = micros() - radioStart;
    updatePerformanceMetrics(radioTime, &perfMetrics.radioTxTime, &perfMetrics.maxRadioTxTime);
    lastRadioListen = currentTime;
  }
  
  // Handle current mode (focused on telemetry transmission)
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
}

void SystemController::setMode(SystemMode mode) {
  if (mode != currentMode && transitionState == TRANSITION_IDLE) {
    Serial.print("Mode transition requested: ");
    Serial.print(currentMode);
    Serial.print(" -> ");
    Serial.println(mode);
    
    pendingMode = mode;
    transitionState = TRANSITION_STARTING;
    transitionStartTime = millis();
    
    if (mode == MODE_MAINTENANCE) {
      maintenanceModeStartTime = millis();
    }
  }
}

void SystemController::updateModeTransition() {
  if (transitionState == TRANSITION_IDLE) {
    return;
  }
  
  unsigned long currentTime = millis();
  const unsigned long STEP_DELAY = 250; // 250ms between initialization steps
  
  // Check if enough time has passed for the next step
  if (currentTime - transitionStartTime < STEP_DELAY) {
    return;
  }
  
  // Feed watchdog during transitions
  yield();
  
  switch (transitionState) {
    case TRANSITION_STARTING:
      Serial.println("Starting mode transition...");
      // Handle immediate changes (camera power, flush SD data)
      if (sdManager.isInitialized()) {
        sdManager.forceSync();
      }
      
      // Handle camera power control
      if (pendingMode == MODE_SLEEP) {
        digitalWrite(CAMERA_POWER_PIN, LOW);
        Serial.println("Camera power OFF (sleep mode)");
      } else {
        digitalWrite(CAMERA_POWER_PIN, HIGH);
        Serial.println("Camera power ON");
      }
      
      transitionState = TRANSITION_INIT_GPS;
      transitionStartTime = currentTime;
      break;
      
    case TRANSITION_INIT_GPS:
      if (pendingMode != MODE_SLEEP) {
        gpsModule.initialize();
      }
      transitionState = TRANSITION_INIT_PRESSURE;
      transitionStartTime = currentTime;
      break;
      
    case TRANSITION_INIT_PRESSURE:
      if (pendingMode != MODE_SLEEP) {
        pressureSensor.initialize();
      }
      transitionState = TRANSITION_INIT_IMU;
      transitionStartTime = currentTime;
      break;
      
    case TRANSITION_INIT_IMU:
      if (pendingMode != MODE_SLEEP) {
        imuSensor.initialize();
      }
      transitionState = TRANSITION_INIT_POWER;
      transitionStartTime = currentTime;
      break;
      
    case TRANSITION_INIT_POWER:
      powerSensor.initialize();
      transitionState = TRANSITION_RADIO_CONFIG;
      transitionStartTime = currentTime;
      break;
      
    case TRANSITION_RADIO_CONFIG:
      // Configure radio power based on mode
      if (pendingMode == MODE_FLIGHT) {
        radioModule.setHighPower();
      } else {
        radioModule.setLowPower();
      }
      transitionState = TRANSITION_WIFI_CONFIG;
      transitionStartTime = currentTime;
      break;
      
    case TRANSITION_WIFI_CONFIG:
      // Configure WiFi and power management
      completeModeTransition();
      break;
      
    default:
      transitionState = TRANSITION_IDLE;
      break;
  }
}

void SystemController::completeModeTransition() {
  // Handle power management and WiFi based on mode
  switch (pendingMode) {
    case MODE_FLIGHT:
      powerManager.enableSensors();
      wifiManager.powerOff(); // Turn off WiFi during flight for power saving
      break;
    case MODE_MAINTENANCE:
      powerManager.enableSensors();
      wifiManager.powerOn(); // Turn on WiFi for maintenance
      wifiManager.connect(WIFI_SSID, WIFI_PASSWORD);
      // List SD card files when entering maintenance mode
      if (sdManager.isInitialized()) {
        sdManager.listLogFiles();
      }
      break;
    case MODE_SLEEP:
      // Ensure all data is written before sleep
      if (sdManager.isInitialized()) {
        sdManager.forceSync();
      }
      powerManager.disableSensors();
      wifiManager.powerOff(); // Completely turn off WiFi to save power
      break;
  }
  
  // Transition complete
  currentMode = pendingMode;
  transitionState = TRANSITION_IDLE;
  
  Serial.print("Mode transition complete: ");
  Serial.println(currentMode);
}

void SystemController::handleFlightMode() {
  unsigned long currentTime = millis();
  
  // Send telemetry at radio transmission interval using latest sensor data
  if (currentTime - lastRadioTx >= RADIO_TX_INTERVAL) {
    sendTelemetry();
  }
}

void SystemController::handleMaintenanceMode() {
  unsigned long currentTime = millis();
  
  // Send telemetry at radio transmission interval using latest sensor data
  if (currentTime - lastRadioTx >= RADIO_TX_INTERVAL) {
    sendTelemetry();
  }
  
  // Auto-exit maintenance mode after timeout
  if (currentTime - maintenanceModeStartTime >= MAINTENANCE_TIMEOUT) {
    Serial.println("Maintenance mode timeout, returning to sleep");
    setMode(MODE_SLEEP);
  }
}

void SystemController::handleSleepMode() {
  unsigned long currentTime = millis();
  
  // In sleep mode, send telemetry with heartbeat so ground station knows device status
  if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    sendTelemetry(); // Send status telemetry to ground station using latest data
    lastHeartbeat = currentTime;
  }
  
  // Power management is handled in mode transitions
}

void SystemController::updateSensors() {
  unsigned long currentTime = millis();
  unsigned long sensorStart = micros();
  
  // Multi-rate sensor reading strategy for optimal performance
  // Priority 1: High-speed sensors (IMU) - 40Hz (25ms interval)
  // Priority 2: Medium-speed sensors (Pressure) - 10Hz (100ms interval) 
  // Priority 3: Low-speed sensors (Power) - 5Hz (200ms interval)
  // Priority 4: Very slow sensors (GPS) - 1Hz (1000ms interval)
  
  bool readGPS = false;
  bool readPressure = false; 
  bool readPower = false;
  bool readIMU = false;
  
  // Determine which sensors to read this cycle
  if (currentMode != MODE_SLEEP && (currentTime - lastGPSRead >= GPS_READ_INTERVAL)) {
    readGPS = true;
    lastGPSRead = currentTime;
  }
  
  if (currentMode != MODE_SLEEP && (currentTime - lastPressureRead >= PRESSURE_READ_INTERVAL)) {
    readPressure = true;
    lastPressureRead = currentTime;
  }
  
  if (currentTime - lastPowerRead >= POWER_READ_INTERVAL) {
    readPower = true; // Always read power for battery monitoring
    lastPowerRead = currentTime;
  }
  
  if (currentMode != MODE_SLEEP) {
    readIMU = true; // Always read IMU at high frequency when not sleeping
  }
  
  // Pre-read sensor data outside of mutex to minimize lock time
  bool gpsValid = false;
  float lat = 0, lon = 0, altGps = 0;
  bool pressureValid = false;
  float pressure = 0, altPressure = 0;
  PowerData powerData = {0};
  bool powerValid = false;
  IMUData imuData = {0};
  bool imuValid = false;
  
  // Read sensors based on their schedules (non-blocking approach)
  if (readGPS) {
    gpsValid = gpsModule.readData(lat, lon, altGps);
  }
  
  if (readPressure) {
    pressureValid = pressureSensor.readData(pressure, altPressure);
  }
  
  if (readPower) {
    powerValid = powerSensor.readData(powerData);
  }
  
  if (readIMU) {
    imuValid = imuSensor.readData(imuData);
  }
  
  // Quick mutex lock to update telemetry data
  if (xSemaphoreTake(telemetryMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    // Update GPS data (only when read)
    if (readGPS && gpsValid) {
      telemetryData.latitude = lat;
      telemetryData.longitude = lon;
      telemetryData.altitude_gps = altGps;
      telemetryData.gps_valid = true;
    }
    
    // Update pressure data (only when read)
    if (readPressure && pressureValid) {
      telemetryData.pressure = pressure;
      telemetryData.altitude_pressure = altPressure;
      telemetryData.pressure_valid = true;
    }
    
    // Update IMU data (high frequency)
    if (readIMU && imuValid && imuData.valid) {
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
      telemetryData.imu_valid = true;
    }
    
    // Update power data (only when read)
    if (readPower && powerValid && powerData.valid) {
      telemetryData.bus_voltage = powerData.voltage;
      telemetryData.current = powerData.current * -1.66;
      telemetryData.power = powerData.power * 1.66;
      telemetryData.power_valid = true;
    }
    
    // Always update timestamp and mode
    telemetryData.timestamp = millis();
    telemetryData.mode = currentMode;
    
    xSemaphoreGive(telemetryMutex);
    
    // Update performance metrics
    unsigned long sensorTime = micros() - sensorStart;
    updatePerformanceMetrics(sensorTime, &perfMetrics.sensorReadTime, &perfMetrics.maxSensorReadTime);
    
    // Debug output for multi-rate sensor reading
    if (sensorTime > 5000) { // Only log if > 5ms
      Serial.printf("Sensor read: %lu μs (GPS:%d, Press:%d, Pwr:%d, IMU:%d)\n", 
                    sensorTime, readGPS, readPressure, readPower, readIMU);
    }
  } else {
    Serial.println("Warning: Failed to acquire telemetry mutex in updateSensors");
  }
}

void SystemController::checkRadioCommands() {
  String command = radioModule.receiveCommand();
  
  if (command.length() > 0) {
    Serial.print("Received command: ");
    Serial.println(command);
    radioModule.sendAcknowledgment("Received command: " + command);
    if (command == CMD_FLIGHT_MODE) {
      setMode(MODE_FLIGHT);
    } else if (command == CMD_SLEEP_MODE) {
      setMode(MODE_SLEEP);
    } else if (command == CMD_MAINTENANCE_MODE) {
      setMode(MODE_MAINTENANCE);
    } else if (command == CMD_CAM_TOGGLE) {
      pulseCameraPin();
    }
  }
}

void SystemController::pulseCameraPin() {
  Serial.println("Pulsing camera pin 6 times");
  digitalWrite(CAMERA_POWER_PIN, HIGH);
  delay(500); // Pulse high for 500ms
  // Pulse the pin 7  times
  for (int i = 0; i < 7; i++) {
    digitalWrite(CAMERA_POWER_PIN, LOW);
    delay(200); // Pulse high for 200ms
    digitalWrite(CAMERA_POWER_PIN, HIGH);
    delay(200); // Pulse low for 200ms
    Serial.print("Pulse ");
    Serial.print(i);
    Serial.println(" complete");
  }
  
  // Add a small delay before restoring state
  delay(50);
  
  // Restore the original state based on current mode
  if (currentMode != MODE_SLEEP) {
    digitalWrite(CAMERA_POWER_PIN, HIGH); // Camera should be on in flight/maintenance
  } else {
    digitalWrite(CAMERA_POWER_PIN, LOW);  // Camera should be off in sleep
  }
  
  Serial.println("Camera pin pulsing complete");
}

void SystemController::updatePerformanceMetrics(unsigned long duration, unsigned long* metric, unsigned long* maxMetric) {
  *metric = duration;
  if (duration > *maxMetric) {
    *maxMetric = duration;
  }
  
  // Log warnings for excessive durations
  if (duration > 5000) { // > 5ms warning threshold
    if (metric == &perfMetrics.sensorReadTime) {
      logPerformanceWarning("sensor read", duration);
    } else if (metric == &perfMetrics.radioTxTime) {
      logPerformanceWarning("radio transmission", duration);
    } else if (metric == &perfMetrics.sdWriteTime) {
      logPerformanceWarning("SD write", duration);
    }
  }
}

void SystemController::logPerformanceWarning(const char* operation, unsigned long duration) {
  Serial.printf("Performance Warning: %s took %lu μs (>5ms)\n", operation, duration);
}

void SystemController::resetPerformanceMetrics() {
  memset(&perfMetrics, 0, sizeof(PerformanceMetrics));
}

void SystemController::sendTelemetry() {
  unsigned long currentTime = millis();
  
  // Respect radio transmission interval to prevent flooding
  if (currentTime - lastRadioTx < RADIO_TX_INTERVAL) {
    return; // Skip transmission if interval hasn't elapsed
  }
  
  // Get a thread-safe copy of the latest sensor data
  TelemetryData telemetryCopy = getTelemetryDataCopy();
  
  // Send telemetry over radio (time critical) with performance monitoring
  unsigned long radioStart = micros();
  radioModule.sendTelemetry(telemetryCopy);
  unsigned long radioTime = micros() - radioStart;
  updatePerformanceMetrics(radioTime, &perfMetrics.radioTxTime, &perfMetrics.maxRadioTxTime);
  
  // Update last transmission time
  lastRadioTx = currentTime;
  
  // Store data to SD card if available with performance monitoring
  if (sdManager.isInitialized()) {
    unsigned long sdStart = micros();
    sdManager.addData(telemetryCopy);
    unsigned long sdTime = micros() - sdStart;
    updatePerformanceMetrics(sdTime, &perfMetrics.sdWriteTime, &perfMetrics.maxSdWriteTime);
  }
}

bool SystemController::isSDCardAvailable() const {
  return sdManager.isInitialized();
}

void SystemController::flushSDCardData() {
  if (sdManager.isInitialized()) {
    sdManager.forceSync();
    Serial.println("SD card data flushed");
  } else {
    Serial.println("SD card not available");
  }
}

void SystemController::listSDCardFiles() {
  if (sdManager.isInitialized()) {
    sdManager.listLogFiles();
  } else {
    Serial.println("SD card not available");
  }
}

String SystemController::getSDCardStatus() const {
  // Use the detailed status from SD manager which includes retry information
  return sdManager.getDetailedStatus();
}

String SystemController::getDetailedSDCardStatus() const {
  if (!sdManager.isInitialized()) {
    return "SD system not initialized";
  }
  
  return sdManager.getDetailedStatus();
}

void SystemController::backgroundTask(void* parameter) {
  SystemController* controller = static_cast<SystemController*>(parameter);
  controller->runBackgroundTasks();
}

void SystemController::sensorTask(void* parameter) {
  SystemController* controller = static_cast<SystemController*>(parameter);
  controller->runSensorTasks();
}

void SystemController::runBackgroundTasks() {
  Serial.println("Background task started");
  
  unsigned long lastHeartbeat = 0;
  unsigned long lastSDUpdate = 0;
  
  while (backgroundTaskRunning) {
    unsigned long currentTime = millis();
    
    // Send heartbeat and status updates (moved from main loop)
    if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL) {
      Serial.print("System running in mode: ");
      Serial.println(currentMode);
      
      // SD card status reporting (non-blocking)
      String detailedStatus = sdManager.getDetailedStatus();
      Serial.println(detailedStatus);
      
      lastHeartbeat = currentTime;
    }
    
    // Handle SD card operations (health checks, retries, etc.)
    if (currentTime - lastSDUpdate >= 100) { // 10Hz update rate
      sdManager.update();
      lastSDUpdate = currentTime;
    }
    
    // WiFi operations in maintenance mode (can be slow/blocking)
    if (currentMode == MODE_MAINTENANCE && wifiManager.isValid()) {
      // Get a thread-safe copy of telemetry data
      TelemetryData telemetryCopy = getTelemetryDataCopy();
      wifiManager.broadcastData(telemetryCopy);
    }
    
    // Small delay to prevent this task from consuming too much CPU
    vTaskDelay(pdMS_TO_TICKS(5)); // 5ms delay = 200Hz update rate
  }
  
  Serial.println("Background task stopping");
  // Notify main task that we're shutting down
  xTaskNotify(mainTaskHandle, 1, eSetValueWithOverwrite);
  vTaskDelete(NULL);
}

TelemetryData SystemController::getTelemetryDataCopy() const {
  TelemetryData copy;
  if (xSemaphoreTake(telemetryMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    copy = telemetryData;
    xSemaphoreGive(telemetryMutex);
  } else {
    // If we can't get the mutex, return a zeroed structure
    memset(&copy, 0, sizeof(TelemetryData));
  }
  return copy;
}

void SystemController::runSensorTasks() {
  Serial.println("Sensor task started");
  
  while (sensorTaskRunning) {
    unsigned long currentTime = millis();
    
    // Use the consolidated updateSensors method
    updateSensors();
    
    // Adjust task frequency based on mode
    if (currentMode != MODE_SLEEP) {
      // High frequency in flight/maintenance mode (40Hz)
      vTaskDelay(pdMS_TO_TICKS(25));
    } else {
      // Lower frequency in sleep mode (1Hz) for power saving
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
  
  Serial.println("Sensor task stopping");
  // Notify main task that we're shutting down
  xTaskNotify(mainTaskHandle, 2, eSetValueWithOverwrite);
  vTaskDelete(NULL);
}
