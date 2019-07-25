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

class MongooseHttpClientRequest
{
  friend MongooseHttpClient;

  private:
    MongooseHttpClient *_client;
    MongooseHttpResponseHandler _onResponse;

    const char *_uri;
    HttpRequestMethodComposite _method;
    const char *_contentType;
    int64_t _contentLength;
    const uint8_t *_body;

  public:
    MongooseHttpClientRequest(MongooseHttpClient *client, const char *uri);
    virtual ~MongooseHttpClientRequest() {
    }

    void setMethod(HttpRequestMethodComposite method) {
      _method = method;
    }
    void setContentType(const char *contentType) {
      _contentType = contentType;
    }
    void setContentLength(int64_t contentLength) {
      _contentLength = contentLength;
    }
    void setContent(const char *content) {
      setContent((uint8_t *)content, strlen(content));
    }
    void setContent(const uint8_t *content, size_t len);

    bool addHeader(const char *name, const char *value);
#ifdef ARDUINO
    bool addHeader(const String& name, const String& value) {
      return addHeader(name.c_str(), value.c_str());
    };
#endif

    void onResponse(MongooseHttpResponseHandler handler) {
      _onResponse = handler;
    }
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

    size_t contentLength() {
      return _msg->body.len;
    }
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

    void get(const char* uri, MongooseHttpResponseHandler onResponse);
    void post(const char* uri, const char *contentType, const char *body, MongooseHttpResponseHandler onResponse);

#ifdef ARDUINO
    void get(String &uri, MongooseHttpResponseHandler onResponse) {
      get(uri.c_str(), onResponse);
    }
    void post(String &uri, const char *contentType, const char *body, MongooseHttpResponseHandler onResponse) {
      post(uri.c_str(), contentType, body, onResponse);
    }
    void post(String& uri, String& contentType, const char *body, MongooseHttpResponseHandler onResponse) {
      post(uri.c_str(), contentType.c_str(), body, onResponse);
    }
    void post(String &uri, String& contentType, String& body, MongooseHttpResponseHandler onResponse) {
      post(uri.c_str(), contentType.c_str(), body.c_str(), onResponse);
    }
#endif // ARDUINO
};


#endif /* _MongooseHttpClient_H_ */
