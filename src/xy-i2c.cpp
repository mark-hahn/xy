

#include <Arduino.h>
#include "xy.h"
#include "xy-i2c.h"
#include "Wire.h"

void initI2c() {
	Wire.begin(SDA, SCL);  // also default
  Wire.setClock(400000);
}

void writeI2c(char mcuAddr, char bankAddr, char *buf, char qty) {
	Wire.beginTransmission(mcuAddr);
  Wire.write(bankAddr);
	Wire.write(buf, qty);
  int error = Wire.endTransmission();
	Serial.println(String("i2c send error:") + error);
}

	/*
	Wire.requestFrom(1, 6);    // request 6 bytes from slave device #1
  while(Wire.available())    // slave may send less than requested {
    char c = Wire.read();    // receive a byte as character
  }
*/
