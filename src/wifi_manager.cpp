#include "wifi_manager.h"
#include "web_content.h"

WiFiManager::WiFiManager() : 
  webServer(nullptr),
  initialized(false),
  connected(false),
  serverRunning(false) {
  // Initialize with default telemetry data
  latestData = {0.0, 0.0, 0.0, 0.0, 1013.25, 0, MODE_MAINTENANCE, false, false, -999};
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

void WiFiManager::broadcastData(const TelemetryData& data) {
  if (!connected || !serverRunning) return;
  
  // Store latest data for web requests
  latestData = data;
  
  // Handle web server clients
  webServer->handleClient();
}

void WiFiManager::handleClient() {
  if (webServer && serverRunning) {
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
  json += "\"rssi\":" + String(data.rssi);
  json += "}";
  
  return json;
}
