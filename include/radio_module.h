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
  
  void sendATCommand(String command);
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
  bool isValid();
};

#endif
