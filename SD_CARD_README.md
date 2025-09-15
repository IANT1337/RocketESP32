# Dual SD Card Storage Implementation

This implementation adds dual SD card storage functionality to the RocketESP32 flight computer for reliable data storage with automatic failover capability.

## Features

- **Dual SD Card Support**: Primary and backup SD cards for redundancy
- **Automatic Failover**: Switches to backup card if primary card fails during write operations
- **Persistent Retry**: Continuously retries SD card initialization even if both cards fail initially
- **Data Buffering**: Continues collecting data in memory even when both cards are failed
- **Health Monitoring**: Regular health checks of active SD cards with automatic switching
- **Batch Storage**: Data is collected in batches to minimize write operations
- **SPI Interface**: Uses SPI communication for reliable high-speed data transfer
- **Auto-flush**: Automatically writes batches when full, and can force-flush partial batches on mode changes
- **CSV Format**: Data is stored in human-readable CSV format for easy analysis
- **Web Integration**: SD card status shows which card is active and retry status through the maintenance mode web interface
- **Error Handling**: Graceful handling of SD card failures with automatic backup switching and recovery

## Hardware Connections

For Arduino Nano ESP32, connect SD card modules to:

**Primary SD Card:**
- CS (Chip Select): D10 (GPIO10)
- MOSI (Master Out Slave In): D11 (GPIO11)  
- MISO (Master In Slave Out): D12 (GPIO12)
- SCK (Serial Clock): D13 (GPIO13)
- VCC: 3.3V
- GND: Ground

**Backup SD Card:**
- CS (Chip Select): D5 (GPIO5)
- MOSI (Master Out Slave In): D11 (GPIO11) - shared with primary
- MISO (Master In Slave Out): D12 (GPIO12) - shared with primary  
- SCK (Serial Clock): D13 (GPIO13) - shared with primary
- VCC: 3.3V
- GND: Ground

## Configuration

SD card settings are defined in `config.h`:
- `SD_CS_PIN`: Primary SD card chip select pin (D10)
- `SD_CS_BACKUP_PIN`: Backup SD card chip select pin (D5)
- `SD_BATCH_SIZE`: Number of telemetry records per batch (default: 10)
- `SD_MAX_LOG_FILES`: Maximum number of log files to keep (default: 20)
- `SD_HEALTH_CHECK_INTERVAL`: Interval for checking SD card health (default: 2000ms)
- `SD_MAX_CONSECUTIVE_FAILURES`: Maximum consecutive failures before switching cards (default: 3)
- `SD_RETRY_INTERVAL`: Interval for retrying failed card initialization (default: 10000ms)

## File Format

Data is stored in CSV files with the following format:
```
timestamp,mode,lat,lon,alt_gps,alt_press,pressure,gps_valid,press_valid,rssi,
accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,mag_x,mag_y,mag_z,imu_temp,imu_valid,
voltage,current,power,power_valid
```

## Usage

The dual SD card storage with persistent retry is automatically integrated into the system controller:

1. **Initialization**: System tries to initialize primary SD card first, falls back to backup if primary fails
2. **Persistent Retry**: If both cards fail at startup, system continues running and retries initialization every 10 seconds
3. **Data Collection**: Every telemetry reading is automatically added to the current batch, even when cards are failed
4. **Data Buffering**: When no cards are working, most recent data is kept in memory buffer 
5. **Automatic Recovery**: When a card comes back online, buffered data is immediately written
6. **Health Monitoring**: Active cards are checked every 2 seconds for continued operation
7. **Batch Writing**: When a batch is full and cards are available, it's written to the active SD card
8. **Runtime Failover**: If a write operation fails on the primary card, system automatically switches to backup card
9. **Mode Changes**: Data is flushed when changing system modes to ensure no data loss
10. **Web Monitoring**: SD card status shows which card is active and retry countdown at `/sdstatus` endpoint in maintenance mode

## Failover Behavior

1. **Startup**: Primary card is tried first, backup used only if primary fails
2. **Runtime Failover**: If primary card fails during write operations:
   - Current batch is flushed if possible
   - System switches to backup card
   - New log file is created on backup card
   - Operation continues on backup card
   - Status messages indicate active card

## Classes

### SDManager
Main class handling all dual SD card operations:
- `initialize()`: Initialize SD card system (try primary, fallback to backup)
- `addData(TelemetryData)`: Add data to current batch with failover support
- `flushCurrentBatch()`: Write current batch with automatic failover
- `forceSync()`: Force write of partial batch
- `switchToBackupCard()`: Manual switch to backup card
- `isPrimaryCardActive()`: Check if primary card is active
- `isBackupCardActive()`: Check if backup card is active
- `getActiveCard()`: Get currently active card slot
- `listLogFiles()`: List all log files on active card
- `getSDCardStatus()`: Get status string showing active card

## Error Handling

- If both SD cards fail during initialization, system continues without storage
- Write errors trigger automatic failover to backup card if available
- Web interface shows SD card availability and which card is active
- Serial output includes active card information in status messages
- Failed writes are logged but don't affect system operation

## Performance Considerations

- Batch writing reduces SD card wear by minimizing write operations
- 4MHz SPI speed provides good balance of speed and reliability  
- Only one card is active at a time to avoid SPI conflicts
- Failover adds minimal overhead - only occurs on actual write failures
- CSV format is human-readable but larger than binary formats

## Testing

Use `test/test_sd_card.cpp` to verify dual SD card functionality:
- Tests initialization priority (primary first, then backup)
- Writes test data in batches to verify both cards work
- Simulates failover scenarios
- Shows which card is active during operations
- Verifies file creation and data storage on both cards
