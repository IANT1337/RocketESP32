#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "config.h"

class PowerManager {
private:
  bool sensorsEnabled;
  bool radioEnabled;
  bool initialized;

public:
  PowerManager();
  ~PowerManager();
  
  void initialize();
  void enableSensors();
  void disableSensors();
  void enableRadio();
  void disableRadio();
  bool areSensorsEnabled() const { return sensorsEnabled; }
  bool isRadioEnabled() const { return radioEnabled; }
  bool isValid() const { return initialized; }
};

#endif
