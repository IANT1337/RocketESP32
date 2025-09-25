#include "web_content.h"

// Store HTML content in PROGMEM to save RAM
const char WebContent::index_html[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Rocket Flight Computer</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="container">
        <h1>Rocket Flight Computer - Maintenance Mode</h1>
        <button class="refresh-btn" onclick="refreshData()">Refresh Data</button>
        
        <div class="data-grid">
            <div class="data-card">
                <h3>System Status</h3>
                <p>Last Update: <span id="timestamp" class="data-value">--</span></p>
                <p>Mode: <span id="mode" class="data-value">--</span></p>
            </div>
            
            <div class="data-card">
                <h3>GPS Data</h3>
                <p>Latitude: <span id="latitude" class="data-value">--</span></p>
                <p>Longitude: <span id="longitude" class="data-value">--</span></p>
                <p>Altitude: <span id="altitude_gps" class="data-value">--</span></p>
                <p>Status: <span id="gps_status" class="data-value">--</span></p>
            </div>
            
            <div class="data-card">
                <h3>Pressure Data</h3>
                <p>Pressure: <span id="pressure" class="data-value">--</span></p>
                <p>Altitude: <span id="altitude_pressure" class="data-value">--</span></p>
                <p>Status: <span id="pressure_status" class="data-value">--</span></p>
            </div>
            
            <div class="data-card">
                <h3>Radio Data</h3>
                <p>RSSI: <span id="rssi" class="data-value">--</span></p>
                <p>Signal Quality: <span id="signal_quality" class="data-value">--</span></p>
            </div>
            
            <div class="data-card flight-logs-card">
                <h3>Flight Logs</h3>
                <button class="download-btn" onclick="refreshLogsList()">Refresh Logs</button>
                <button class="download-btn download-all-btn" onclick="downloadAllLogs()">Download All</button>
                <div id="logs-list" class="logs-container">
                    <p class="logs-status">Click "Refresh Logs" to view available files</p>
                </div>
            </div>
        </div>
    </div>
    
    <script src="/script.js"></script>
</body>
</html>
)HTML";

const char WebContent::style_css[] PROGMEM = R"CSS(
body {
    font-family: Arial, sans-serif;
    margin: 20px;
    background-color: #f5f5f5;
}

.container {
    max-width: 800px;
    margin: 0 auto;
    background-color: white;
    padding: 20px;
    border-radius: 10px;
    box-shadow: 0 2px 10px rgba(0,0,0,0.1);
}

.data-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 20px;
    margin-top: 20px;
}

.data-card {
    border: 1px solid #ddd;
    padding: 15px;
    border-radius: 5px;
    background-color: #fafafa;
}

.flight-logs-card {
    grid-column: 1 / -1;  /* Span full width */
}

.data-card h3 {
    margin-top: 0;
    color: #333;
}

.data-value {
    font-size: 1.5em;
    font-weight: bold;
    color: #2196F3;
}

.refresh-btn, .download-btn {
    background: #4CAF50;
    color: white;
    padding: 10px 20px;
    border: none;
    border-radius: 5px;
    cursor: pointer;
    font-size: 16px;
    transition: background-color 0.3s;
    margin-right: 10px;
    margin-bottom: 10px;
}

.download-all-btn {
    background: #2196F3;
}

.refresh-btn:hover, .download-btn:hover {
    background: #45a049;
}

.download-all-btn:hover {
    background: #1976D2;
}

.logs-container {
    margin-top: 15px;
    max-height: 200px;
    overflow-y: auto;
    border: 1px solid #ddd;
    border-radius: 3px;
    padding: 10px;
    background: white;
}

.log-file {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 8px;
    border-bottom: 1px solid #eee;
}

.log-file:last-child {
    border-bottom: none;
}

.log-info {
    flex-grow: 1;
}

.log-name {
    font-weight: bold;
    color: #333;
}

.log-size {
    font-size: 0.9em;
    color: #666;
}

.log-download {
    background: #2196F3;
    color: white;
    padding: 5px 10px;
    border: none;
    border-radius: 3px;
    cursor: pointer;
    font-size: 14px;
}

.log-download:hover {
    background: #1976D2;
}

.logs-status {
    text-align: center;
    color: #666;
    font-style: italic;
}

.logs-status.error {
    color: #d32f2f;
}

@media (max-width: 600px) {
    .data-grid {
        grid-template-columns: 1fr;
    }
}
)CSS";

const char WebContent::script_js[] PROGMEM = R"JS(
function refreshData() {
    fetch('/telemetry')
        .then(response => response.json())
        .then(data => {
            document.getElementById('timestamp').textContent = new Date(data.timestamp).toLocaleString();
            document.getElementById('mode').textContent = getModeString(data.mode);
            document.getElementById('latitude').textContent = data.latitude.toFixed(6);
            document.getElementById('longitude').textContent = data.longitude.toFixed(6);
            document.getElementById('altitude_gps').textContent = data.altitude_gps.toFixed(2) + ' m';
            document.getElementById('altitude_pressure').textContent = data.altitude_pressure.toFixed(2) + ' m';
            document.getElementById('pressure').textContent = data.pressure.toFixed(2) + ' hPa';
            document.getElementById('gps_status').textContent = data.gps_valid ? 'Valid' : 'Invalid';
            document.getElementById('pressure_status').textContent = data.pressure_valid ? 'Valid' : 'Invalid';
            document.getElementById('rssi').textContent = data.rssi + ' dBm';
            document.getElementById('signal_quality').textContent = getSignalQuality(data.rssi);
        })
        .catch(error => {
            console.error('Error:', error);
            document.getElementById('timestamp').textContent = 'Connection Error';
        });
}

function refreshLogsList() {
    fetch('/logs')
        .then(response => response.json())
        .then(data => {
            const logsContainer = document.getElementById('logs-list');
            
            if (data.error) {
                logsContainer.innerHTML = '<p class="logs-status error">' + data.error + '</p>';
                return;
            }
            
            if (!data.files || data.files.length === 0) {
                logsContainer.innerHTML = '<p class="logs-status">No log files found</p>';
                return;
            }
            
            let html = '';
            data.files.forEach(file => {
                const sizeKB = (file.size / 1024).toFixed(1);
                html += '<div class="log-file">';
                html += '<div class="log-info">';
                html += '<div class="log-name">' + file.name + '</div>';
                html += '<div class="log-size">' + sizeKB + ' KB</div>';
                html += '</div>';
                html += '<button class="log-download" onclick="downloadFile(\'' + file.name + '\')">Download</button>';
                html += '</div>';
            });
            
            if (data.active_card) {
                html += '<p style="margin-top: 15px; font-size: 0.9em; color: #666;">Active card: ' + data.active_card + '</p>';
            }
            
            logsContainer.innerHTML = html;
        })
        .catch(error => {
            console.error('Error:', error);
            document.getElementById('logs-list').innerHTML = '<p class="logs-status error">Failed to load logs</p>';
        });
}

function downloadFile(filename) {
    const url = '/download?file=' + encodeURIComponent(filename);
    window.open(url, '_blank');
}

function downloadAllLogs() {
    const url = '/download/all';
    window.open(url, '_blank');
}

function getModeString(mode) {
    switch(mode) {
        case 0: return 'Sleep';
        case 1: return 'Flight';
        case 2: return 'Maintenance';
        default: return 'Unknown';
    }
}

function getSignalQuality(rssi) {
    if (rssi >= -50) return 'Excellent';
    else if (rssi >= -60) return 'Very Good';
    else if (rssi >= -70) return 'Good';
    else if (rssi >= -80) return 'Fair';
    else if (rssi >= -90) return 'Poor';
    else return 'Very Poor';
}

setInterval(refreshData, 2000);
window.onload = refreshData;
)JS";

const char* WebContent::getIndexHTML() {
    return index_html;
}

const char* WebContent::getStyleCSS() {
    return style_css;
}

const char* WebContent::getScriptJS() {
    return script_js;
}

const char* WebContent::getContentType(const String& path) {
    if (path.endsWith(".html")) return "text/html";
    else if (path.endsWith(".css")) return "text/css";
    else if (path.endsWith(".js")) return "application/javascript";
    else if (path.endsWith(".json")) return "application/json";
    else return "text/plain";
}
