#ifndef CONFIG_H
#define CONFIG_H

// Arduino Nano ESP32 (ESP32-S3) compatibility
#ifdef ARDUINO_NANO_ESP32
  #define BOARD_NAME "Arduino Nano ESP32"
  #define MCU_TYPE "ESP32-S3"
#else
  #define BOARD_NAME "ESP32"
  #define MCU_TYPE "ESP32"
#endif

// Pin definitions for Arduino Nano ESP32
// Using available GPIO pins that are not reserved for USB, Flash, or other functions
#define GPS_SERIAL_RX_PIN D7     // GPIO6
#define GPS_SERIAL_TX_PIN D6     // GPIO7
#define RADIO_SERIAL_RX_PIN D3  // GPIO11
#define RADIO_SERIAL_TX_PIN D2  // GPIO10
#define PRESSURE_SDA_PIN D9      // GPIO9 - I2C SDA
#define PRESSURE_SCL_PIN D8      // GPIO8 - I2C SCL
#define CAMERA_POWER_PIN D4     // GPIO12 - Camera power control
//#define RADIO_POWER_PIN D14    // GPIO14 - Radio power control (commented out)
#define STATUS_LED_PIN LED_BUILTIN // Use built-in LED (GPIO13 on Nano ESP32)

// SD Card SPI pins for Arduino Nano ESP32
#define SD_CS_PIN D10           // GPIO10 - Primary SD Card Chip Select
#define SD_CS_BACKUP_PIN D5     // GPIO5 - Backup SD Card Chip Select
#define SD_MOSI_PIN D11         // GPIO11 - SD Card MOSI (Master Out Slave In)
#define SD_MISO_PIN D12         // GPIO12 - SD Card MISO (Master In Slave Out)
#define SD_SCK_PIN D13          // GPIO13 - SD Card SCK (Serial Clock)

// Serial communication settings
#define GPS_BAUD_RATE 9600  // Matek M10Q-5883 default baud rate (can support up to 460800)
#define RADIO_BAUD_RATE 115200

// I2C settings
#define I2C_FREQUENCY 100000

// Timing settings (in milliseconds)
#define SENSOR_READ_INTERVAL 10      // Fast sensors (IMU) read interval
#define PRESSURE_READ_INTERVAL 50     // Slow sensors (pressure) read interval
#define POWER_READ_INTERVAL 50      // Power monitoring read interval
#define GPS_READ_INTERVAL 1000       // GPS read interval (1 second)
#define RADIO_LISTEN_INTERVAL 500
#define RADIO_TX_INTERVAL 100        // Radio transmission interval (100ms = 10Hz)
#define HEARTBEAT_INTERVAL 2000
#define MAINTENANCE_TIMEOUT 300000   // 5 minutes
#define RSSI_QUERY_INTERVAL 10000    // 10 seconds

// Threading settings
#define BACKGROUND_TASK_STACK_SIZE 4096
#define BACKGROUND_TASK_PRIORITY 1      // Lower priority than main loop (which runs at priority 1)
#define BACKGROUND_TASK_CORE 0          // Run on core 0 (main loop typically runs on core 1)

#define SENSOR_TASK_STACK_SIZE 4096
#define SENSOR_TASK_PRIORITY 2          // Higher priority than background task
#define SENSOR_TASK_CORE 0              // Run on core 0 with background task

// WiFi settings
#define WIFI_SSID "GF7H5"
#define WIFI_PASSWORD "tastemy1337chicken"
#define WEBSERVER_PORT 80

// SD Card settings
#define SD_BATCH_SIZE 100       // Number of telemetry records per batch
#define SD_MAX_LOG_FILES 2000     // Maximum number of log files to keep
#define SD_SPI_SPEED 4000000    // SD card SPI speed (4MHz)
#define SD_HEALTH_CHECK_INTERVAL 2000  // Check card health every 2 seconds
#define SD_MAX_CONSECUTIVE_FAILURES 3   // Max failures before trying other card
#define SD_RETRY_INTERVAL 1000  // Retry SD initialization every 10 seconds when both fail

// Radio commands
#define CMD_FLIGHT_MODE "FLIGHT"
#define CMD_SLEEP_MODE "SLEEP"
#define CMD_MAINTENANCE_MODE "MAINT"
#define CMD_CAM_TOGGLE "CAM_TOGGLE"

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
