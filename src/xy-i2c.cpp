

#include <Arduino.h>
#include "Wire.h"
#include "xy.h"
#include "xy-i2c.h"
#include "xy-driver.h"

void initI2c() {
	Wire.begin(SDA, SCL);  // also default
  Wire.setClock(400000); // does nothing, core hacked to 700 KHz
}

void writeI2c(char mcu, char bankAddr, char *buf, char qty) {
	Wire.beginTransmission(mcu);
  Wire.write(bankAddr); // could also be immediate command
	Wire.write(buf, qty);
  int error = Wire.endTransmission();
	if(error) Serial.println(String("i2c error:") + error);
}
