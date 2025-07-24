#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"

class WiFiManager {
private:
  WebServer* webServer;
  bool initialized;
  bool connected;
  bool serverRunning;
  
  void handleRoot();
  void handleTelemetry();
  void handleNotFound();
  String createTelemetryJSON(const TelemetryData& data);

public:
  WiFiManager();
  ~WiFiManager();
  
  void initialize();
  bool connect(const char* ssid, const char* password);
  void disconnect();
  void broadcastData(const TelemetryData& data);
  void handleClient();
  bool isConnected() const { return connected; }
  bool isValid() const { return initialized; }
};

#endif
