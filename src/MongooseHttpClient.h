#ifndef MongooseHttpClient_h
#define MongooseHttpClient_h

#ifdef ARDUINO
#include "Arduino.h"
#endif

#include <mongoose.h>

#include <functional>

#include "MongooseString.h"
#include "MongooseHttp.h"

class MongooseHttpClient;
class MongooseHttpClientRequest;
class MongooseHttpClientResponse;

typedef std::function<void(MongooseHttpClientResponse *request)> MongooseHttpResponseHandler;
typedef std::function<void()> MongooseHttpCloseHandler;

class MongooseHttpClientRequest
{
  friend MongooseHttpClient;

  private:
    MongooseHttpClient *_client;
    MongooseHttpResponseHandler _onResponse;
    MongooseHttpResponseHandler _onBody;
    MongooseHttpCloseHandler _onClose;

    const char *_uri;
    HttpRequestMethodComposite _method;
    const char *_contentType;
    int64_t _contentLength;
    const uint8_t *_body;
    char *_extraHeaders;

    mg_connection *_nc;

  public:
    MongooseHttpClientRequest(MongooseHttpClient *client, const char *uri);
    virtual ~MongooseHttpClientRequest();

    MongooseHttpClientRequest *setMethod(HttpRequestMethodComposite method) {
      _method = method;
      return this;
    }
    MongooseHttpClientRequest *setContentType(const char *contentType) {
      _contentType = contentType;
      return this;
    }
    MongooseHttpClientRequest *setContentLength(int64_t contentLength) {
      _contentLength = contentLength;
      return this;
    }
    MongooseHttpClientRequest *setContent(const char *content) {
      setContent((uint8_t *)content, strlen(content));
      return this;
    }
    MongooseHttpClientRequest *setContent(const uint8_t *content, size_t len);

    bool addHeader(const char *name, size_t nameLength, const char *value, size_t valueLength);
    bool addHeader(const char *name, const char *value) {
      return addHeader(name, strlen(name), value, strlen(value));
    }
    bool addHeader(MongooseString name, MongooseString value) {
      return addHeader(name.c_str(), name.length(), value.c_str(), value.length());
    }
#ifdef ARDUINO
    bool addHeader(const String& name, const String& value) {
      return addHeader(name.c_str(), name.length(), value.c_str(), value.length());
    };
#endif

    MongooseHttpClientRequest *onResponse(MongooseHttpResponseHandler handler) {
      _onResponse = handler;
      return this;
    }

    MongooseHttpClientRequest *onBody(MongooseHttpResponseHandler handler) {
      _onBody = handler;
      return this;
    }

    // Request may be NULL if the socket fails to connect
    MongooseHttpClientRequest *onClose(MongooseHttpCloseHandler handler) {
      _onClose = handler;
      return this;
    }

    void abort();
};

class MongooseHttpClientResponse {
  protected:
    http_message *_msg;

  public:
    MongooseHttpClientResponse(http_message *msg) :
      _msg(msg)
    {
    }

    ~MongooseHttpClientResponse() {
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

    MongooseString contentType() {
      return headers("Content-Type");
    }

    size_t contentLength();
};

class MongooseHttpClient
{
  private:

  protected:
    static void eventHandler(struct mg_connection *nc, int ev, void *p, void *u);
    void eventHandler(struct mg_connection *nc, MongooseHttpClientRequest *request, int ev, void *p);

  public:
    MongooseHttpClient();
    ~MongooseHttpClient();

    MongooseHttpClientRequest *beginRequest(const char *uri);
    void send(MongooseHttpClientRequest *request);

    void get(const char* uri, MongooseHttpResponseHandler onResponse = NULL, MongooseHttpCloseHandler onClose = NULL);
    void post(const char* uri, const char *contentType, const char *body, MongooseHttpResponseHandler onResponse = NULL, MongooseHttpCloseHandler onClose = NULL);

#ifdef ARDUINO
    void get(String &uri, MongooseHttpResponseHandler onResponse = NULL, MongooseHttpCloseHandler onClose = NULL) {
      get(uri.c_str(), onResponse, onClose);
    }
    void post(String &uri, const char *contentType, const char *body, MongooseHttpResponseHandler onResponse = NULL, MongooseHttpCloseHandler onClose = NULL) {
      post(uri.c_str(), contentType, body, onResponse, onClose);
    }
    void post(String& uri, String& contentType, const char *body, MongooseHttpResponseHandler onResponse = NULL, MongooseHttpCloseHandler onClose = NULL) {
      post(uri.c_str(), contentType.c_str(), body, onResponse, onClose);
    }
    void post(String &uri, String& contentType, String& body, MongooseHttpResponseHandler onResponse = NULL, MongooseHttpCloseHandler onClose = NULL) {
      post(uri.c_str(), contentType.c_str(), body.c_str(), onResponse, onClose);
    }
#endif // ARDUINO
};


#endif /* _MongooseHttpClient_H_ */
