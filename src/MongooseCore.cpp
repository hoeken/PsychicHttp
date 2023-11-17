
#include "MongooseCore.h"

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
  //TODO: how to handle this?
  //mgr.user_data = this;
  mg_mgr_init(&mgr);
  mg_log_set(MG_LL_DEBUG);  // Set debug log level. Default is MG_LL_INFO

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

//TODO: is this function really necessary?
void MongooseCore::getDefaultOpts(struct mg_connect_opts *opts, bool secure)
{
  //TODO: is this necessary?
  //memset(opts, 0, sizeof(*opts));

  #if MG_ENABLE_SSL
    if(secure) {
      opts->ssl_ca_cert = _rootCaCallback();
    }
  #else
    (void)secure;
  #endif
}

//TODO: is this one necessary?
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
      //TODO: is this necessary?
      //mg_set_nameserver(&mgr, _nameserver.c_str());
    #endif
  #endif // ARDUINO
}

MongooseCore Mongoose;