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
class MongooseHttpServerResponseBasic;
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
  friend MongooseHttpServer;

  private:
    MongooseHttpServer *_server;
    mg_connection *_nc;
    http_message *_msg;
    HttpRequestMethodComposite _method;
    MongooseHttpServerResponse *_response;

    void sendBody();

  public:
    MongooseHttpServerRequest(MongooseHttpServer *server, mg_connection *nc, http_message *msg);
    virtual ~MongooseHttpServerRequest();

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

    MongooseHttpServerResponseBasic *beginResponse();

#ifdef ARDUINO
    MongooseHttpServerResponseStream *beginResponseStream();
#endif

    // Takes ownership of `response`, will delete when finished. Do not use `response` after calling
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
#ifdef ARDUINO
    bool authenticate(const String& username, const String& password) {
      return authenticate(username.c_str(), password.c_str());
    }
#endif
    void requestAuthentication(const char* realm);
};

class MongooseHttpServerResponse
{
  private:
    int _code;
    const char *_contentType;
    int64_t _contentLength;

  public:
    MongooseHttpServerResponse();
    virtual ~MongooseHttpServerResponse() {
    }

    void setCode(int code) {
      _code = code;
    }
    void setContentType(const char *contentType) {
      _contentType = contentType;
    }
    void setContentLength(int64_t contentLength) {
      _contentLength = contentLength;
    }

    bool addHeader(const char *name, const char *value) const;
#ifdef ARDUINO
    bool addHeader(const String& name, const String& value) const;
#endif

    // send the to `nc`, return true if more to send
    virtual void sendHeaders(struct mg_connection *nc);

    // send (a part of) the body to `nc`, return < `bytes` if no more to send
    virtual size_t sendBody(struct mg_connection *nc, size_t bytes) = 0;
};

class MongooseHttpServerResponseBasic: 
  public MongooseHttpServerResponse
{
  private:
    const uint8_t *ptr;
    size_t len;

  public:
    MongooseHttpServerResponseBasic();
  
    void setContent(const char *content);
    void setContent(const uint8_t *content, size_t len);
    virtual size_t sendBody(struct mg_connection *nc, size_t bytes);
};

#ifdef ARDUINO
class MongooseHttpServerResponseStream: 
  public MongooseHttpServerResponse,
  public Print
{
  private:
    mbuf _content;

  public:
    MongooseHttpServerResponseStream();
    virtual ~MongooseHttpServerResponseStream();

    size_t write(const uint8_t *data, size_t len);
    size_t write(uint8_t data);
  //  using Print::write;

    virtual size_t sendBody(struct mg_connection *nc, size_t bytes);
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
