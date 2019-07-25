
#include "MongooseCore.h"

#ifdef ARDUINO
#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#endif // ARDUINO

MongooseCore::MongooseCore() : 
#if MG_ENABLE_SSL
  _rootCa(ARDUINO_MONGOOSE_DEFAULT_ROOT_CA),
#endif
#ifdef ARDUINO
  _nameserver(""),
#endif
  mgr({0})
{
}

void MongooseCore::begin() 
{
  mg_mgr_init(&mgr, this);
}

void MongooseCore::end() 
{
  mg_mgr_free(&mgr);
}

void MongooseCore::poll(int timeout_ms) 
{
  mg_mgr_poll(&mgr, timeout_ms);
}

struct mg_mgr *MongooseCore::getMgr()
{
  return &mgr;
}

void MongooseCore::getDefaultOpts(struct mg_connect_opts *opts)
{
  memset(opts, 0, sizeof(*opts));

#ifdef ARDUINO
#if defined(ESP32) || defined(ESP8266)
  IPAddress dns = WiFi.dnsIP(0);
  _nameserver = dns.toString();
  opts->nameserver = _nameserver.c_str();
#endif
#endif // ARDUINO

#if MG_ENABLE_SSL
    opts->ssl_ca_cert = _rootCa;
#endif
}


MongooseCore Mongoose;
