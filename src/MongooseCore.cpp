
#include "MongooseCore.h"

#ifdef ARDUINO
#ifdef ESP32
#include <WiFi.h>
#ifdef ENABLE_WIRED_ETHERNET
#include <ETH.h>
#endif
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#endif // ARDUINO

MongooseCore::MongooseCore() : 
#if MG_ENABLE_SSL
  _rootCa(ARDUINO_MONGOOSE_DEFAULT_ROOT_CA),
  _rootCaCallback([this]() -> const char * { return _rootCa; }),
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

  ipConfigChanged();
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

void MongooseCore::getDefaultOpts(struct mg_connect_opts *opts, bool secure)
{
  memset(opts, 0, sizeof(*opts));

#if MG_ENABLE_SSL
  if(secure) {
    opts->ssl_ca_cert = _rootCaCallback();
  }
#else
  (void)secure;
#endif
}


void MongooseCore::ipConfigChanged() 
{
#ifdef ARDUINO
#if defined(ESP32) || defined(ESP8266)
  IPAddress dns = WiFi.dnsIP(0);
#if defined(ESP32) && defined(ENABLE_WIRED_ETHERNET)
  if(0 == dns) {
    dns = ETH.dnsIP(0);
  }
#endif
  _nameserver = dns.toString();
  mg_set_nameserver(&mgr, _nameserver.c_str());
#endif
#endif // ARDUINO
}

MongooseCore Mongoose;
