#include "gps_module.h"

GPSModule::GPSModule() : initialized(false) {
  gpsSerial = new HardwareSerial(1);
}

GPSModule::~GPSModule() {
  delete gpsSerial;
}

void GPSModule::initialize() {
  Serial.println("Initializing GPS module...");
  
  // Initialize GPS serial communication
  gpsSerial->begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_SERIAL_RX_PIN, GPS_SERIAL_TX_PIN);
  
  // Brief delay for serial to stabilize, but not too long to cause watchdog
  delay(100);
  
  // Clear any existing data in the buffer
  while (gpsSerial->available()) {
    gpsSerial->read();
  }
  
  initialized = true;
  Serial.println("GPS module initialized");
}

bool GPSModule::readData(float& latitude, float& longitude, float& altitude) {
  if (!initialized) {
    return false;
  }
  
  // Read available NMEA sentences
  while (gpsSerial->available()) {
    String nmea = gpsSerial->readStringUntil('\n');
    nmea.trim();
    
    if (nmea.startsWith("$GPGGA") || nmea.startsWith("$GNGGA")) {
      if (parseGGA(nmea, latitude, longitude, altitude)) {
        return true;
      }
    }
  }
  
  return false;
}

bool GPSModule::isValid() {
  return initialized;
}

bool GPSModule::parseNMEA(String nmea) {
  // Basic NMEA checksum validation
  int checksumIndex = nmea.lastIndexOf('*');
  if (checksumIndex == -1) {
    return false;
  }
  
  // Extract checksum
  String checksumStr = nmea.substring(checksumIndex + 1);
  int expectedChecksum = strtol(checksumStr.c_str(), NULL, 16);
  
  // Calculate actual checksum
  int calculatedChecksum = 0;
  for (int i = 1; i < checksumIndex; i++) {
    calculatedChecksum ^= nmea.charAt(i);
  }
  
  return calculatedChecksum == expectedChecksum;
}

bool GPSModule::parseGGA(String gga, float& lat, float& lon, float& alt) {
  if (!parseNMEA(gga)) {
    return false;
  }
  
  // Split GGA sentence by commas
  int fieldIndex = 0;
  int startIndex = 0;
  String fields[15];
  
  for (int i = 0; i <= gga.length(); i++) {
    if (i == gga.length() || gga.charAt(i) == ',') {
      if (fieldIndex < 15) {
        fields[fieldIndex] = gga.substring(startIndex, i);
        fieldIndex++;
      }
      startIndex = i + 1;
    }
  }
  
  // Check if we have a valid fix
  if (fields[6].toInt() == 0) {
    return false; // No GPS fix
  }
  
  // Parse latitude (field 2 and 3)
  if (fields[2].length() > 0 && fields[3].length() > 0) {
    lat = parseCoordinate(fields[2], fields[3].charAt(0));
  } else {
    return false;
  }
  
  // Parse longitude (field 4 and 5)
  if (fields[4].length() > 0 && fields[5].length() > 0) {
    lon = parseCoordinate(fields[4], fields[5].charAt(0));
  } else {
    return false;
  }
  
  // Parse altitude (field 9)
  if (fields[9].length() > 0) {
    alt = fields[9].toFloat();
  } else {
    alt = 0.0;
  }
  
  return true;
}

float GPSModule::parseCoordinate(String coord, char direction) {
  if (coord.length() < 4) {
    return 0.0;
  }
  
  // Extract degrees and minutes
  float degrees, minutes;
  
  if (direction == 'N' || direction == 'S') {
    // Latitude: DDMM.MMMM
    degrees = coord.substring(0, 2).toFloat();
    minutes = coord.substring(2).toFloat();
  } else {
    // Longitude: DDDMM.MMMM
    degrees = coord.substring(0, 3).toFloat();
    minutes = coord.substring(3).toFloat();
  }
  
  float result = degrees + (minutes / 60.0);
  
  // Apply direction
  if (direction == 'S' || direction == 'W') {
    result = -result;
  }
  
  return result;
}
