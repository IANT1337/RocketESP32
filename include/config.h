#ifndef CONFIG_H
#define CONFIG_H

// Pin definitions for ESP32 Feather V2
#define GPS_SERIAL_RX_PIN 15
#define GPS_SERIAL_TX_PIN 14
#define RADIO_SERIAL_RX_PIN 33
#define RADIO_SERIAL_TX_PIN 32
#define PRESSURE_SDA_PIN 22
#define PRESSURE_SCL_PIN 20
#define SENSOR_POWER_PIN 27
//#define RADIO_POWER_PIN 14
#define STATUS_LED_PIN 13

// Serial communication settings
#define GPS_BAUD_RATE 9600  // Matek M10Q-5883 default baud rate (can support up to 460800)
#define RADIO_BAUD_RATE 115200

// I2C settings
#define I2C_FREQUENCY 100000

// Timing settings (in milliseconds)
#define SENSOR_READ_INTERVAL 25
#define GPS_READ_INTERVAL 1000      // 1 second
#define RADIO_LISTEN_INTERVAL 500
#define HEARTBEAT_INTERVAL 2000
#define MAINTENANCE_TIMEOUT 300000  // 5 minutes
#define RSSI_QUERY_INTERVAL 10000   // 10 seconds

// WiFi settings
#define WIFI_SSID "XXX"
#define WIFI_PASSWORD "XXX"
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
  int16_t rssi;  // Radio signal strength in dBm
  
  // IMU data from MPU9250
  float accel_x, accel_y, accel_z;       // Accelerometer (g)
  float gyro_x, gyro_y, gyro_z;          // Gyroscope (deg/s)
  float mag_x, mag_y, mag_z;             // Magnetometer (µT)
  float imu_temperature;                 // IMU temperature (°C)
  bool imu_valid;                        // IMU data validity
  
  // Power data from INA260
  float bus_voltage;                     // Bus voltage (V)
  float current;                         // Current (mA)
  float power;                           // Power (mW)
  bool power_valid;                      // Power data validity
};

#endif
