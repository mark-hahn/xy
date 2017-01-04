
/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <FS.h>

/* AP credentials. */
char ap_ssid[18];
String ap_ssid_str = "eridien_XY_" + String(ESP.getChipId(), HEX);
const char *ap_pwd = "NBVcvbasd987";

/* STA credentials. */
const char *sta_ssid = "hahn-fi";
const char *sta_pwd = "NBVcvbasd987";

const byte DNS_PORT = 53;
DNSServer dnsServer;
ESP8266WebServer server(80);

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
	EEPROM.begin(512);
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
	EEPROM.end();
}

/////////////  HTTP HANDLERS  /////////////
void handleRoot() {
	server.send(200, "text/plain", "XY App");
}

void handle204() {
	server.send(200, "text/plain", "The XY application can be found at xy.com when using the local " + ap_ssid_str);
}

void chkSrvr() {
	server.handleClient();
  dnsServer.processNextRequest();
}


/////////////  SETUP  /////////////
void setup() {
	delay(3000);

	pinMode(2, OUTPUT);
	Serial.begin(115200);
	SPIFFS.begin();
	initEEROM();
	WiFi.mode(WIFI_AP_STA);

	Serial.println();


/////////////  AP  /////////////
  ap_ssid_str.toCharArray(ap_ssid, 48);

	Serial.print("Configuring access point " + String(ap_ssid) + ": ");
	WiFi.softAP(ap_ssid, ap_pwd);

	IPAddress apIP  = WiFi.softAPIP();
	Serial.print("AP address: "); Serial.println(apIP);


/////////////  DNS  /////////////
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());


/////////////  SERVER  /////////////
	server.on("/", handleRoot);
	server.on("/generate_204", handle204);
	server.begin();
	Serial.println("HTTP server started");


/////////////  STA  /////////////
	Serial.print("Configuring station " + String(sta_ssid) + ": ");
  WiFi.begin(sta_ssid, sta_pwd);

  int timeout = 100;
	while (WiFi.status() != WL_CONNECTED && timeout-- > 0) {
    digitalWrite(2, 0);
    delay(150);
    digitalWrite(2, 1);
    delay(150);
		chkSrvr();
  }
	digitalWrite(2, 1);

	IPAddress staIP = WiFi.localIP();
	Serial.print("STA address: "); Serial.println(staIP);

/////////////  MDNS  /////////////
  MDNS.begin("xy");
  MDNS.addService("http", "tcp", 80);
}

/////////////  LOOP  /////////////
void loop() {
	chkSrvr();
}
