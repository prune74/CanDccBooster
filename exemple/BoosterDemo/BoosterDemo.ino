#include <CanDccBooster.h>
#include "Drv8874BoosterHardware.h"

Drv8874BoosterHardware hw;
CanDccBooster booster(hw);

void setup() {
  Serial.begin(115200);
}

void loop() {
  booster.update();
  auto &t = booster.getTelemetry();
  Serial.printf("Courant: %u mA, Tension: %u mV\n", t.current_mA, t.voltage_mV);
  delay(100);
}

