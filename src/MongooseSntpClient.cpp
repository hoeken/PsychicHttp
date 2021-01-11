#if defined(ENABLE_DEBUG) && !defined(ENABLE_DEBUG_MONGOOSE_SNTP_CLIENT)
#undef ENABLE_DEBUG
#endif

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include <MicroDebug.h>

#include "MongooseCore.h"
#include "MongooseSntpClient.h"

#if MG_ENABLE_SNTP

MongooseSntpClient::MongooseSntpClient() :
  _nc(NULL),
  _onTime(NULL),
  _onError(NULL)
{

}

MongooseSntpClient::~MongooseSntpClient()
{

}

void MongooseSntpClient::eventHandler(struct mg_connection *nc, int ev, void *p, void *u)
{
  MongooseSntpClient *self = (MongooseSntpClient *)u;
  self->eventHandler(nc, ev, p);
}

void MongooseSntpClient::eventHandler(struct mg_connection *nc, int ev, void *p)
{
  struct mg_sntp_message *msg = (struct mg_sntp_message *) p;

  if (ev != MG_EV_POLL) { DBUGF("%s %p: %d", __PRETTY_FUNCTION__, nc, ev); }

  switch (ev) 
  {
    case MG_SNTP_REPLY:
      if(_onTime) {
        _onTime(msg->tv);
      }
      break;

    case MG_SNTP_FAILED:
      if(_onError) {
        _onError(-1);
      }
      break;

    case MG_EV_CLOSE: {
      DBUGF("Connection %p closed", nc);
      _nc = NULL;
      break;
    }
  }
}

bool MongooseSntpClient::getTime(const char *server, MongooseSntpTimeHandler onTime)
{
  if(NULL == _nc) 
  {
    DBUGF("Trying to connect to %s", server);
    _onTime = onTime;

    _nc = mg_sntp_get_time(Mongoose.getMgr(), eventHandler, server, this);
    if(_nc) {
      return true;
    }

    DBUGF("Failed to connect to %s", server);
  }

  return false;
}

#endif // MG_ENABLE_SNTP
