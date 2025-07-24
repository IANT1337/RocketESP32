#ifndef CONFIG_H
#define CONFIG_H

// Pin definitions for ESP32 Feather V2
#define GPS_SERIAL_RX_PIN 15
#define GPS_SERIAL_TX_PIN 12
#define RADIO_SERIAL_RX_PIN 33
#define RADIO_SERIAL_TX_PIN 32
#define PRESSURE_SDA_PIN 22
#define PRESSURE_SCL_PIN 20
#define SENSOR_POWER_PIN 27
#define RADIO_POWER_PIN 14
#define STATUS_LED_PIN 13

// Serial communication settings
#define GPS_BAUD_RATE 9600
#define RADIO_BAUD_RATE 57600

// I2C settings
#define I2C_FREQUENCY 100000

// Timing settings (in milliseconds)
#define SENSOR_READ_INTERVAL 1000
#define RADIO_LISTEN_INTERVAL 500
#define HEARTBEAT_INTERVAL 5000
#define MAINTENANCE_TIMEOUT 300000  // 5 minutes

// WiFi settings
#define WIFI_SSID "XXXX"
#define WIFI_PASSWORD "XXXX"
#define WEBSERVER_PORT 80

// Radio commands
#define CMD_FLIGHT_MODE "FLIGHT"
#define CMD_SLEEP_MODE "SLEEP"
#define CMD_MAINTENANCE_MODE "MAINT"

// System states
enum SystemMode {
  MODE_SLEEP,
  MODE_FLIGHT,
  MODE_MAINTENANCE
};

// Data packet structure
struct TelemetryData {
  float latitude;
  float longitude;
  float altitude_gps;
  float altitude_pressure;
  float pressure;
  uint32_t timestamp;
  SystemMode mode;
  bool gps_valid;
  bool pressure_valid;
};

#endif
