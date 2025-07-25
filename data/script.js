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
        })
        .catch(error => {
            console.error('Error:', error);
            // Update UI to show error state
            document.getElementById('timestamp').textContent = 'Connection Error';
        });
}

function getModeString(mode) {
    switch(mode) {
        case 0: return 'Sleep';
        case 1: return 'Flight';
        case 2: return 'Maintenance';
        default: return 'Unknown';
    }
}

// Auto-refresh every 2 seconds
setInterval(refreshData, 2000);

// Initial load
window.onload = refreshData;
