#ifndef MongooseCore_h
#define MongooseCore_h

#ifdef ARDUINO
  #include "Arduino.h"
  #include <IPAddress.h>
  #ifdef ESP32
    #include <WiFi.h>
    #ifdef ENABLE_WIRED_ETHERNET
      #include <ETH.h>
    #endif
  #elif defined(ESP8266)
    #include <ESP8266WiFi.h>
  #endif
#endif // ARDUINO

#if defined(ARDUINO) && !defined(CS_PLATFORM)
  #ifdef ESP32
    #define CS_PLATFORM CS_P_ESP32
    #define START_ESP_WIFI
  #elif defined(ESP8266)
    #define CS_PLATFORM CS_P_ESP8266
    #define MG_ESP8266
    #undef LWIP_COMPAT_SOCKETS
    #define LWIP_COMPAT_SOCKETS 0
  #else
    #error Platform not supported
  #endif
#endif

#define MG_ENABLE_CALLBACK_USERDATA 1

#include "mongoose.h"
#include <functional>
#include "MongooseString.h"
#include "MongooseHTTP.h"
#include <MicroDebug.h>

#ifndef ARDUINO_MONGOOSE_DEFAULT_ROOT_CA
#define ARDUINO_MONGOOSE_DEFAULT_ROOT_CA ""
#endif

typedef std::function<const char *(void)> ArduinoMongooseGetRootCaCallback;

class MongooseCore
{
  private:
#if MG_ENABLE_SSL
    const char *_rootCa;
    ArduinoMongooseGetRootCaCallback _rootCaCallback;
#endif
#ifdef ARDUINO
    String _nameserver;
#endif // ARDUINO
    struct mg_mgr mgr;

  public:
    MongooseCore();
    void begin();
    void end();
    void poll(int timeout_ms);

    struct mg_mgr *getMgr();
    void getDefaultOpts(struct mg_connect_opts *opts, bool secure = false);

    void ipConfigChanged();

#if MG_ENABLE_SSL
    void setRootCa(const char *rootCa) {
      _rootCa = rootCa;
    }

    void setRootCaCallback(ArduinoMongooseGetRootCaCallback callback) {
      _rootCaCallback = callback;
    }
#endif

};

extern MongooseCore Mongoose;

#endif // MongooseCore_h
