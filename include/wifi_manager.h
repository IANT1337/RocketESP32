#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"
#include "web_content.h"

// Forward declaration to avoid circular dependency
class SystemController;

class WiFiManager {
private:
  WebServer* webServer;
  bool initialized;
  bool connected;
  bool serverRunning;
  TelemetryData latestData;
  SystemController* systemController;  // Reference to system controller for SD card status
  
  void handleRoot();
  void handleTelemetry();
  void handleSDStatus();
  void handleNotFound();
  void handleStyle();
  void handleScript();
  void handleLogsList();
  void handleDownloadFile();
  void handleDownloadAll();
  String createTelemetryJSON(const TelemetryData& data);

public:
  WiFiManager();
  ~WiFiManager();
  
  void initialize();
  bool connect(const char* ssid, const char* password);
  void disconnect();
  void powerOff();
  void powerOn();
  void broadcastData(const TelemetryData& data);
  void handleClient();
  bool isConnected() const { return connected; }
  bool isValid() const { return initialized; }
  void setSystemController(SystemController* controller) { systemController = controller; }
};

#endif
