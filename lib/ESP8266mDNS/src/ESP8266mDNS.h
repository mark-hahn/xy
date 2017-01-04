
#ifndef ESP8266MDNS_H
#define ESP8266MDNS_H

#include "ESP8266WiFi.h"
#include "WiFiUdp.h"

//this should be defined at build time
#ifndef ARDUINO_BOARD
#define ARDUINO_BOARD "generic"
#endif

class UdpContext;

struct MDNSService;
struct MDNSTxt;
struct MDNSAnswer;

class MDNSResponder {
public:
  MDNSResponder();
  ~MDNSResponder();
  bool begin(const char* hostName);
  //for compatibility
  bool begin(const char* hostName, IPAddress ip, uint32_t ttl=120);
  void update();

  void addService(char *service, char *proto, uint16_t port);
  void addService(const char *service, const char *proto, uint16_t port){
    addService((char *)service, (char *)proto, port);
  }
  void addService(String service, String proto, uint16_t port){
    addService(service.c_str(), proto.c_str(), port);
  }

  bool addServiceTxt(char *name, char *proto, char * key, char * value);
  void addServiceTxt(const char *name, const char *proto, const char *key,const char * value){
    addServiceTxt((char *)name, (char *)proto, (char *)key, (char *)value);
  }
  void addServiceTxt(String name, String proto, String key, String value){
    addServiceTxt(name.c_str(), proto.c_str(), key.c_str(), value.c_str());
  }

  int queryService(char *service, char *proto);
  int queryService(const char *service, const char *proto){
    return queryService((char *)service, (char *)proto);
  }
  int queryService(String service, String proto){
    return queryService(service.c_str(), proto.c_str());
  }
  String hostname(int idx);
  IPAddress IP(int idx);
  uint16_t port(int idx);

  void enableArduino(uint16_t port, bool auth=false);

  void setInstanceName(String name);
  void setInstanceName(const char * name){
    setInstanceName(String(name));
  }
  void setInstanceName(char * name){
    setInstanceName(String(name));
  }

private:
  struct MDNSService * _services;
  UdpContext* _conn;
  String _hostName;
  String _instanceName;
  struct MDNSAnswer * _answers;
  struct MDNSQuery * _query;
  bool _newQuery;
  bool _waitingForAnswers;
  WiFiEventHandler _disconnectedHandler;
  WiFiEventHandler _gotIPHandler;
  uint32_t _ip;

  bool _begin(const char* hostName, uint32_t ip, uint32_t ttl);
  uint32_t _getOurIp();
  uint16_t _getServicePort(char *service, char *proto);
  MDNSTxt * _getServiceTxt(char *name, char *proto);
  uint16_t _getServiceTxtLen(char *name, char *proto);
  void _parsePacket();
  void _reply(uint8_t replyMask, char * service, char *proto, uint16_t port);
  size_t advertiseServices(); // advertise all hosted services
  MDNSAnswer* _getAnswerFromIdx(int idx);
  int _getNumAnswers();
  bool _listen();
  void _restart();
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_MDNS)
extern MDNSResponder MDNS;
#endif

#endif //ESP8266MDNS_H
