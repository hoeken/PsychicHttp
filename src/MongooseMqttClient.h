#ifndef MongooseMqttClient_h
#define MongooseMqttClient_h

#ifdef ARDUINO
#include "Arduino.h"
#endif

#include <mongoose.h>

#include <functional>

#include "MongooseString.h"

class MongooseMqttClient;

typedef std::function<void()> MongooseMqttConnectionHandler;
typedef std::function<void(const MongooseString topic, const MongooseString payload)> MongooseMqttMessageHandler;
typedef std::function<void(uint8_t retCode)> MongooseMqttErrorHandler;

class MongooseMqttClient
{
  private:
    const char *_username;
    const char *_password;
    struct mg_connection *_nc;
    bool _connected;

    MongooseMqttConnectionHandler _onConnect;
    MongooseMqttMessageHandler _onMessage;
    MongooseMqttErrorHandler _onError;

  protected:
    static void eventHandler(struct mg_connection *nc, int ev, void *p, void *u);
    void eventHandler(struct mg_connection *nc, int ev, void *p);

  public:
    MongooseMqttClient();
    ~MongooseMqttClient();

  bool connect(const char *server, MongooseMqttConnectionHandler onConnect) {
    return connect(server, NULL, NULL, onConnect);
  }
  bool connect(const char *server, const char *username, const char *password, MongooseMqttConnectionHandler onConnect);
  bool connected() {
    return _connected;
  }

  void onMessage(MongooseMqttMessageHandler fnHandler) {
    _onMessage = fnHandler;
  }
  void onError(MongooseMqttErrorHandler fnHandler) {
    _onError = fnHandler;
  }

  bool subscribe(const char *topic);

  bool publish(const char *topic, const char *payload) {
    return publish(topic, mg_mk_str(payload));
  }
  bool publish(const char *topic, MongooseString payload) {
    return publish(topic, payload.toMgStr());
  }
  bool publish(const char *topic, mg_str payload);
};

#endif /* MongooseMqttClient_h */
