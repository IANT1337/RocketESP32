#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "config.h"

class GPSModule {
private:
  HardwareSerial* gpsSerial;
  bool initialized;
  
  // NMEA parsing helpers
  bool parseNMEA(String nmea);
  bool parseGGA(String gga, float& lat, float& lon, float& alt);
  float parseCoordinate(String coord, char direction);

public:
  GPSModule();
  ~GPSModule();
  
  void initialize();
  bool readData(float& latitude, float& longitude, float& altitude);
  bool isValid();
};

#endif
