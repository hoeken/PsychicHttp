#ifndef MongooseHttpServer_h
#define MongooseHttpServer_h

#ifdef ARDUINO
#include "Arduino.h"
#endif

#include <mongoose.h>

#include <functional>

#include "MongooseString.h"

class MongooseHttpServer;
class MongooseHttpServerRequest;
class MongooseHttpServerResponse;
#ifdef ARDUINO
class MongooseHttpServerResponseStream;
#endif

typedef enum {
  HTTP_GET     = 0b00000001,
  HTTP_POST    = 0b00000010,
  HTTP_DELETE  = 0b00000100,
  HTTP_PUT     = 0b00001000,
  HTTP_PATCH   = 0b00010000,
  HTTP_HEAD    = 0b00100000,
  HTTP_OPTIONS = 0b01000000,
  HTTP_ANY     = 0b01111111,
} HttpRequestMethod;

typedef uint8_t HttpRequestMethodComposite;

class MongooseHttpServerRequest {
  private:
    MongooseHttpServer *_server;
    mg_connection *_nc;
    http_message *_msg;
    HttpRequestMethodComposite _method;

  public:
    MongooseHttpServerRequest(MongooseHttpServer *server, mg_connection *nc, http_message *msg);
    ~MongooseHttpServerRequest();

    HttpRequestMethodComposite method() {
      return _method;
    }

    MongooseString message() {
      return MongooseString(_msg->message);
    }
    MongooseString body() {
      return MongooseString(_msg->body);
    }

    MongooseString methodStr() {
      return MongooseString(_msg->method);
    }
    MongooseString uri() {
      return MongooseString(_msg->uri);
    }
    MongooseString proto() {
      return MongooseString(_msg->proto);
    }
    
    int respCode() {
      return _msg->resp_code;
    }
    MongooseString respStatusMsg() {
      return MongooseString(_msg->resp_status_msg);
    }

    MongooseString queryString() {
      return MongooseString(_msg->query_string);
    }

    int headers() {
      int i;
      for (i = 0; i < MG_MAX_HTTP_HEADERS && _msg->header_names[i].len > 0; i++) {
      }
      return i;
    }
    MongooseString headers(const char *name) {
      MongooseString ret(mg_get_http_header(_msg, name));
      return ret;
    }
    MongooseString headerNames(int i) {
      return MongooseString(_msg->header_names[i]);
    }
    MongooseString headerValues(int i) {
      return MongooseString(_msg->header_values[i]);
    }

    MongooseString host() {
      return headers("Host");
    }

    size_t contentLength() {
      return _msg->body.len;
    }

    void redirect(const char *url);
#ifdef ARDUINO
    void redirect(const String& url);
#endif

    MongooseHttpServerResponse *beginResponse(const char *contentType="");
    MongooseHttpServerResponse *beginResponse(int code, const char *contentType="", const char *content="");

#ifdef ARDUINO
    MongooseHttpServerResponse *beginResponse(const String& contentType="");
    MongooseHttpServerResponse *beginResponse(int code, const String& contentType=String(), const String& content=String());
    MongooseHttpServerResponseStream *beginResponseStream(const char *contentType="");
    MongooseHttpServerResponseStream *beginResponseStream(const String& contentType=String());
#endif

    void send(MongooseHttpServerResponse *response);
    void send(int code);
    void send(int code, const char *contentType, const char *content="");
#ifdef ARDUINO
    void send(int code, const String& contentType, const String& content=String());
#endif

    bool hasParam(const char *name) const;
#ifdef ARDUINO
    bool hasParam(const String& name) const;
    bool hasParam(const __FlashStringHelper * data) const;
#endif

    int getParam(const char *name, char *dst, size_t dst_len) const;
#ifdef ARDUINO
    int getParam(const String& name, char *dst, size_t dst_len) const;
    int getParam(const __FlashStringHelper * data, char *dst, size_t dst_len) const; 
#endif

#ifdef ARDUINO
    String getParam(const char *name) const;
    String getParam(const String& name) const;
    String getParam(const __FlashStringHelper * data) const; 
#endif

    bool authenticate(const char * username, const char * password);
    void requestAuthentication(const char* realm);
};

class MongooseHttpServerResponse
{
  public:
    virtual void setCode(int code);

    bool addHeader(const char *name, const char *value) const;
#ifdef ARDUINO
    bool addHeader(const String& name, const String& value) const;
#endif


};

#ifdef ARDUINO
class MongooseHttpServerResponseStream: 
  public MongooseHttpServerResponse,
  public Print
{
  public:

    size_t write(const uint8_t *data, size_t len);
    size_t write(uint8_t data);
  //  using Print::write;

};
#endif


typedef std::function<void(MongooseHttpServerRequest *request)> ArRequestHandlerFunction;

class MongooseHttpServer 
{
  protected:
    struct mg_connection *nc;
    ArRequestHandlerFunction fnNotFound;

    static void defaultEventHandler(struct mg_connection *nc, int ev, void *p, void *u);
    static void endpointEventHandler(struct mg_connection *nc, int ev, void *p, void *u);
    void eventHandler(struct mg_connection *nc, int ev, void *p, HttpRequestMethodComposite method, ArRequestHandlerFunction onRequest); 

  public:
    MongooseHttpServer();
    ~MongooseHttpServer();

    bool begin(uint16_t port);

#if MG_ENABLE_SSL
    bool begin(uint16_t port, const char *cert, const char *private_key);
#endif

    void on(const char* uri, ArRequestHandlerFunction onRequest);
    void on(const char* uri, HttpRequestMethodComposite method, ArRequestHandlerFunction onRequest);

    void onNotFound(ArRequestHandlerFunction fn);

    void reset();
};

#endif /* _MongooseHttpServer_H_ */
