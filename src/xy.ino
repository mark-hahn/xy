
/* Create a WiFi access point and provide a web server on it. */

#include <string.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ArduinoOTA.h>

ESP8266WebServer server(80);
DNSServer dnsServer;


/////////////  OTA  /////////////
int otaCmd;
void startOTA() {
	ArduinoOTA.onStart([]() {
		String type;
		otaCmd = ArduinoOTA.getCommand();
    if (otaCmd == U_FLASH) type = "firmware";
                      else type = "filesystem";
    if(otaCmd == U_SPIFFS) SPIFFS.end();
    Serial.println("\nOTA updating " + type);
  });
  ArduinoOTA.onEnd([]() {
		if(otaCmd == U_SPIFFS) SPIFFS.begin();
    Serial.println("Finished OTA update");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", progress*100/total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
		const char* err_str;
		switch (error) {
			case OTA_AUTH_ERROR: err_str = "Auth"; break;
			case OTA_BEGIN_ERROR: err_str = "Begin"; break;
			case OTA_CONNECT_ERROR: err_str = "Connect"; break;
			case OTA_RECEIVE_ERROR: err_str = "Receive"; break;
			case OTA_END_ERROR: err_str = "End"; break;
		}
    Serial.println("OTA " + String(err_str) + " error");
		if(otaCmd == U_SPIFFS) SPIFFS.begin();
  });
	ArduinoOTA.begin();
}


/////////////  YIELD  TO WIFI  /////////////
void chkSrvr() {
	server.handleClient();
  dnsServer.processNextRequest();
	ArduinoOTA.handle();
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
	4 STA choices (86 each)
		 33 ssid
		 33 pwd
		 4  static ip
		 4  gateway ip
		 4  subnet
		 4  dns1 ip
		 4  dns2 ip
	379 bytes total
*/
#define EEPROM_BYTES_OFS      35
#define EEPROM_BYTES_PER_SSID 86

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
IPAddress sta_static_ip;
IPAddress sta_gateway_ip = ip(192.168.1.1);
IPAddress sta_subnet;
IPAddress sta_dns1_ip;
IPAddress sta_dns2_ip;
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
					int indx = 2+i*71;
					strcpy(sta_ssid, eeprom_ssid);
					eepromIdx = eepromGetStr(sta_pwd,       eepromIdx);
					eepromIdx = eepromGetIP(sta_static_ip,  eepromIdx);
					eepromIdx = eepromGetIP(sta_gateway_ip, eepromIdx);
					eepromIdx = eepromGetIP(sta_subnet,     eepromIdx);
					eepromIdx = eepromGetIP(sta_dns1_ip,    eepromIdx);
					eepromIdx = eepromGetIP(sta_dns2_ip,    eepromIdx);
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
	if(sta_static_ip[0] || sta_gateway_ip[0] ||
		    sta_subnet[0] || sta_dns1_ip[0] || sta_dns2_ip[0] )
		WiFi.config(sta_static_ip, sta_gateway_ip,
			         sta_subnet, sta_dns1_ip, sta_dns2_ip);
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
	ESP.getFreeSketchSpace();
	delay(1000);
	Serial.println("\napp start");

	pinMode(2, OUTPUT);
	Serial.begin(115200);
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
	server.begin();
	Serial.println("HTTP server started");


/////////////  STA  /////////////
	if (find_and_connect_STA()) {
		Serial.println("Connected as station " + WiFi.localIP().toString());
		startOTA();
  }

/////////////  MDNS  /////////////
  MDNS.begin("xy");
  MDNS.addService("http", "tcp", 80);


/////////////  CLEAN UP SETUP  /////////////
	EEPROM.end();
}


/////////////  LOOP  /////////////
void loop() { chkSrvr(); }
