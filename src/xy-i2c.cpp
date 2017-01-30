

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
	// mcu i2c addr does not include rw bit so it ranges from 1 up
	Wire.beginTransmission(mcu);
  Wire.write(bankAddr); // can also be immediate command
	Wire.write(buf, qty);
  int error = Wire.endTransmission();
	if(error) Serial.println(String("i2c error:") + error);
}

void writeI2cRaw(char i2cAddr, char *buf, int qty) {
	// i2cAddr does not include rw bit so it ranges from 1 up
	Wire.beginTransmission(i2cAddr);
	Wire.write(buf, qty);
  int error = Wire.endTransmission();
	if(error) Serial.println(String("i2c raw error error, addr, qty: ") + error +
                           ", " + ((int)i2cAddr) + ", " + qty);
}

char readI2cRaw(char addr, char *buf, int qty) {
	int idx = 0;
	Wire.requestFrom(addr, qty);
	while(Wire.available()) {
		buf[idx++] = Wire.read();
		if (idx == qty) break;
	}
	while(Wire.available()) {Wire.read();}
	return idx;
}
