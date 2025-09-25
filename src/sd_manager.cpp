#include "sd_manager.h"

SDManager::SDManager() : 
  sdInitialized(false),
  primaryCardPresent(false),
  backupCardPresent(false),
  activeCard(SD_NONE),
  totalBatchesStored(0),
  lastCardHealthCheck(0),
  lastRetryAttempt(0),
  consecutiveFailures(0),
  bothCardsFailed(false) {
  
  // Initialize current batch
  memset(&currentBatch, 0, sizeof(DataBatch));
  currentBatch.batchStartTime = millis();
}

SDManager::~SDManager() {
  if (sdInitialized) {
    flushCurrentBatch();
    SD.end();
  }
}

bool SDManager::initialize() {
  Serial.println("Initializing dual SD card manager...");
  
  // Configure SPI pins
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  
  // Try to initialize SD card system
  if (initializeSD()) {
    // Success! Create log file
    if (createLogFile()) {
      Serial.print("SD card system initialized. Active card: ");
      Serial.print(getCardSlotName(activeCard));
      Serial.print(", Log file: ");
      Serial.println(currentLogFile);
      Serial.print("Available space: ");
      Serial.print(getAvailableSpace() / 1024);
      Serial.println(" KB");
      return true;
    } else {
      Serial.println("Failed to create log file");
      sdInitialized = false;
    }
  }
  
  // If we get here, both cards failed - but don't give up!
  Serial.println("Both SD cards failed at startup - will retry periodically");
  bothCardsFailed = true;
  lastRetryAttempt = millis();
  
  // Return true so the system continues - we'll keep trying in background
  return true;
}

bool SDManager::initializeSD() {
  // Try to initialize primary card first
  if (tryInitializeCard(SD_PRIMARY)) {
    activeCard = SD_PRIMARY;
    primaryCardPresent = true;
    sdInitialized = true;
    bothCardsFailed = false;
    consecutiveFailures = 0;
    Serial.println("Primary SD card initialized successfully");
    return true;
  }
  
  // If primary fails, try backup card
  if (tryInitializeCard(SD_BACKUP)) {
    activeCard = SD_BACKUP;
    backupCardPresent = true;
    sdInitialized = true;
    bothCardsFailed = false;
    consecutiveFailures = 0;
    Serial.println("Primary SD card failed, backup SD card initialized successfully");
    return true;
  }
  
  // Both cards failed
  Serial.println("Both SD cards failed to initialize");
  activeCard = SD_NONE;
  sdInitialized = false;
  bothCardsFailed = true;
  return false;
}

bool SDManager::tryInitializeCard(SDCardSlot slot) {
  int csPin = (slot == SD_PRIMARY) ? SD_CS_PIN : SD_CS_BACKUP_PIN;
  
  Serial.print("Trying to initialize ");
  Serial.print(getCardSlotName(slot));
  Serial.print(" SD card on pin ");
  Serial.println(csPin);
  
  // Try to initialize with high speed
  if (SD.begin(csPin, SPI, SD_SPI_SPEED)) {
    Serial.print(getCardSlotName(slot));
    Serial.println(" SD card initialized at full speed");
    
    // Print card information
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.print("SD card size: ");
    Serial.print(cardSize);
    Serial.println(" MB");
    return true;
  }
  
  // Try with slower speed
  if (SD.begin(csPin, SPI, 1000000)) {
    Serial.print(getCardSlotName(slot));
    Serial.println(" SD card initialized at reduced speed");
    return true;
  }
  
  Serial.print(getCardSlotName(slot));
  Serial.println(" SD card initialization failed");
  return false;
}

bool SDManager::switchToBackupCard() {
  if (!backupCardPresent && !tryInitializeCard(SD_BACKUP)) {
    Serial.println("Backup SD card not available for switch");
    return false;
  }
  
  // Flush any pending data on current card
  if (currentBatch.count > 0) {
    flushCurrentBatch();
  }
  
  // End current SD connection
  SD.end();
  
  // Reinitialize SD library with backup card's CS pin
  if (!SD.begin(SD_CS_BACKUP_PIN, SPI, SD_SPI_SPEED)) {
    Serial.println("Failed to reinitialize SD library for backup card");
    // Try with slower speed
    if (!SD.begin(SD_CS_BACKUP_PIN, SPI, 1000000)) {
      Serial.println("Failed to reinitialize SD library for backup card even at reduced speed");
      return false;
    }
  }
  
  // Switch to backup card
  activeCard = SD_BACKUP;
  backupCardPresent = true;
  
  // Create new log file on backup card
  if (!createLogFile()) {
    Serial.println("Failed to create log file on backup card");
    return false;
  }
  
  Serial.println("Successfully switched to backup SD card");
  return true;
}

bool SDManager::createLogFile() {
  currentLogFile = generateFileName();
  
  // Create the file and write header
  File file = SD.open(currentLogFile, FILE_WRITE);
  if (!file) {
    Serial.print("Failed to create log file: ");
    Serial.println(currentLogFile);
    return false;
  }
  
  // Write CSV header
  file.println("timestamp,mode,lat,lon,alt_gps,alt_press,pressure,gps_valid,press_valid,rssi,"
               "accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,mag_x,mag_y,mag_z,imu_temp,imu_valid,"
               "voltage,current,power,power_valid");
  file.close();
  
  Serial.print("Created log file: ");
  Serial.println(currentLogFile);
  
  return true;
}

String SDManager::generateFileName() {
  // Generate filename based on current time
  unsigned long timestamp = millis();
  char filename[32];
  snprintf(filename, sizeof(filename), "/flight_%08lu.csv", timestamp);
  return String(filename);
}

bool SDManager::addData(const TelemetryData& data) {
  // Always try to add data to batch, even if cards are currently failed
  // This way when cards come back online, we don't lose the most recent data
  
  // Add data to current batch
  if (currentBatch.count < SD_BATCH_SIZE) {
    currentBatch.data[currentBatch.count] = data;
    currentBatch.count++;
    
    // If we have a working card and batch is full, write it
    if (sdInitialized && activeCard != SD_NONE && currentBatch.count >= SD_BATCH_SIZE) {
      if (!flushCurrentBatch()) {
        // Handle card failure
        handleCardFailure();
        // Even if card failed, we still successfully added data to batch
        return true;
      } else {
        consecutiveFailures = 0; // Reset failure count on success
      }
    }
    
    return true;
  }
  
  // Batch is full but we can't write it - this shouldn't normally happen
  // but if it does, try to flush and then add the new data
  if (sdInitialized && activeCard != SD_NONE) {
    flushCurrentBatch();
  } else {
    // No working cards - overwrite oldest data in batch to keep most recent
    Serial.println("SD: No working cards, overwriting oldest data in batch");
    memmove(&currentBatch.data[0], &currentBatch.data[1], 
           (SD_BATCH_SIZE - 1) * sizeof(TelemetryData));
    currentBatch.data[SD_BATCH_SIZE - 1] = data;
    return true;
  }
  
  // Try to add the data again after flush
  if (currentBatch.count < SD_BATCH_SIZE) {
    currentBatch.data[currentBatch.count] = data;
    currentBatch.count++;
    return true;
  }
  
  return false;
}

bool SDManager::flushCurrentBatch() {
  if (!sdInitialized || activeCard == SD_NONE || currentBatch.count == 0) {
    return false;
  }
  
  bool success = writeBatchToFile(currentBatch);
  
  if (success) {
    totalBatchesStored++;
    consecutiveFailures = 0; // Reset failure count on success
    Serial.print("Batch ");
    Serial.print(totalBatchesStored);
    Serial.print(" written to ");
    Serial.print(getCardSlotName(activeCard));
    Serial.print(" card (");
    Serial.print(currentBatch.count);
    Serial.println(" records)");
  } else {
    consecutiveFailures++;
    Serial.print("Failed to write batch to ");
    Serial.print(getCardSlotName(activeCard));
    Serial.print(" card. Failure #");
    Serial.println(consecutiveFailures);
  }
  
  // Reset batch for next data
  memset(&currentBatch, 0, sizeof(DataBatch));
  currentBatch.batchStartTime = millis();
  
  return success;
}

bool SDManager::writeBatchToFile(const DataBatch& batch) {
  File file = SD.open(currentLogFile, FILE_APPEND);
  if (!file) {
    Serial.print("Failed to open log file for writing: ");
    Serial.println(currentLogFile);
    return false;
  }
  
  // Write all data in the batch
  for (int i = 0; i < batch.count; i++) {
    String dataLine = formatTelemetryData(batch.data[i]);
    file.println(dataLine);
  }
  
  file.close();
  return true;
}

String SDManager::formatTelemetryData(const TelemetryData& data) {
  char buffer[512];
  
  snprintf(buffer, sizeof(buffer),
    "%lu,%d,%.6f,%.6f,%.2f,%.2f,%.2f,%d,%d,%d,"
    "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.2f,%d,"
    "%.3f,%.2f,%.2f,%d",
    data.timestamp, data.mode,
    data.latitude, data.longitude, data.altitude_gps, data.altitude_pressure, data.pressure,
    data.gps_valid, data.pressure_valid, data.rssi,
    data.accel_x, data.accel_y, data.accel_z,
    data.gyro_x, data.gyro_y, data.gyro_z,
    data.mag_x, data.mag_y, data.mag_z, data.imu_temperature, data.imu_valid,
    data.bus_voltage, data.current, data.power, data.power_valid
  );
  
  return String(buffer);
}

bool SDManager::forceSync() {
  if (!sdInitialized || activeCard == SD_NONE) {
    return false;
  }
  
  // Flush current batch even if not full
  if (currentBatch.count > 0) {
    return flushCurrentBatch();
  }
  
  return true;
}

bool SDManager::listLogFiles() {
  if (!sdInitialized || activeCard == SD_NONE) {
    return false;
  }
  
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory");
    return false;
  }
  
  Serial.print("Log files on ");
  Serial.print(getCardSlotName(activeCard));
  Serial.println(" SD card:");
  File file = root.openNextFile();
  int fileCount = 0;
  
  while (file) {
    if (!file.isDirectory() && String(file.name()).endsWith(".csv")) {
      Serial.print("  ");
      Serial.print(file.name());
      Serial.print(" (");
      Serial.print(file.size());
      Serial.println(" bytes)");
      fileCount++;
    }
    file.close();
    file = root.openNextFile();
  }
  
  root.close();
  Serial.print("Total log files: ");
  Serial.println(fileCount);
  
  return true;
}

bool SDManager::deleteOldFiles(int maxFiles) {
  if (!sdInitialized || activeCard == SD_NONE) {
    return false;
  }
  
  // This is a simplified implementation
  // In a production system, you'd want to sort by date and delete oldest files
  File root = SD.open("/");
  if (!root) {
    return false;
  }
  
  int fileCount = 0;
  File file = root.openNextFile();
  
  // Count CSV files first
  while (file) {
    if (!file.isDirectory() && String(file.name()).endsWith(".csv")) {
      fileCount++;
    }
    file.close();
    file = root.openNextFile();
  }
  
  root.close();
  
  if (fileCount > maxFiles) {
    Serial.print("Too many log files on ");
    Serial.print(getCardSlotName(activeCard));
    Serial.print(" card (");
    Serial.print(fileCount);
    Serial.println("), cleanup needed but not implemented in this simple version");
  }
  
  return true;
}

size_t SDManager::getAvailableSpace() const {
  if (!sdInitialized || activeCard == SD_NONE) {
    return 0;
  }
  
  return SD.totalBytes() - SD.usedBytes();
}

size_t SDManager::getUsedSpace() const {
  if (!sdInitialized || activeCard == SD_NONE) {
    return 0;
  }
  
  return SD.usedBytes();
}

String SDManager::getCardSlotName(SDCardSlot slot) const {
  switch (slot) {
    case SD_PRIMARY: return "Primary";
    case SD_BACKUP: return "Backup";
    case SD_NONE: return "None";
    default: return "Unknown";
  }
}

bool SDManager::testCardHealth(SDCardSlot slot) {
  int csPin = (slot == SD_PRIMARY) ? SD_CS_PIN : SD_CS_BACKUP_PIN;
  
  // Remember the current active card so we can restore it
  SDCardSlot originalActiveCard = activeCard;
  int originalCsPin = (originalActiveCard == SD_PRIMARY) ? SD_CS_PIN : SD_CS_BACKUP_PIN;
  
  // Test the specific slot
  Serial.print("Testing health of ");
  Serial.print(getCardSlotName(slot));
  Serial.println(" card...");
  
  // End current connection temporarily
  SD.end();
  delay(10); // Small delay to ensure clean disconnection
  
  // Try to initialize the card we're testing
  bool cardHealthy = false;
  if (SD.begin(csPin, SPI, SD_SPI_SPEED)) {
    // Try to open root directory as a health check
    File root = SD.open("/");
    if (root) {
      cardHealthy = true;
      root.close();
      Serial.print(getCardSlotName(slot));
      Serial.println(" card is healthy");
    } else {
      Serial.print(getCardSlotName(slot));
      Serial.println(" card failed directory test");
    }
  } else {
    // Try with slower speed
    if (SD.begin(csPin, SPI, 1000000)) {
      File root = SD.open("/");
      if (root) {
        cardHealthy = true;
        root.close();
        Serial.print(getCardSlotName(slot));
        Serial.println(" card is healthy (slow speed)");
      } else {
        Serial.print(getCardSlotName(slot));
        Serial.println(" card failed directory test (slow speed)");
      }
    } else {
      Serial.print(getCardSlotName(slot));
      Serial.println(" card is not responding");
    }
  }
  
  // Restore the original active card connection if we had one
  if (originalActiveCard != SD_NONE && sdInitialized) {
    SD.end();
    delay(10);
    
    // Restore connection to originally active card
    if (SD.begin(originalCsPin, SPI, SD_SPI_SPEED) || 
        SD.begin(originalCsPin, SPI, 1000000)) {
      Serial.print("Restored connection to ");
      Serial.print(getCardSlotName(originalActiveCard));
      Serial.println(" card");
    } else {
      Serial.print("Warning: Failed to restore connection to ");
      Serial.print(getCardSlotName(originalActiveCard));
      Serial.println(" card");
    }
  }
  
  return cardHealthy;
}

bool SDManager::performCardHealthCheck() {
  Serial.println("Performing SD card health check...");
  
  // Check both cards periodically
  bool primaryHealthy = testCardHealth(SD_PRIMARY);
  bool backupHealthy = testCardHealth(SD_BACKUP);
  
  // Update card presence status
  primaryCardPresent = primaryHealthy;
  backupCardPresent = backupHealthy;
  
  Serial.print("Health check results: Primary=");
  Serial.print(primaryHealthy ? "OK" : "FAIL");
  Serial.print(", Backup=");
  Serial.print(backupHealthy ? "OK" : "FAIL");
  Serial.print(", Active=");
  Serial.println(getCardSlotName(activeCard));
  
  // Only switch if current active card is unhealthy
  if (activeCard == SD_PRIMARY && !primaryHealthy) {
    Serial.println("Active primary card is unhealthy, attempting failover");
    return handleCardFailure();
  } else if (activeCard == SD_BACKUP && !backupHealthy) {
    Serial.println("Active backup card is unhealthy, attempting failover");
    return handleCardFailure();
  }
  
  Serial.println("Active card is healthy, no action needed");
  return true;
}

bool SDManager::handleCardFailure() {
  consecutiveFailures++;
  
  Serial.print("SD card failure detected. Consecutive failures: ");
  Serial.println(consecutiveFailures);
  
  if (consecutiveFailures >= SD_MAX_CONSECUTIVE_FAILURES) {
    // Try to switch to the other card
    if (activeCard == SD_PRIMARY && backupCardPresent) {
      if (switchToBackupCard()) {
        Serial.println("Switched to backup card due to repeated primary failures");
        consecutiveFailures = 0;
        return true;
      }
    } else if (activeCard == SD_BACKUP && primaryCardPresent) {
      if (switchToPrimaryCard()) {
        Serial.println("Switched to primary card due to repeated backup failures");
        consecutiveFailures = 0;
        return true;
      }
    }
    
    // Both cards failed or no alternative available
    Serial.println("Critical: Both SD cards appear to have failed");
    bothCardsFailed = true;
    sdInitialized = false;
    activeCard = SD_NONE;
    lastRetryAttempt = millis();
    return false;
  }
  
  // Still within failure threshold, keep trying current card
  return false;
}

bool SDManager::switchToPrimaryCard() {
  if (!primaryCardPresent && !tryInitializeCard(SD_PRIMARY)) {
    Serial.println("Primary SD card not available for switch");
    return false;
  }
  
  // Flush any pending data on current card
  if (currentBatch.count > 0) {
    flushCurrentBatch();
  }
  
  // End current SD connection
  SD.end();
  
  // Reinitialize SD library with primary card's CS pin
  if (!SD.begin(SD_CS_PIN, SPI, SD_SPI_SPEED)) {
    Serial.println("Failed to reinitialize SD library for primary card");
    // Try with slower speed
    if (!SD.begin(SD_CS_PIN, SPI, 1000000)) {
      Serial.println("Failed to reinitialize SD library for primary card even at reduced speed");
      return false;
    }
  }
  
  // Switch to primary card
  activeCard = SD_PRIMARY;
  primaryCardPresent = true;
  
  // Create new log file on primary card
  if (!createLogFile()) {
    Serial.println("Failed to create log file on primary card");
    return false;
  }
  
  Serial.println("Successfully switched to primary SD card");
  return true;
}

String SDManager::getDetailedStatus() const {
  if (bothCardsFailed) {
    char status[256];
    snprintf(status, sizeof(status),
      "SD: Both cards failed, will retry in %d seconds, P:%s B:%s",
      (int)((SD_RETRY_INTERVAL - (millis() - lastRetryAttempt)) / 1000),
      primaryCardPresent ? "OK" : "FAIL",
      backupCardPresent ? "OK" : "FAIL"
    );
    return String(status);
  }
  
  if (!sdInitialized || activeCard == SD_NONE) {
    return "SD: No cards available";
  }
  
  char status[256];
  snprintf(status, sizeof(status),
    "SD: %s card active, %d batches, %d/%d current, %dKB free, %d failures, P:%s B:%s",
    getCardSlotName(activeCard).c_str(),
    totalBatchesStored,
    currentBatch.count,
    SD_BATCH_SIZE,
    (int)(getAvailableSpace() / 1024),
    consecutiveFailures,
    primaryCardPresent ? "OK" : "FAIL",
    backupCardPresent ? "OK" : "FAIL"
  );
  
  return String(status);
}

void SDManager::update() {
  performPeriodicTasks();
}

void SDManager::performPeriodicTasks() {
  unsigned long currentTime = millis();
  
  // If both cards failed, try to reinitialize periodically
  if (bothCardsFailed) {
    unsigned long timeSinceLastRetry = currentTime - lastRetryAttempt;
    
    // Debug output every 2 seconds to show we're still trying
    static unsigned long lastDebugOutput = 0;
    if (currentTime - lastDebugOutput >= 2000) {
      Serial.print("SD: Retry check - elapsed: ");
      Serial.print(timeSinceLastRetry / 1000);
      Serial.print("s, target: ");
      Serial.print(SD_RETRY_INTERVAL / 1000);
      Serial.print("s, remaining: ");
      if (timeSinceLastRetry < SD_RETRY_INTERVAL) {
        Serial.print((SD_RETRY_INTERVAL - timeSinceLastRetry) / 1000);
        Serial.println("s");
      } else {
        Serial.println("READY");
      }
      lastDebugOutput = currentTime;
    }
    
    if (timeSinceLastRetry >= SD_RETRY_INTERVAL) {
      Serial.println("Retrying SD card initialization...");
      if (retryCardInitialization()) {
        Serial.println("SD card recovered successfully!");
        bothCardsFailed = false;
      } else {
        Serial.println("SD card retry failed, will try again later");
        lastRetryAttempt = currentTime;
      }
    }
    return;  // Skip health checks if both cards failed
  }
  
  // Perform regular health checks if we have an active card
  if (sdInitialized && activeCard != SD_NONE) {
    if (currentTime - lastCardHealthCheck >= SD_HEALTH_CHECK_INTERVAL) {
      performCardHealthCheck();
      lastCardHealthCheck = currentTime;
    }
  }
}

bool SDManager::retryCardInitialization() {
  // Reset card presence flags
  primaryCardPresent = false;
  backupCardPresent = false;
  
  // Try to initialize the SD system again
  return initializeSD();
}

String SDManager::getLogFilesList() {
  String json = "{\"files\":[";
  bool firstFile = true;
  
  if (!sdInitialized || activeCard == SD_NONE) {
    json += "],\"error\":\"SD card not available\"}";
    return json;
  }
  
  File root = SD.open("/");
  if (!root) {
    json += "],\"error\":\"Failed to open root directory\"}";
    return json;
  }
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory() && String(file.name()).endsWith(".csv")) {
      if (!firstFile) {
        json += ",";
      }
      json += "{";
      json += "\"name\":\"" + String(file.name()) + "\",";
      json += "\"size\":" + String(file.size());
      json += "}";
      firstFile = false;
    }
    file.close();
    file = root.openNextFile();
  }
  
  root.close();
  json += "],\"active_card\":\"" + getCardSlotName(activeCard) + "\"}";
  return json;
}

bool SDManager::readLogFile(const String& filename, String& content) {
  if (!sdInitialized || activeCard == SD_NONE) {
    return false;
  }
  
  // Ensure filename starts with "/"
  String fullPath = filename.startsWith("/") ? filename : "/" + filename;
  
  File file = SD.open(fullPath, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file: " + fullPath);
    return false;
  }
  
  content = "";
  while (file.available()) {
    content += file.readString();
  }
  file.close();
  
  return true;
}

bool SDManager::fileExists(const String& filename) {
  if (!sdInitialized || activeCard == SD_NONE) {
    return false;
  }
  
  // Ensure filename starts with "/"
  String fullPath = filename.startsWith("/") ? filename : "/" + filename;
  
  return SD.exists(fullPath);
}

size_t SDManager::getFileSize(const String& filename) {
  if (!sdInitialized || activeCard == SD_NONE) {
    return 0;
  }
  
  // Ensure filename starts with "/"
  String fullPath = filename.startsWith("/") ? filename : "/" + filename;
  
  File file = SD.open(fullPath, FILE_READ);
  if (!file) {
    return 0;
  }
  
  size_t size = file.size();
  file.close();
  return size;
}
