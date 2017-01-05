
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
	4 STA choices (71 each)
		 33 ssid
		 33 pwd
		 4  static ip
		 1  min quality
	319 bytes total
*/

void initEEROM() {
	Serial.println(String("magic byte ") + String(EEPROM.read(0), HEX));
	if (EEPROM.read(0) != 0xed || EEPROM.read(1) != 0xde) {
		Serial.println("initializing empty eerom");
		File f = SPIFFS.open("/empty-eerom.dat", "r");
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
char* getString(char* res, int idx){
	byte i=0, byt = 1;
	do { res[i] = EEPROM.read(idx); i++; idx++;}
	  while (res[i]);
}
int get_int(int idx){
	int i, res = 0;
	for(i=0; i<4; i++) {res=res << 8 || EEPROM.read(idx+i);}
	return res;
}
int get_byte(int idx) {
	return EEPROM.read(idx);
}
void get_ap_pwd(char* res)           {res = getString(res, 2);}
void get_ssid(char* res, int idx)    {res = getString(res, 2+idx*71+0);}
void get_sta_pwd(char* res, int idx) {res = getString(res, 2+idx*71+33);}
int get_static_ip(int idx){return get_int(2+idx*71+66);}
int get_quality(int idx){return get_byte(2+idx*71+70);}


/////////////  AP SETUP  /////////////
char ap_ssid[18];
String ap_ssid_str = "eridien_XY_" + String(ESP.getChipId(), HEX);


/////////////  STA SETUP  /////////////
char sta_ssid[33];
char sta_pwd[33];
int  sta_static_ip;
int  sta_quality = -1;

int find_and_connect_STA() {
	int n = WiFi.scanNetworks(), i, j;
	char eerom_ssid[33];
	Serial.println(String("Wifi scan found ") + n + " ssids");
	for(i=0; i<4; i++) {
		get_ssid(eerom_ssid, i);
		for(j=0; j<n; j++) {
			if(strcmp(WiFi.SSID(j).c_str(), eerom_ssid) == 0 &&
			      WiFi.encryptionType(j) == ENC_TYPE_NONE) {
		    int q, rssi = WiFi.RSSI(j);
			  if (rssi <= -100) q = 0;
			  else if (rssi >= -50) q = 100;
			  else q = 2 * (rssi + 100);
        if (q > get_quality(i) && q > sta_quality) {
					strcpy(sta_ssid, eerom_ssid);
					get_sta_pwd(sta_pwd, i);
					sta_static_ip = get_static_ip(i);
          sta_quality = q;
				}
			}
		}
	}
	// TODO  DEBUG
  if(sta_quality == -1) {
    Serial.println("no match found in scan, using debug settings");
		strcpy(sta_ssid, "hahn-fi");
		strcpy(sta_pwd, "NBVcvbasd987");
		sta_quality = 101;
		// return 0;
	}
	Serial.println("Trying AP " + String(sta_ssid) + " with quality " + sta_quality);
	if(WiFi.status() == WL_CONNECTED) WiFi.disconnect();
	WiFi.begin(sta_ssid, sta_pwd);
	while (WiFi.status() != WL_CONNECTED) chkSrvrBlink();
	return 1;
}


/////////////  HTTP HANDLERS  /////////////
void handleRoot() {
	server.send(200, "text/plain", "XY App");
}

void handle204() {
	server.send(200, "text/plain", "The XY application can be found at xy.com when using the local " + ap_ssid_str);
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
	initEEROM();
	EEPROM.begin(512);


/////////////  AP  /////////////
  char ap_pwd[33];
	get_ap_pwd(ap_pwd);
  ap_ssid_str.toCharArray(ap_ssid, 48);

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
		led_off();
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
