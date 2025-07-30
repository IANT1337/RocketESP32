# Rocket Flight Computer ESP32

A comprehensive flight computer software for ESP32 Feather V2 that interfaces with GPS, pressure sensors, IMU, power monitoring, and radio communication for advanced rocketry applications.

<img width="345" height="970" alt="new" src="https://github.com/user-attachments/assets/1280123c-ef81-465a-9f24-02427c6436ea" />

## Features

- **Three Operating Modes:**
  - **Sleep Mode**: Ultra-low power consumption, WiFi off, sensors off, periodic radio listening
  - **Flight Mode**: High power radio, active sensor reading and telemetry transmission, WiFi off for power savings
  - **Maintenance Mode**: WiFi connectivity for web-based monitoring and configuration, low power radio

- **Comprehensive Sensor Integration:**
  - GoouuuTech GT-U7 GPS module (UART communication) - 1Hz update rate
  - MPRLS pressure sensor (I2C communication) - 10Hz update rate
  - MPU9250 9-axis IMU (accelerometer, gyroscope, magnetometer) - 10Hz update rate
  - INA260 power sensor (voltage, current, power monitoring) - 10Hz update rate
  - RFD900x radio modem with RSSI monitoring (UART communication)

- **Advanced Power Management:**
  - Controllable sensor power switching (shared between pressure and IMU sensors)
  - Dynamic radio power level adjustment (high/low power modes with retry logic)
  - WiFi power management (completely off in sleep/flight modes)
  - Real-time power consumption monitoring

- **Robust Communication:**
  - Serial radio telemetry transmission with comprehensive sensor data
  - Remote mode switching via radio commands with acknowledgments
  - WiFi web server with real-time data visualization
  - RSSI monitoring with 10-second caching to minimize impact on data rate

## Hardware Connections

### ESP32 Feather V2 Pin Assignments

| Component | ESP32 Pin | Function |
|-----------|-----------|----------|
| GPS RX | 16 | GPS Serial Receive |
| GPS TX | 17 | GPS Serial Transmit |
| Radio RX | 33 | Radio Serial Receive |
| Radio TX | 32 | Radio Serial Transmit |
| Sensors SDA | 23 | I2C Data (Pressure, IMU, Power) |
| Sensors SCL | 22 | I2C Clock (Pressure, IMU, Power) |
| Sensor Power | 27 | Pressure & IMU Power Control |
| Radio Power | 14 | Radio Power Control |
| Status LED | 13 | System Status Indicator |

### Sensor Addresses

| Sensor | I2C Address | Notes |
|--------|-------------|-------|
| MPRLS Pressure | 0x18 | Primary pressure sensor |
| MPU9250 IMU | 0x68 | 9-axis inertial measurement |
| AK8963 Magnetometer | 0x0C | Integrated with MPU9250 |
| INA260 Power Monitor | 0x40 | Voltage/current/power sensing |

## Software Architecture

The application uses a robust modular design with comprehensive error handling:

### Core Modules

- **SystemController**: Main state machine, sensor coordination, and mode management
- **GPSModule**: NMEA parsing, GPS data acquisition with retry logic
- **PressureSensor**: MPRLS sensor interface, altitude calculation with retry logic
- **MPU9250Sensor**: 9-axis IMU data acquisition with graceful magnetometer fallback
- **INA260Sensor**: Power monitoring (voltage, current, power) with retry logic
- **RadioModule**: RFD900x communication, AT command handling, RSSI monitoring
- **PowerManager**: Hardware power control and management
- **WiFiManager**: Web server, wireless connectivity, and power management
- **WebContent**: Separated HTML/CSS/JavaScript content stored in PROGMEM

### Enhanced Configuration

All settings centralized in `include/config.h`:
- Pin assignments and hardware configuration
- Communication parameters and timing intervals
- Power management settings
- WiFi credentials and web server configuration
- Sensor update rates and retry parameters

## Building and Deployment

### Prerequisites

- PlatformIO IDE or CLI
- ESP32 development environment
- Adafruit ESP32 Feather V2 board
- Required libraries (automatically managed by PlatformIO)

### Build Instructions

1. Clone or download the project
2. Open in PlatformIO
3. Build and upload:
   ```bash
   pio run --target upload
   ```

### Serial Monitor

Monitor system output at 115200 baud:
```bash
pio device monitor
```

## Operation

### Mode Switching

The system responds to radio commands:
- `FLIGHT` - Enter flight mode (high power radio, WiFi off, all sensors active)
- `SLEEP` - Enter sleep mode (low power radio, WiFi off, sensors off)
- `MAINT` - Enter maintenance mode (low power radio, WiFi on, web interface active)

### Enhanced Telemetry Format

Radio telemetry packets include comprehensive sensor data (CSV format):
```
TELEM,timestamp,mode,latitude,longitude,altitude_gps,altitude_pressure,pressure,gps_valid,pressure_valid,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,mag_x,mag_y,mag_z,imu_temperature,imu_valid,bus_voltage,current,power,power_valid,rssi
```

### Maintenance Mode Web Interface

When in maintenance mode:
1. System connects to configured WiFi network
2. Web server starts on port 80 with modern responsive interface
3. Real-time telemetry display with 2-second updates
4. Access via ESP32's IP address for live monitoring
5. Automatic timeout after 5 minutes to conserve power

**Web Interface Features:**
- Live sensor data display
- GPS coordinates and altitude
- Pressure readings and calculated altitude
- IMU data (acceleration, rotation, magnetic field)
- Power consumption monitoring
- Radio signal quality (RSSI and signal strength indicator)
- System status and mode information

## Advanced Features

### Power Optimization
- **GPS**: Read only once per second to reduce power consumption
- **RSSI**: Cached for 10 seconds to minimize AT command mode impact
- **WiFi**: Completely disabled in sleep and flight modes
- **Sensors**: Powered off in sleep mode, shared power control

### Robust Initialization
- **Retry Logic**: All sensors have 3-attempt initialization with appropriate delays
- **Graceful Degradation**: System continues operation even if some sensors fail
- **Error Handling**: Comprehensive logging and status reporting

### Radio Communication
- **AT Command Protocol**: Proper `+++` command handling without terminators
- **Power Management**: Automatic high/low power switching with retry logic
- **Settings Persistence**: Radio configuration saved to EEPROM with automatic reboot

## Customization

### Adding Sensors

1. Create new sensor class in `include/` and `src/`
2. Add initialization in `SystemController::initialize()`
3. Include sensor reading in `SystemController::updateSensors()`
4. Update `TelemetryData` structure in `config.h`
5. Modify telemetry transmission in `RadioModule::sendTelemetry()`
6. Update web interface JSON in `WiFiManager::createTelemetryJSON()`

### Modifying Communications

- **Radio settings**: Adjust AT commands and power levels in `RadioModule`
- **WiFi credentials**: Update `WIFI_SSID` and `WIFI_PASSWORD` in `config.h`
- **Telemetry format**: Modify `RadioModule::sendTelemetry()` and corresponding parsers
- **Web interface**: Update HTML/CSS/JavaScript in `web_content.cpp`

### Timing Adjustments

- **Sensor rates**: Modify `SENSOR_READ_INTERVAL` and `GPS_READ_INTERVAL` in `config.h`
- **RSSI updates**: Adjust `RSSI_QUERY_INTERVAL` for different update frequencies
- **Maintenance timeout**: Change `MAINTENANCE_MODE_TIMEOUT` as needed

## Troubleshooting

### Common Issues

1. **GPS not responding**: Check wiring, baud rate, and retry initialization
2. **Pressure sensor communication errors**: Verify I2C connections and power
3. **IMU initialization fails**: Check I2C bus and magnetometer connectivity
4. **Radio initialization fails**: Confirm RFD900x is in correct mode, check AT commands
5. **WiFi connection problems**: Verify network credentials, range, and power management
6. **Power sensor errors**: Check I2C address and voltage range compatibility

### Debug Output

Enable detailed logging by monitoring serial output. Each module provides:
- Initialization status with retry attempts
- Sensor reading success/failure indicators
- Communication protocol debugging
- Power management state changes
- Mode transition logging

### Performance Monitoring

- **Memory usage**: Monitor PROGMEM usage for web content
- **Power consumption**: Use INA260 readings to optimize power usage
- **Communication reliability**: Monitor RSSI and failed transmission counts
- **Sensor health**: Track validity flags and error rates

## Safety Considerations

- **Comprehensive testing**: Test all functionality and failure modes before flight
- **Radio reliability**: Verify range, power levels, and command responsiveness
- **Power systems**: Monitor battery voltage and current consumption
- **Sensor validation**: Implement data sanity checks and backup systems
- **Antenna isolation**: Ensure proper RF isolation and grounding
- **Regulatory compliance**: Follow local regulations for radio equipment and frequencies

## Technical Specifications

### Performance
- **Sensor Update Rate**: 10Hz (pressure, IMU, power), 1Hz (GPS)
- **Telemetry Rate**: Configurable, optimized for radio bandwidth
- **Web Interface**: 2-second refresh rate with responsive design
- **Power Consumption**: Optimized for each mode (sleep/flight/maintenance)

### Memory Usage
- **PROGMEM**: Web content stored in flash memory to preserve RAM
- **RAM Optimization**: Efficient data structures and minimal dynamic allocation
- **Flash Usage**: Modular design for efficient code organization

## License

This project is provided as-is for educational and research purposes. Users are responsible for compliance with local regulations and safety requirements.
