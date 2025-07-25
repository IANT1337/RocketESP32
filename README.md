# Rocket Flight Computer ESP32

A modular flight computer software for ESP32 Feather V2 that interfaces with GPS, pressure sensors, and radio communication for rocketry applications.

## Features

- **Three Operating Modes:**
  - **Sleep Mode**: Low power consumption, sensors off, periodic radio listening
  - **Flight Mode**: High power, active sensor reading and telemetry transmission
  - **Maintenance Mode**: WiFi connectivity for web-based monitoring and configuration

- **Sensor Integration:**
  - GoouuuTech GT-U7 GPS module (UART communication)
  - MPRLS pressure sensor (I2C communication)
  - RFD900x radio modem (UART communication)

- **Power Management:**
  - Controllable sensor power switching
  - Dynamic radio power level adjustment
  - Power-optimized sleep mode

- **Communication:**
  - Serial radio telemetry transmission
  - Remote mode switching via radio commands
  - WiFi web server for maintenance mode

## Hardware Connections

### ESP32 Feather V2 Pin Assignments

| Component | ESP32 Pin | Function |
|-----------|-----------|----------|
| GPS RX | 16 | GPS Serial Receive |
| GPS TX | 17 | GPS Serial Transmit |
| Radio RX | 33 | Radio Serial Receive |
| Radio TX | 32 | Radio Serial Transmit |
| Pressure SDA | 23 | I2C Data |
| Pressure SCL | 22 | I2C Clock |
| Sensor Power | 27 | Sensor Power Control |
| Radio Power | 14 | Radio Power Control |
| Status LED | 13 | System Status Indicator |

## Software Architecture

The application is structured using a modular design:

### Core Modules

- **SystemController**: Main state machine and coordination
- **GPSModule**: NMEA parsing and GPS data acquisition
- **PressureSensor**: MPRLS sensor interface and altitude calculation
- **RadioModule**: RFD900x communication and AT command handling
- **PowerManager**: Hardware power control
- **WiFiManager**: Web server and wireless connectivity

### Configuration

All hardware-specific settings are centralized in `include/config.h`:
- Pin assignments
- Communication parameters
- Timing intervals
- WiFi credentials
- System constants

## Building and Deployment

### Prerequisites

- PlatformIO IDE or CLI
- ESP32 development environment
- Adafruit ESP32 Feather V2 board

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
- `FLIGHT` - Enter flight mode
- `SLEEP` - Enter sleep mode  
- `MAINT` - Enter maintenance mode

### Telemetry Format

Radio telemetry packets are CSV formatted:
```
TELEM,timestamp,mode,latitude,longitude,altitude_gps,altitude_pressure,pressure,gps_valid,pressure_valid
```

### Maintenance Mode

When in maintenance mode:
1. System connects to WiFi network
2. Web server starts on port 80
3. Access via ESP32's IP address for real-time monitoring
4. Automatic timeout after 5 minutes

## Customization

### Adding Sensors

1. Create new sensor class in `include/` and `src/`
2. Add initialization in `SystemController::initialize()`
3. Include sensor reading in `SystemController::updateSensors()`
4. Update telemetry structure in `config.h`

### Modifying Communications

- Radio settings: Adjust AT commands in `RadioModule`
- WiFi credentials: Update `config.h`
- Telemetry format: Modify `RadioModule::sendTelemetry()`

## Troubleshooting

### Common Issues

1. **GPS not responding**: Check wiring and baud rate
2. **Pressure sensor communication errors**: Verify I2C connections
3. **Radio initialization fails**: Confirm RFD900x is in correct mode
4. **WiFi connection problems**: Verify network credentials and range

### Debug Output

Enable detailed logging by monitoring serial output. Each module provides status messages during initialization and operation.

## Safety Considerations

- Test all functionality thoroughly before flight
- Verify radio range and reliability
- Implement proper antenna isolation
- Consider backup power systems
- Follow local regulations for radio equipment

## License

This project is provided as-is for educational and research purposes.
