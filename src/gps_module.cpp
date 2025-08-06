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
  
  // Brief delay for serial to stabilize
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
  
  // Read all available NMEA sentences
  while (gpsSerial->available()) {
    String nmea = gpsSerial->readStringUntil('\n');
    nmea.trim();
    
    // Process GGA sentences (Global Positioning System Fix Data)
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
  // Simple NMEA validation - check for proper format
  if (nmea.length() < 10 || !nmea.startsWith("$")) {
    return false;
  }
  
  // Check for checksum if present
  int checksumIndex = nmea.lastIndexOf('*');
  if (checksumIndex == -1) {
    return true; // No checksum, assume valid
  }
  
  // Validate checksum
  String checksumStr = nmea.substring(checksumIndex + 1);
  if (checksumStr.length() != 2) {
    return false;
  }
  
  int expectedChecksum = strtol(checksumStr.c_str(), NULL, 16);
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
  
  // Parse GGA sentence: $GPGGA,time,lat,N/S,lon,E/W,quality,numSat,hdop,alt,M,geoidHeight,M,dgpsTime,dgpsID*checksum
  int commaCount = 0;
  int startIndex = 0;
  String fields[15];
  
  // Split by commas
  for (int i = 0; i <= gga.length(); i++) {
    if (i == gga.length() || gga.charAt(i) == ',') {
      if (commaCount < 15) {
        fields[commaCount] = gga.substring(startIndex, i);
      }
      commaCount++;
      startIndex = i + 1;
    }
  }
  
  // Check GPS fix quality (field 6: 0=invalid, 1=GPS fix, 2=DGPS fix)
  if (commaCount < 10 || fields[6].toInt() == 0) {
    return false; // No GPS fix
  }
  
  // Parse latitude (fields 2 and 3)
  if (fields[2].length() > 0 && fields[3].length() > 0) {
    lat = parseCoordinate(fields[2], fields[3].charAt(0));
  } else {
    return false;
  }
  
  // Parse longitude (fields 4 and 5)
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
  
  float degrees, minutes;
  
  // Determine format based on direction (latitude vs longitude)
  if (direction == 'N' || direction == 'S') {
    // Latitude format: DDMM.MMMM (2 digits degrees)
    degrees = coord.substring(0, 2).toFloat();
    minutes = coord.substring(2).toFloat();
  } else {
    // Longitude format: DDDMM.MMMM (3 digits degrees)
    degrees = coord.substring(0, 3).toFloat();
    minutes = coord.substring(3).toFloat();
  }
  
  // Convert to decimal degrees
  float result = degrees + (minutes / 60.0);
  
  // Apply hemisphere direction
  if (direction == 'S' || direction == 'W') {
    result = -result;
  }
  
  return result;
}
