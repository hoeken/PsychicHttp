#ifndef MongooseHttpServer_h
#define MongooseHttpServer_h

#ifdef ARDUINO
#include "Arduino.h"
#endif

#include <mongoose.h>

#include <functional>

class MongooseHttpServer;
class MongooseHttpServerRequest;
class MongooseHttpServerResponse;

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

    void redirect(const char *url);
#ifdef ARDUINO
    void redirect(const String& url);
#endif

    void send(int code, const char *contentType="", const char *content="");
#ifdef ARDUINO
    void send(int code, const String& contentType=String(), const String& content=String());
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
};


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
