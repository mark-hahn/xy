
/* Create a WiFi access point and provide a web server on it. */

#include <string.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

ESP8266WebServer server(80);
DNSServer dnsServer;


///////////////  UPDATES  //////////////
void do_firmware_update() {
	server.send(200, "text/plain", "Started firmware update");
	Serial.println("Updating firmware from http://hahnca.com/xy/firmware.bin");
	delay(1000);
	ESPhttpUpdate.rebootOnUpdate(true);
  t_httpUpdate_return ret = ESPhttpUpdate.update("http://hahnca.com/xy/firmware.bin");
  switch(ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("Firmware Update Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("No firmware Updates");
      break;
    default:
      Serial.println("firmware Updated (you shouldn't see this)");
      break;
  }
}

void do_spiffs_update() {
	server.send(200, "text/plain", "Started file system update");
	Serial.println("Updating file system from http://hahnca.com/xy/spiffs.bin");
	delay(1000);
	ESPhttpUpdate.rebootOnUpdate(false);
  t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(
															"http://hahnca.com/xy/spiffs.bin");
  switch(ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("FS Update Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("No FS Updates");
      break;
    default:
      Serial.println("FS Updated");
      break;
  }
}


/////////////  YIELD  TO WIFI  /////////////
void chkSrvr() {
	server.handleClient();
  dnsServer.processNextRequest();
}
void chkSrvrBlink() {
		int i;
		led_on();
		for (i=0; i<100; i++) {
			delay(1);
      chkSrvr();
		}
		led_off();
		for (i=0; i<100; i++) {
			delay(1);
      chkSrvr();
		}
}


/////////////  LEDS  /////////////
void led_on()  {digitalWrite(2, 0);}
void led_off() {digitalWrite(2, 1);}


/////////////  EEPROM  /////////////
/* eeprom map
  2  Magic = 0xedde
  33 AP password
	4 STA choices (66 each)
		 33 ssid
		 33 pwd
	299 bytes total
*/
#define EEPROM_BYTES_OFS      35
#define EEPROM_BYTES_PER_SSID 66

void initeeprom() {
	Serial.println(String("magic byte ") + String(EEPROM.read(0), HEX));
	if (EEPROM.read(0) != 0xed || EEPROM.read(1) != 0xde) {
		Serial.println("initializing empty eeprom");
		File f = SPIFFS.open("/empty-eeprom.dat", "r");
    char buf[1];
		int eeAddr;
		for (eeAddr=0; eeAddr<f.size(); eeAddr++) {
		  f.readBytes(buf,1);
			EEPROM.write(eeAddr, buf[0]);
		}
		EEPROM.commit();
		Serial.println(String("initialized ") + f.size() + " bytes");
		f.close();
	}
}
int eepromGetStr(char* res, int idx){
	int i=0;
	do { res[i] = EEPROM.read(idx); i++; idx++;} while (res[i]);
	return idx+33;
}
int eepromGetIP(IPAddress res, int idx){
	int i;
	for(i=0; i<4; i++) res[i] = EEPROM.read(idx+i);
	return idx+4;
}


/////////////  STA SETUP  /////////////
char sta_ssid[33];
char sta_pwd[33];
int best_quality = -1;

int find_and_connect_STA() {
	int n = WiFi.scanNetworks(), i, j, eepromIdx;
	char eeprom_ssid[33];
	Serial.println(String("Wifi scan found ") + n + " ssids");
	for(i=0; i<4; i++) {
		eepromIdx = eepromGetStr(eeprom_ssid, EEPROM_BYTES_OFS + i * EEPROM_BYTES_PER_SSID);
		for(j=0; j<n; j++) {
			if(strcmp(WiFi.SSID(j).c_str(), eeprom_ssid) == 0 &&
			      WiFi.encryptionType(j) == ENC_TYPE_NONE) {
		    int q, rssi = WiFi.RSSI(j);
			  if (rssi <= -100) q = 0;
			  else if (rssi >= -50) q = 100;
			  else q = 2 * (rssi + 100);
        if (q > best_quality) {
					strcpy(sta_ssid, eeprom_ssid);
					eepromIdx = eepromGetStr(sta_pwd,       eepromIdx);
          best_quality = q;
				}
			}
		}
	}
	// TODO  REMOVE DEBUG
  if(best_quality == -1) {
    Serial.println("no match found in scan, using debug settings");
		strcpy(sta_ssid, "hahn-fi");
		strcpy(sta_pwd, "NBVcvbasd987");
		best_quality = 101;
		// return 0;
	}
	Serial.println("Connecting to AP " + String(sta_ssid) + " with quality " + best_quality);
	if(WiFi.status() == WL_CONNECTED) WiFi.disconnect();
	WiFi.begin(sta_ssid, sta_pwd);
	while (WiFi.status() != WL_CONNECTED) chkSrvrBlink();
	led_off();
	return 1;
}


/////////////  HTTP HANDLERS  /////////////
void handleRoot() {
	server.send(200, "text/plain", "XY App");
}
void handle204() {
	server.send(200, "text/plain", "The XY application can usually be found at xy.local");
}

/////////////  SETUP  /////////////
void setup() {
	delay(1000);

	Serial.begin(115200);
	Serial.println("\nApp Start -- v5");
	Serial.println(String("FreeSketchSpace: ") + ESP.getFreeSketchSpace());

	pinMode(2, OUTPUT);
	SPIFFS.begin();
	WiFi.mode(WIFI_AP_STA);
	initeeprom();
	EEPROM.begin(512);


/////////////  AP  /////////////
	char ap_ssid[18];
  String ap_ssid_str = "eridien_XY_" + String(ESP.getChipId(), HEX);
	ap_ssid_str.toCharArray(ap_ssid, 18);
	char ap_pwd[33];
	eepromGetStr(ap_pwd, 2);

	Serial.print("Configuring access point " + String(ap_ssid) + ": ");
	WiFi.softAP(ap_ssid, ap_pwd);

	IPAddress apIP  = WiFi.softAPIP();
	Serial.print("AP address: "); Serial.println(apIP);


/////////////  DNS  /////////////
	const byte DNS_PORT = 53;
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());


/////////////  SERVER  /////////////
	server.on("/", handleRoot);
	server.on("/generate_204", handle204);
	server.on ( "/f", do_firmware_update);
	server.on ( "/fs", do_spiffs_update);
	server.begin();
	Serial.println("HTTP server started");


/////////////  STA  /////////////
	if (find_and_connect_STA()) {
		Serial.println("Connected as station " + WiFi.localIP().toString());
  }

/////////////  MDNS  /////////////
  MDNS.begin("xy");
  MDNS.addService("http", "tcp", 80);


/////////////  CLEAN UP SETUP  /////////////
	EEPROM.end();
}


/////////////  LOOP  /////////////
void loop() { chkSrvr(); }
