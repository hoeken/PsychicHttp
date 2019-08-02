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

typedef enum {
  MQTT_MQTT,
  MQTT_MQTTS,
  MQTT_WS,  // TODO
  MQTT_WSS  // TODO
} MongooseMqttProtocol;

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
    return connect(MQTT_MQTT, server, NULL, NULL, onConnect);
  }
  bool connect(MongooseMqttProtocol protocol, const char *server, MongooseMqttConnectionHandler onConnect) {
    return connect(protocol, server, NULL, NULL, onConnect);
  }
  bool connect(const char *server, const char *username, const char *password, MongooseMqttConnectionHandler onConnect) {
    return connect(MQTT_MQTT, server, username, password, onConnect);
  }
  bool connect(MongooseMqttProtocol protocol, const char *server, const char *username, const char *password, MongooseMqttConnectionHandler onConnect);

#ifdef ARDUINO
  bool connect(String &server, MongooseMqttConnectionHandler onConnect) {
    return connect(MQTT_MQTT, server.c_str(), NULL, NULL, onConnect);
  }
  bool connect(MongooseMqttProtocol protocol, String &server, MongooseMqttConnectionHandler onConnect) {
    return connect(protocol, server.c_str(), NULL, NULL, onConnect);
  }
  bool connect(String &server, String &username, String &password, MongooseMqttConnectionHandler onConnect) {
    return connect(MQTT_MQTT, server.c_str(), username.c_str(), password.c_str(), onConnect);
  }
  bool connect(MongooseMqttProtocol protocol, String &server, String &username, String &password, MongooseMqttConnectionHandler onConnect) {
    return connect(protocol, server.c_str(), username.c_str(), password.c_str(), onConnect);
  }
#endif

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
#ifdef ARDUINO
  bool subscribe(String &topic) {
    return subscribe(topic.c_str());
  }
#endif

  bool publish(const char *topic, const char *payload) {
    return publish(topic, mg_mk_str(payload));
  }
  bool publish(const char *topic, MongooseString payload) {
    return publish(topic, payload.toMgStr());
  }
  bool publish(const char *topic, mg_str payload);
#ifdef ARDUINO
  bool publish(String &topic, const char *payload) {
    return publish(topic.c_str(), mg_mk_str(payload));
  }
  bool publish(String &topic, String &payload) {
    return publish(topic.c_str(), mg_mk_str(payload.c_str()));
  }
  bool publish(const char *topic, String &payload) {
    return publish(topic, mg_mk_str(payload.c_str()));
  }
#endif
};

#endif /* MongooseMqttClient_h */
