#ifndef MongooseSntpClient_h
#define MongooseSntpClient_h

#ifdef ARDUINO
#include "Arduino.h"
#endif

#include <mongoose.h>

#if MG_ENABLE_SNTP

#include <functional>

#include "MongooseString.h"

class MongooseSntpClient;

typedef std::function<void(struct timeval time)> MongooseSntpTimeHandler;
typedef std::function<void(uint8_t retCode)> MongooseSntpErrorHandler;

class MongooseSntpClient
{
  private:
    struct mg_connection *_nc;

    MongooseSntpTimeHandler _onTime;
    MongooseSntpErrorHandler _onError;

  protected:
    static void eventHandler(struct mg_connection *nc, int ev, void *p, void *u);
    void eventHandler(struct mg_connection *nc, int ev, void *p);

  public:
    MongooseSntpClient();
    ~MongooseSntpClient();

  bool getTime(const char *server, MongooseSntpTimeHandler onTime);

#ifdef ARDUINO
  bool getTime(String &server, MongooseSntpTimeHandler onTime) {
    return getTime(server.c_str(), onTime);
  }
#endif

  void onError(MongooseSntpErrorHandler fnHandler) {
    _onError = fnHandler;
  }
};

#endif // MG_ENABLE_SNTP

#endif /* MongooseSntpClient_h */
