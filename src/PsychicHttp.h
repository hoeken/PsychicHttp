#ifndef PsychicHttp_h
#define PsychicHttp_h

#include <ArduinoTrace.h>
#include <esp_event.h>
#include <esp_http_server.h>
#include <http_status.h>
#include <string>
#include <map>
#include <list>
#include <libb64/cencode.h>
#include "esp_random.h"
#include "MD5Builder.h"
#include <UrlEncode.h>

typedef std::map<String, String> SessionData;

enum HTTPAuthMethod { BASIC_AUTH, DIGEST_AUTH };

#define MAX_COOKIE_SIZE 256
#define TAG "PsychicHttp"

class PsychicHttpServer;
class PsychicHttpServerRequest;
class PsychicHttpServerResponse;
class PsychicHttpWebSocketConnection;

class PsychicHttpServerRequest {
  friend PsychicHttpServer;

  protected:
    PsychicHttpServer *_server;
    http_method _method;
    httpd_req_t *_req;
    PsychicHttpServerResponse *_response;
    String _body;

    SessionData *_session;

    void loadBody();

  public:
    PsychicHttpServerRequest(PsychicHttpServer *server, httpd_req_t *req);
    virtual ~PsychicHttpServerRequest();

    virtual bool isUpload() { return false; }
    virtual bool isWebSocket() { return false; }

    static void freeSession(void *ctx);
    bool hasSessionKey(String key);
    String getSessionKey(String key);
    void setSessionKey(String key, String value);

    bool hasCookie(const char * key);
    String getCookie(const char * key);

    http_method method();
    String methodStr();
    String uri();
    String queryString();
    String headers(const char *name);
    String header(const char *name);
    bool hasHeader(const char *name);
    String host();
    String contentType();
    size_t contentLength();
    String body();
    void redirect(const char *url);

    bool hasParam(const char *key);
    esp_err_t getParam(const char *name, char *value);
    String getParam(const char *name);

    String _extractParam(String& authReq, const String& param, const char delimit);
    String _getRandomHexString();
    bool authenticate(const char * username, const char * password);
    void requestAuthentication(HTTPAuthMethod mode, const char* realm, const String& authFailMsg);

    PsychicHttpServerResponse *beginResponse();

    // Takes ownership of `response`, will delete when finished. Do not use `response` after calling
    void send(PsychicHttpServerResponse *response);
    void send(int code);
    void send(const char *content);
    void send(int code, const char *contentType, const char *content="");
};

class PsychicHttpServerResponse
{
  protected:
    httpd_req_t *_req;
    int64_t _contentLength;
    char _status[60];
    const char * body;

    std::list<char *> cookies;

  public:
    PsychicHttpServerResponse(httpd_req_t *request);
    virtual ~PsychicHttpServerResponse();

    void setCode(int code);

    void setContentType(const char *contentType);
    void setContentLength(int64_t contentLength) {
      _contentLength = contentLength;
    }

    void addHeader(const char *name, const char *value);
    char * setCookie(const char * key, const char * value, unsigned long max_age = 60*60*24*30);
    void freeCookies();

    void setContent(const char *content);
    void setContent(const uint8_t *content, size_t len);

    const char * getContent();
    size_t getContentLength();
};

typedef std::function<esp_err_t(PsychicHttpServerRequest *request)> PsychicHttpRequestHandler;
//typedef std::function<size_t(PsychicHttpServerRequest *request, int ev, MongooseString filename, uint64_t index, uint8_t *data, size_t len)> PsychicHttpUploadHandler;
typedef std::function<esp_err_t(PsychicHttpWebSocketConnection *connection)> PsychicHttpWebSocketConnectionHandler;
typedef std::function<esp_err_t(PsychicHttpWebSocketConnection *connection, httpd_ws_frame *frame)> PsychicHttpWebSocketFrameHandler;

class PsychicHttpServerEndpoint
{
  friend PsychicHttpServer;

  private:
    PsychicHttpServer *server;
    std::string uri;
    http_method method;
    PsychicHttpRequestHandler request;
    //PsychicHttpUploadHandler upload;
    PsychicHttpRequestHandler close;
    PsychicHttpWebSocketConnectionHandler wsConnect;
    PsychicHttpWebSocketFrameHandler wsFrame;

  public:
    PsychicHttpServerEndpoint();
    PsychicHttpServerEndpoint(PsychicHttpServer *server, http_method method);
    PsychicHttpServerEndpoint *onRequest(PsychicHttpRequestHandler handler);
    // PsychicHttpServerEndpoint *onUpload(PsychicHttpUploadHandler handler);
    PsychicHttpServerEndpoint *onConnect(PsychicHttpWebSocketConnectionHandler handler);
    PsychicHttpServerEndpoint *onFrame(PsychicHttpWebSocketFrameHandler handler);
    PsychicHttpServerEndpoint *onClose(PsychicHttpRequestHandler handler);

    static esp_err_t requestHandler(httpd_req_t *req);
    static void closeHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static esp_err_t websocketHandler(httpd_req_t *req);
    static esp_err_t notFoundHandler(httpd_req_t *req, httpd_err_code_t err);
};

class PsychicHttpWebSocketConnection : public PsychicHttpServerRequest
{
  friend PsychicHttpServer;

  protected:
    httpd_handle_t _server;
    int _fd;
    httpd_ws_frame_t * _packet;

  public:
    PsychicHttpWebSocketConnection(PsychicHttpServer *server, httpd_req_t *req);
    virtual ~PsychicHttpWebSocketConnection();

    virtual bool isWebSocket() { return true; }

    esp_err_t send(httpd_ws_frame_t * ws_pkt);
    esp_err_t send(httpd_ws_type_t op, const void *data, size_t len);
    esp_err_t send(const char *buf);

    //static void ws_async_send(void *arg);

    int getConnection() {
      return this->_fd;
    }
};

class PsychicHttpServer
{
  protected:
    httpd_handle_t server;

  public:
    PsychicHttpServer();
    ~PsychicHttpServer();

    PsychicHttpServerEndpoint defaultEndpoint;
    httpd_config_t config;

    static void destroy(void *ctx);

    bool begin(uint16_t port);
    bool begin(uint16_t port, const char *cert, const char *private_key);
    void stop();

    PsychicHttpServerEndpoint *on(const char* uri);
    PsychicHttpServerEndpoint *on(const char* uri, http_method method);
    PsychicHttpServerEndpoint *on(const char* uri, PsychicHttpRequestHandler onRequest);
    PsychicHttpServerEndpoint *on(const char* uri, http_method method, PsychicHttpRequestHandler onRequest);
    void onNotFound(PsychicHttpRequestHandler fn);

    PsychicHttpServerEndpoint *websocket(const char* uri);

    void sendAll(httpd_ws_frame_t * ws_pkt);
    void sendAll(httpd_ws_type_t op, const void *data, size_t len);
    void sendAll(const char *buf);

    //static void sendAsync(void *arg);
};

#endif /* PsychicHttp_h */