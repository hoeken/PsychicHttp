#ifdef ARDUINO
#include <Arduino.h>
#endif

#include <MicroDebug.h>

#include "MongooseCore.h"
#include "MongooseMqttClient.h"

MongooseMqttClient::MongooseMqttClient() :
  _client_id(NULL),
  _username(NULL),
  _password(NULL),
  _will_topic(NULL),
  _will_message(NULL),
  _will_retain(false),
  _nc(NULL),
  _connected(false),
  _reject_unauthorized(true),
  _onConnect(NULL),
  _onMessage(NULL),
  _onError(NULL)
{

}

MongooseMqttClient::~MongooseMqttClient()
{

}

void MongooseMqttClient::eventHandler(struct mg_connection *nc, int ev, void *p, void *u)
{
  MongooseMqttClient *self = (MongooseMqttClient *)u;
  self->eventHandler(nc, ev, p);
}

void MongooseMqttClient::eventHandler(struct mg_connection *nc, int ev, void *p)
{
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;

  if (ev != MG_EV_POLL) { DBUGF("%s %p: %d", __PRETTY_FUNCTION__, nc, ev); }

  switch (ev) 
  {
    case MG_EV_CONNECT: {
      int err = *((int *)p);
      if(0 == err)
      {
        struct mg_send_mqtt_handshake_opts opts;
        memset(&opts, 0, sizeof(opts));
        opts.user_name = _username;
        opts.password = _password;
        opts.will_topic = _will_topic;
        opts.will_message = _will_message;
        
        if(_will_retain) {
          opts.flags |= MG_MQTT_WILL_RETAIN;
        }
        
        DBUGVAR(_client_id);
        DBUGVAR(opts.user_name);
        DBUGVAR(opts.password);
        DBUGVAR(opts.will_topic);
        DBUGVAR(opts.will_message);
        DBUGVAR(opts.flags);

        mg_set_protocol_mqtt(nc);
        mg_send_mqtt_handshake_opt(nc, _client_id, opts);
      } else {
        DBUGVAR(err);
        _onError(err);
      }
      break;
    }

    case MG_EV_MQTT_CONNACK:
      if (MG_EV_MQTT_CONNACK_ACCEPTED == msg->connack_ret_code) {
        _connected = true;
        if(_onConnect) {
          _onConnect();
        }
      } else {
        DBUGF("Got mqtt connection error: %d", msg->connack_ret_code);
        if(_onError) {
          _onError(msg->connack_ret_code);
          _nc = NULL;
        }
      }
      break;

    case MG_EV_MQTT_PUBACK:
      DBUGF("Message publishing acknowledged (msg_id: %d)", msg->message_id);
      break;

    case MG_EV_MQTT_SUBACK:
      DBUGF("Subscription acknowledged");
      break;

    case MG_EV_MQTT_PUBLISH: {
      DBUGF("Got incoming message %.*s: %.*s", (int) msg->topic.len,
             msg->topic.p, (int) msg->payload.len, msg->payload.p);
      if(_onMessage) {
        _onMessage(MongooseString(msg->topic), MongooseString(msg->payload));
      }
      break;
    }

    case MG_EV_CLOSE: {
      DBUGF("Connection %p closed", nc);
      _nc = NULL;
      _connected = false;
      break;
    }
  }
}

bool MongooseMqttClient::connect(MongooseMqttProtocol protocol, const char *server, const char *client_id, MongooseMqttConnectionHandler onConnect)
{
  if(NULL == _nc) 
  {
    struct mg_connect_opts opts;
    bool secure = false;

#if MG_ENABLE_SSL
    if(MQTT_MQTTS == protocol || MQTT_WSS == protocol) {
      secure = true;
    }
#endif

    Mongoose.getDefaultOpts(&opts, secure);
#if MG_ENABLE_SSL
    if(!_reject_unauthorized) {
      opts.ssl_ca_cert = "*";
    }
#endif

    const char *err;
    opts.error_string = &err;

    DBUGF("Trying to connect to %s", server);
    _onConnect = onConnect;
    _client_id = client_id;
    _nc = mg_connect_opt(Mongoose.getMgr(), server, eventHandler, this, opts);
    if(_nc) {
      return true;
    }

    DBUGF("Failed to connect to %s: %s", server, err);
  }
  return false;
}

bool MongooseMqttClient::subscribe(const char *topic)
{
  if(connected())
  {
    struct mg_mqtt_topic_expression s_topic_expr = {NULL, 0};
    s_topic_expr.topic = topic;
    DBUGF("Subscribing to '%s'", topic);
    mg_mqtt_subscribe(_nc, &s_topic_expr, 1, 42);
    return true;
  }
  return false;
}

bool MongooseMqttClient::publish(const char *topic, mg_str payload, bool retain, int qos)
{
  int flags = qos;
  if(retain) {
    flags |= MG_MQTT_RETAIN;
  }
  
  if(connected()) {
    mg_mqtt_publish(_nc, topic, 65, flags, payload.p, payload.len);
    return true;
  }

  return false;
}

bool MongooseMqttClient::disconnect()
{
  if(connected()) {
    mg_mqtt_disconnect(_nc);
    _nc->flags |= MG_F_SEND_AND_CLOSE;
    return true;
  }

  return false;
}
