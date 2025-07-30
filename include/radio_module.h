#ifndef RADIO_MODULE_H
#define RADIO_MODULE_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "config.h"

class RadioModule {
private:
  HardwareSerial* radioSerial;
  bool initialized;
  bool highPowerMode;
  unsigned long lastRSSIQuery;
  int16_t cachedRSSI;
  
  void sendATCommand(String command, bool addTerminator = true);
  String readATResponse(unsigned long timeout = 1000);
  bool setParameter(String param, String value);

public:
  RadioModule();
  ~RadioModule();
  
  void initialize();
  void setHighPower();
  void setLowPower();
  void sendTelemetry(const TelemetryData& data);
  String receiveCommand();
  int16_t getRSSI();
  bool isValid();
  void sendAcknowledgment(String message);    
};

#endif
