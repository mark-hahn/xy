
/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
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
  33 AP ssid name
  33 AP password
	4 STA choices (70 each)
		 33 ssid
		 33 pwd
		 4  static ip
	348 bytes total
*/
#define EEPROM_BYTES_OFS      68
#define EEPROM_BYTES_PER_SSID 70

void initeeprom() {
	char buf[33];
	int bufidx, eeAddr;
	EEPROM.begin(512);
	if (EEPROM.read(0) != 0xed || EEPROM.read(1) != 0xde) {
		Serial.println("initializing empty eeprom, magic was: " +
										String(EEPROM.read(0),HEX) + String(EEPROM.read(1),HEX));
		EEPROM.write(0, 0xed);
		EEPROM.write(1, 0xde);
		for (eeAddr=2; eeAddr<348; eeAddr++) { EEPROM.write(eeAddr, 0); }

	  String ap_ssid_str = "eridien_XY_" + String(ESP.getChipId(), HEX);
		ap_ssid_str.toCharArray(buf, 33);
		for(bufidx=0; buf[bufidx]; bufidx++) EEPROM.write(bufidx+2, buf[bufidx]);
		EEPROM.write(bufidx+2, 0);

    String("eridien").toCharArray(buf, 33);
		for(bufidx=0; buf[bufidx]; bufidx++) EEPROM.write(bufidx+35, buf[bufidx]);
		EEPROM.write(bufidx+35, 0);
		EEPROM.commit();
		Serial.println("initialized eeprom");
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
	Serial.println(String("\nWifi scan found ") + n + " ssids");
	for(i=0; i<4; i++) {
		eepromIdx = eepromGetStr(eeprom_ssid, EEPROM_BYTES_OFS + i * EEPROM_BYTES_PER_SSID);
		for(j=0; j<n; j++) {
			if(strcmp(WiFi.SSID(j).c_str(), eeprom_ssid) == 0 &&
			      WiFi.encryptionType(j) == ENC_TYPE_NONE) {
        if (WiFi.RSSI(j) > best_quality) {
					strcpy(sta_ssid, eeprom_ssid);
					eepromIdx = eepromGetStr(sta_pwd, eepromIdx);
          best_quality = WiFi.RSSI(j);
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
File fsUploadFile;

String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
	Serial.println(String("reading ") + path);
	if (path.endsWith("/")) path += "index.html";
	path.replace("scripts/", "");
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
	bool gzExists;
  if( (gzExists = SPIFFS.exists(pathWithGz)) || SPIFFS.exists(path)){
    if(gzExists) path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
		Serial.println(String("sent ") + sent + " bytes from file " + path);
    file.close();
    return true;
  }
  return false;
}
void handleFileUpload(){
	Serial.println(String("handleFileUpload: ") + server.uri());
	if(server.uri() != "/upload") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
		Serial.println(String("status: ") + upload.status);
		Serial.println(String("filename: ") + upload.filename);
		Serial.println(String("name: ") + upload.name);
		Serial.println(String("type: ") + upload.type);
		Serial.println(String("totalSize: ") + upload.totalSize);
		Serial.println(String("currentSize: ") + upload.currentSize);
    if(!filename.startsWith("/")) filename = "/"+filename;
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    Serial.println(String("UPLOAD_FILE_WRITE size: ")+upload.currentSize);
    if(fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) fsUploadFile.close();
    Serial.println(String("UPLOAD_FILE_END size: ")+upload.totalSize);
  }
}
void handle204() {
	server.send(200, "text/plain", "The XY application can usually be found at xy.local");
}
void handle_list_ssids() {
	WiFiClient client = server.client();
	int n = WiFi.scanNetworks(), i, j, eepromIdx;
	Serial.println(String("Wifi scan listing ") + n + " ssids");
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json;charset=utf-8");
  client.println("Server: Arduino");
  client.println("Connection: close");
  client.println();
  client.print("[");
	for(j=0; j<n; j++) {
		client.print("{\"ssid\":\"" + WiFi.SSID(j) + "\",");
		client.print(String("\"rssi\":") + WiFi.RSSI(j) + "}" +
	    																		(j==n-1?"":","));
	}
	client.println("]");
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


/////////////  AP  /////////////
	char ap_ssid[33];
	char ap_pwd[33];
	eepromGetStr(ap_ssid, 2);
	eepromGetStr(ap_pwd, 35);

	Serial.print("Configuring access point " + String(ap_ssid) + ": ");
	WiFi.softAP(ap_ssid, ap_pwd);

	IPAddress apIP  = WiFi.softAPIP();
	Serial.print("AP address: "); Serial.println(apIP);


/////////////  DNS  /////////////
	const byte DNS_PORT = 53;
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());


/////////////  SERVER  /////////////
	server.on ( "/f", do_firmware_update);
	server.on ( "/fs", do_spiffs_update);
	server.on ( "/ssids", handle_list_ssids);
	server.on("/generate_204", handle204);
  server.on("/edit", HTTP_POST,
						[](){server.send(200, "text/plain", "");}, handleFileUpload);
  // server.onFileUpload(handleFileUpload);
	server.onNotFound([](){
		if(!handleFileRead(server.uri()))
			Serial.println(String("File Not Found: ") + server.uri());
			server.send(404, "text/plain", "File Not Found");
		});
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
