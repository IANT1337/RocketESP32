#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "config.h"

// Data structure for batch storage
struct DataBatch {
  TelemetryData data[SD_BATCH_SIZE];
  int count;
  unsigned long batchStartTime;
};

enum SDCardSlot {
  SD_PRIMARY = 0,
  SD_BACKUP = 1,
  SD_NONE = -1
};

class SDManager {
private:
  bool sdInitialized;
  bool primaryCardPresent;
  bool backupCardPresent;
  SDCardSlot activeCard;
  DataBatch currentBatch;
  int totalBatchesStored;
  String currentLogFile;
  unsigned long lastCardHealthCheck;
  unsigned long lastRetryAttempt;
  int consecutiveFailures;
  bool bothCardsFailed;
  
  bool initializeSD();
  bool tryInitializeCard(SDCardSlot slot);
  bool switchToBackupCard();
  bool switchToPrimaryCard();
  bool testCardHealth(SDCardSlot slot);
  bool performCardHealthCheck();
  bool handleCardFailure();
  bool retryCardInitialization();
  void performPeriodicTasks();
  bool createLogFile();
  String generateFileName();
  bool writeBatchToFile(const DataBatch& batch);
  bool writeHeader();
  String formatTelemetryData(const TelemetryData& data);
  String getCardSlotName(SDCardSlot slot) const;

public:
  SDManager();
  ~SDManager();
  
  bool initialize();
  bool isInitialized() const { return sdInitialized; }
  bool isCardPresent() const { return primaryCardPresent || backupCardPresent; }
  SDCardSlot getActiveCard() const { return activeCard; }
  bool isPrimaryCardActive() const { return activeCard == SD_PRIMARY; }
  bool isBackupCardActive() const { return activeCard == SD_BACKUP; }
  
  // Data storage methods
  bool addData(const TelemetryData& data);
  bool flushCurrentBatch();
  bool forceSync();
  void update();  // Call this regularly to perform health checks and retries
  
  // File management methods
  bool listLogFiles();
  bool deleteOldFiles(int maxFiles = 10);
  String getCurrentLogFile() const { return currentLogFile; }
  int getTotalBatchesStored() const { return totalBatchesStored; }
  
  // Statistics
  size_t getAvailableSpace() const;
  size_t getUsedSpace() const;
  int getCurrentBatchSize() const { return currentBatch.count; }
  int getConsecutiveFailures() const { return consecutiveFailures; }
  String getDetailedStatus() const;
};

#endif
