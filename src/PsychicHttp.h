#ifndef PsychicHttp_h
#define PsychicHttp_h

#include <ArduinoTrace.h>
#include <esp_http_server.h>
#include <http_status.h>
#include <string>

#define SCRATCH_BUFSIZE (10240)
//char scratch[SCRATCH_BUFSIZE];

class PsychicHttpServer;
class PsychicHttpServerRequest;
class PsychicHttpServerResponse;
#ifdef ARDUINO
  class PsychicHttpServerResponseStream;
#endif
class PsychicHttpWebSocketConnection;

class PsychicHttpServerRequest {
  friend PsychicHttpServer;

  protected:
    PsychicHttpServer *_server;
    http_method _method;
    httpd_req_t *_req;
    PsychicHttpServerResponse *_response;

    char * _header;
    size_t _header_len;
    String _body;
    char * _query;
    size_t _query_len;

    void loadBody();

  public:
    PsychicHttpServerRequest(PsychicHttpServer *server, httpd_req_t *req);
    virtual ~PsychicHttpServerRequest();

    virtual bool isUpload() { return false; }
    virtual bool isWebSocket() { return false; }

    http_method method();
    const char * methodStr();
    const char * uri();
    const char * queryString();
    const char * headers(const char *name);
    const char * header(const char *name);
    const char * host();
    const char * contentType();
    size_t contentLength();
    String body();
    void redirect(const char *url);

    bool hasParam(const char *key);
    esp_err_t getParam(const char *name, char *value);

    std::string getParam(const char *name);

    bool authenticate(const char * username, const char * password);
    void requestAuthentication(const char* realm);

    PsychicHttpServerResponse *beginResponse();
    #ifdef ARDUINO
      PsychicHttpServerResponseStream *beginResponseStream();
    #endif

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

  public:
    PsychicHttpServerResponse(httpd_req_t *request);
    virtual ~PsychicHttpServerResponse();

    void setCode(int code);

    void setContentType(const char *contentType);
    void setContentLength(int64_t contentLength) {
      _contentLength = contentLength;
    }

    void addHeader(const char *name, const char *value);

    void setContent(const char *content);
    void setContent(const uint8_t *content, size_t len);

    const char * getContent();
    size_t getContentLength();
};

#ifdef ARDUINO
  class PsychicHttpServerResponseStream:
    public PsychicHttpServerResponse,
    public Print
  {
    private:
      std::string _content;

    public:
      PsychicHttpServerResponseStream(httpd_req_t *request);
      virtual ~PsychicHttpServerResponseStream();

      size_t write(const uint8_t *data, size_t len);
      size_t write(uint8_t data);
  };
#endif

typedef std::function<esp_err_t(PsychicHttpServerRequest *request)> PsychicHttpRequestHandler;
//typedef std::function<size_t(PsychicHttpServerRequest *request, int ev, MongooseString filename, uint64_t index, uint8_t *data, size_t len)> PsychicHttpUploadHandler;
typedef std::function<void(PsychicHttpWebSocketConnection *connection)> PsychicHttpWebSocketConnectionHandler;
typedef std::function<void(PsychicHttpWebSocketConnection *connection, int flags, uint8_t *data, size_t len)> PsychicHttpWebSocketFrameHandler;

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
    PsychicHttpServerEndpoint(PsychicHttpServer *server, http_method method);
    PsychicHttpServerEndpoint *onRequest(PsychicHttpRequestHandler handler);
    // PsychicHttpServerEndpoint *onUpload(PsychicHttpUploadHandler handler);
    PsychicHttpServerEndpoint *onConnect(PsychicHttpWebSocketConnectionHandler handler);
    PsychicHttpServerEndpoint *onFrame(PsychicHttpWebSocketFrameHandler handler);
    PsychicHttpServerEndpoint *onClose(PsychicHttpRequestHandler handler);
    static esp_err_t endpointRequestHandler(httpd_req_t *req);
};

class PsychicHttpWebSocketConnection : public PsychicHttpServerRequest
{
  friend PsychicHttpServer;

  public:
    PsychicHttpWebSocketConnection(PsychicHttpServer *server, httpd_req_t *req);
    virtual ~PsychicHttpWebSocketConnection();

    virtual bool isWebSocket() { return true; }

    void send(int op, const void *data, size_t len);
    void send(const char *buf) {
      //send(WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }

    const char * getRemoteAddress() {
      //return &(_nc->rem);
      return "";
    }

    // const mg_connection *getConnection() {
    //   //return _nc;
    // }
};

class PsychicHttpServer
{
  protected:
    httpd_handle_t server;

    //TODO: is this necessary?
    //PsychicHttpServerEndpoint defaultEndpoint;

  public:
    PsychicHttpServer();
    ~PsychicHttpServer();

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

    void sendAll(PsychicHttpWebSocketConnection *from, const char *endpoint, int op, const void *data, size_t len);
    void sendAll(PsychicHttpWebSocketConnection *from, int op, const void *data, size_t len);
    void sendAll(int op, const void *data, size_t len);
    void sendAll(PsychicHttpWebSocketConnection *from, const char *buf);
    void sendAll(const char *buf);
    void sendAll(const char *endpoint, int op, const void *data, size_t len);
    void sendAll(PsychicHttpWebSocketConnection *from, const char *endpoint, const char *buf);
    void sendAll(const char *endpoint, const char *buf);
};

#endif /* PsychicHttp_h */