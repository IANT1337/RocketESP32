#include "wifi_manager.h"
#include "web_content.h"
#include "system_controller.h"
#include "esp_wifi.h"
#include "esp_sleep.h"

WiFiManager::WiFiManager() : 
  webServer(nullptr),
  initialized(false),
  connected(false),
  serverRunning(false),
  systemController(nullptr) {
  // Initialize with default telemetry data
  latestData = {0.0, 0.0, 0.0, 0.0, 1013.25, 0, MODE_MAINTENANCE, false, false, -999,
               0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, false,
               0.0, 0.0, 0.0, false};
}

WiFiManager::~WiFiManager() {
  if (webServer) {
    delete webServer;
  }
}

void WiFiManager::initialize() {
  Serial.println("Initializing WiFi manager...");
  
  // Set WiFi mode to station
  WiFi.mode(WIFI_STA);
  
  initialized = true;
  Serial.println("WiFi manager initialized");
}

bool WiFiManager::connect(const char* ssid, const char* password) {
  if (!initialized) return false;
  
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Wait for connection (up to 10 seconds)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    connected = true;
    Serial.println();
    Serial.print("WiFi connected! IP address: ");
    Serial.println(WiFi.localIP());
    
    // Start web server
    if (!webServer) {
      webServer = new WebServer(WEBSERVER_PORT);
    }
    
    // Setup routes
    webServer->on("/", [this]() { handleRoot(); });
    webServer->on("/telemetry", [this]() { handleTelemetry(); });
    webServer->on("/sdstatus", [this]() { handleSDStatus(); });
    webServer->on("/style.css", [this]() { handleStyle(); });
    webServer->on("/script.js", [this]() { handleScript(); });
    webServer->onNotFound([this]() { handleNotFound(); });
    
    webServer->begin();
    serverRunning = true;
    
    Serial.print("Web server started on port ");
    Serial.println(WEBSERVER_PORT);
    
    return true;
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi");
    connected = false;
    return false;
  }
}

void WiFiManager::disconnect() {
  if (connected) {
    Serial.println("Disconnecting from WiFi");
    
    if (webServer && serverRunning) {
      webServer->stop();
      serverRunning = false;
    }
    
    WiFi.disconnect();
    connected = false;
  }
}

void WiFiManager::powerOff() {
  Serial.println("Turning off WiFi for power saving");
  
  // Disconnect first if connected
  disconnect();
  
  // Turn off WiFi radio completely
  WiFi.mode(WIFI_OFF);
  
  // Additional power saving for ESP32-S3 / Arduino Nano ESP32
  if (initialized) {
    // Safely stop and deinitialize WiFi
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_OK) {
      esp_wifi_deinit();
    }
  }
  
  // Configure power domain for maximum power savings
  // Note: ESP32-S3 power management may differ slightly
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  
  initialized = false;
  Serial.println("WiFi powered off");
}

void WiFiManager::powerOn() {
  Serial.println("Powering on WiFi");
  
  // Re-initialize WiFi
  initialize();
  
  Serial.println("WiFi powered on and ready");
}

void WiFiManager::broadcastData(const TelemetryData& data) {
  // Only handle data if WiFi is initialized and connected
  if (!initialized || !connected || !serverRunning) return;
  
  // Store latest data for web requests
  latestData = data;
  
  // Handle web server clients
  webServer->handleClient();
}

void WiFiManager::handleClient() {
  // Only handle clients if WiFi is properly initialized and server is running
  if (webServer && serverRunning && initialized) {
    webServer->handleClient();
  }
}

void WiFiManager::handleRoot() {
  webServer->send_P(200, "text/html", WebContent::getIndexHTML());
}

void WiFiManager::handleTelemetry() {
  // Return the latest telemetry data
  String json = createTelemetryJSON(latestData);
  webServer->send(200, "application/json", json);
}

void WiFiManager::handleSDStatus() {
  String json = "{";
  
  if (systemController) {
    json += "\"available\":" + String(systemController->isSDCardAvailable() ? "true" : "false") + ",";
    json += "\"status\":\"" + systemController->getSDCardStatus() + "\"";
  } else {
    json += "\"available\":false,";
    json += "\"status\":\"System controller not available\"";
  }
  
  json += "}";
  webServer->send(200, "application/json", json);
}

void WiFiManager::handleNotFound() {
  webServer->send(404, "text/plain", "Not Found");
}

void WiFiManager::handleStyle() {
  webServer->send_P(200, "text/css", WebContent::getStyleCSS());
}

void WiFiManager::handleScript() {
  webServer->send_P(200, "application/javascript", WebContent::getScriptJS());
}

String WiFiManager::createTelemetryJSON(const TelemetryData& data) {
  String json = "{";
  json += "\"timestamp\":" + String(data.timestamp) + ",";
  json += "\"mode\":" + String(data.mode) + ",";
  json += "\"latitude\":" + String(data.latitude, 6) + ",";
  json += "\"longitude\":" + String(data.longitude, 6) + ",";
  json += "\"altitude_gps\":" + String(data.altitude_gps, 2) + ",";
  json += "\"altitude_pressure\":" + String(data.altitude_pressure, 2) + ",";
  json += "\"pressure\":" + String(data.pressure, 2) + ",";
  json += "\"gps_valid\":" + String(data.gps_valid ? "true" : "false") + ",";
  json += "\"pressure_valid\":" + String(data.pressure_valid ? "true" : "false") + ",";
  
  // Add IMU data
  json += "\"accel_x\":" + String(data.accel_x, 3) + ",";
  json += "\"accel_y\":" + String(data.accel_y, 3) + ",";
  json += "\"accel_z\":" + String(data.accel_z, 3) + ",";
  json += "\"gyro_x\":" + String(data.gyro_x, 2) + ",";
  json += "\"gyro_y\":" + String(data.gyro_y, 2) + ",";
  json += "\"gyro_z\":" + String(data.gyro_z, 2) + ",";
  json += "\"mag_x\":" + String(data.mag_x, 1) + ",";
  json += "\"mag_y\":" + String(data.mag_y, 1) + ",";
  json += "\"mag_z\":" + String(data.mag_z, 1) + ",";
  json += "\"imu_temperature\":" + String(data.imu_temperature, 1) + ",";
  json += "\"imu_valid\":" + String(data.imu_valid ? "true" : "false") + ",";
  
  // Add power sensor data
  json += "\"bus_voltage\":" + String(data.bus_voltage, 3) + ",";
  json += "\"current\":" + String(data.current, 2) + ",";
  json += "\"power\":" + String(data.power, 2) + ",";
  json += "\"power_valid\":" + String(data.power_valid ? "true" : "false") + ",";
  
  json += "\"rssi\":" + String(data.rssi);
  json += "}";
  
  return json;
}
