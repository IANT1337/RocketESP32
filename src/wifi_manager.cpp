#include "wifi_manager.h"

WiFiManager::WiFiManager() : 
  webServer(nullptr),
  initialized(false),
  connected(false),
  serverRunning(false) {
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
  
  // Handle web server clients
  webServer->handleClient();
  
  // Store latest data for web requests
  // In a real implementation, you might want to store this in a member variable
  // or send it to an external server
}

void WiFiManager::handleClient() {
  if (webServer && serverRunning) {
    webServer->handleClient();
  }
}

void WiFiManager::handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>Rocket Flight Computer</title>";
  html += "<meta charset=\"utf-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; }";
  html += ".container { max-width: 800px; margin: 0 auto; }";
  html += ".data-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }";
  html += ".data-card { border: 1px solid #ddd; padding: 15px; border-radius: 5px; }";
  html += ".data-value { font-size: 1.5em; font-weight: bold; color: #2196F3; }";
  html += ".refresh-btn { background: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; }";
  html += "</style>";
  html += "<script>";
  html += "function refreshData() {";
  html += "fetch('/telemetry')";
  html += ".then(response => response.json())";
  html += ".then(data => {";
  html += "document.getElementById('timestamp').textContent = new Date(data.timestamp).toLocaleString();";
  html += "document.getElementById('mode').textContent = data.mode;";
  html += "document.getElementById('latitude').textContent = data.latitude.toFixed(6);";
  html += "document.getElementById('longitude').textContent = data.longitude.toFixed(6);";
  html += "document.getElementById('altitude_gps').textContent = data.altitude_gps.toFixed(2) + ' m';";
  html += "document.getElementById('altitude_pressure').textContent = data.altitude_pressure.toFixed(2) + ' m';";
  html += "document.getElementById('pressure').textContent = data.pressure.toFixed(2) + ' hPa';";
  html += "document.getElementById('gps_status').textContent = data.gps_valid ? 'Valid' : 'Invalid';";
  html += "document.getElementById('pressure_status').textContent = data.pressure_valid ? 'Valid' : 'Invalid';";
  html += "})";
  html += ".catch(error => console.error('Error:', error));";
  html += "}";
  html += "setInterval(refreshData, 2000);";
  html += "window.onload = refreshData;";
  html += "</script>";
  html += "</head>";
  html += "<body>";
  html += "<div class=\"container\">";
  html += "<h1>Rocket Flight Computer - Maintenance Mode</h1>";
  html += "<button class=\"refresh-btn\" onclick=\"refreshData()\">Refresh Data</button>";
  html += "<div class=\"data-grid\">";
  html += "<div class=\"data-card\">";
  html += "<h3>System Status</h3>";
  html += "<p>Last Update: <span id=\"timestamp\" class=\"data-value\">--</span></p>";
  html += "<p>Mode: <span id=\"mode\" class=\"data-value\">--</span></p>";
  html += "</div>";
  html += "<div class=\"data-card\">";
  html += "<h3>GPS Data</h3>";
  html += "<p>Latitude: <span id=\"latitude\" class=\"data-value\">--</span></p>";
  html += "<p>Longitude: <span id=\"longitude\" class=\"data-value\">--</span></p>";
  html += "<p>Altitude: <span id=\"altitude_gps\" class=\"data-value\">--</span></p>";
  html += "<p>Status: <span id=\"gps_status\" class=\"data-value\">--</span></p>";
  html += "</div>";
  html += "<div class=\"data-card\">";
  html += "<h3>Pressure Data</h3>";
  html += "<p>Pressure: <span id=\"pressure\" class=\"data-value\">--</span></p>";
  html += "<p>Altitude: <span id=\"altitude_pressure\" class=\"data-value\">--</span></p>";
  html += "<p>Status: <span id=\"pressure_status\" class=\"data-value\">--</span></p>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "</body>";
  html += "</html>";
  
  webServer->send(200, "text/html", html);
}

void WiFiManager::handleTelemetry() {
  // In a real implementation, you would return the latest telemetry data
  // For now, return a placeholder JSON structure
  String json = "{";
  json += "\"timestamp\": 0,";
  json += "\"mode\": 2,";
  json += "\"latitude\": 0.0,";
  json += "\"longitude\": 0.0,";
  json += "\"altitude_gps\": 0.0,";
  json += "\"altitude_pressure\": 0.0,";
  json += "\"pressure\": 1013.25,";
  json += "\"gps_valid\": false,";
  json += "\"pressure_valid\": false";
  json += "}";
  
  webServer->send(200, "application/json", json);
}

void WiFiManager::handleNotFound() {
  webServer->send(404, "text/plain", "Not Found");
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
  json += "\"pressure_valid\":" + String(data.pressure_valid ? "true" : "false");
  json += "}";
  
  return json;
}
