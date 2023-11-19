#ifndef PsychicHTTP_h
#define PsychicHTTP_h

#include <esp_http_server.h>
#include <http_status.h>
//#include <list>

class PsychicHTTPServer;
class PsychicHTTPServerRequest;
class PsychicHTTPServerResponse;
class PsychicHTTPServerResponseStream;

class PsychicHTTPWebSocketConnection;

class PsychicHTTPServerRequest {
  friend PsychicHTTPServer;

  protected:
    PsychicHTTPServer *_server;
    mg_connection *_nc;
    mg_http_message *_msg;
    HttpRequestMethodComposite _method;
    PsychicHTTPServerResponse *_response;
    bool _responseSent;

    #if MG_COPY_HTTP_MESSAGE
        mg_http_message *duplicateMessage(mg_http_message *);
    #endif

  public:
    PsychicHTTPServerRequest(PsychicHTTPServer *server, mg_connection *nc, mg_http_message *msg);
    virtual ~PsychicHTTPServerRequest();

    virtual bool isUpload() { return false; }
    virtual bool isWebSocket() { return false; }

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

    MongooseString queryString() {
      return MongooseString(_msg->query);
    }

    MongooseString proto() {
      return MongooseString(_msg->proto);
    }

    //TODO: verify this
    int respCode() {
      return mg_http_status(_msg);
    }

    //TODO: verify this
    MongooseString respStatusMsg() {
      return MongooseString(_msg->message);
    }

    //TODO: not sure this is needed
    // int headers() {
    //   int i;
    //   for (i = 0; i < MG_MAX_HTTP_HEADERS && _msg->headers[i].len > 0; i++) {
    //   }
    //   return i;
    // }

    MongooseString headers(const char *name) {
      MongooseString ret(mg_http_get_header(_msg, name));
      return ret;
    }

    //TODO: not sure this is possible
    // MongooseString headerNames(int i) {
    //   return MongooseString(_msg->header_names[i]);
    // }
    // MongooseString headerValues(int i) {
    //   return MongooseString(_msg->header_values[i]);
    // }

    MongooseString host() {
      return headers("Host");
    }

    MongooseString contentType() {
      return headers("Content-Type");
    }

    size_t contentLength() {
      return _msg->body.len;
    }

    void redirect(const char *url);

    PsychicHTTPServerResponse *beginResponse();


    // Takes ownership of `response`, will delete when finished. Do not use `response` after calling
    void send(PsychicHTTPServerResponse *response);
    bool responseSent() {
      return NULL != _response;
    }

    void send(int code);
    void send(int code, const char *contentType, const char *content="");
    bool hasParam(const char *name) const;
    int getParam(const char *name, char *dst, size_t dst_len) const;
    bool authenticate(const char * username, const char * password);
    void requestAuthentication(const char* realm);
};

class PsychicHTTPServerResponse
{
  protected:
    int64_t _contentLength;
    int _code;
    std::list<mg_http_header> headers;
    //mg_str body;
    const char * body;

  public:
    PsychicHTTPServerResponse();
    virtual ~PsychicHTTPServerResponse();

    void setCode(int code) {
      _code = code;
    }
    void setContentType(const char *contentType);
    void setContentLength(int64_t contentLength) {
      _contentLength = contentLength;
    }

    void addHeader(const char *name, const char *value);

    const char * getHeaderString();

    void setContent(const char *content);
    void setContent(const uint8_t *content, size_t len);
    void setContent(MongooseString &content) {
      setContent((const uint8_t *)content.c_str(), content.length());
    }

    virtual void send(struct mg_connection *nc);

    // // send the to `nc`, return true if more to send
    // virtual void sendHeaders(struct mg_connection *nc);

    // // send (a part of) the body to `nc`, return < `bytes` if no more to send
    // virtual size_t sendBody(struct mg_connection *nc, size_t bytes) = 0;
};

#ifdef ARDUINO
  class PsychicHTTPServerResponseStream:
    public PsychicHTTPServerResponse,
    public Print
  {
    private:
      mg_iobuf _content;

    public:
      PsychicHTTPServerResponseStream();
      virtual ~PsychicHTTPServerResponseStream();

      size_t write(const uint8_t *data, size_t len);
      size_t write(uint8_t data);

      virtual void send(struct mg_connection *nc);
  };
#endif

typedef std::function<void(PsychicHTTPServerRequest *request)> PsychicHTTPRequestHandler;
typedef std::function<size_t(PsychicHTTPServerRequest *request, int ev, MongooseString filename, uint64_t index, uint8_t *data, size_t len)> PsychicHTTPUploadHandler;
typedef std::function<void(PsychicHTTPWebSocketConnection *connection)> PsychicHTTPWebSocketConnectionHandler;
typedef std::function<void(PsychicHTTPWebSocketConnection *connection, int flags, uint8_t *data, size_t len)> PsychicHTTPWebSocketFrameHandler;

class PsychicHTTPServerEndpoint
{
  friend PsychicHTTPServer;

  private:
    PsychicHTTPServer *server;
    MongooseString *uri;
    HttpRequestMethodComposite method;
    PsychicHTTPRequestHandler request;
    PsychicHTTPUploadHandler upload;
    PsychicHTTPRequestHandler close;
    PsychicHTTPWebSocketConnectionHandler wsConnect;
    PsychicHTTPWebSocketFrameHandler wsFrame;
  public:
    PsychicHTTPServerEndpoint(PsychicHTTPServer *server, HttpRequestMethodComposite method) :
      server(server),
      method(method),
      request(NULL),
      upload(NULL),
      close(NULL),
      wsConnect(NULL),
      wsFrame(NULL)
    {
    }

    PsychicHTTPServerEndpoint *onRequest(PsychicHTTPRequestHandler handler) {
      this->request = handler;
      return this;
    }

    PsychicHTTPServerEndpoint *onUpload(PsychicHTTPUploadHandler handler) {
      this->upload = handler;
      return this;
    }

    PsychicHTTPServerEndpoint *onClose(PsychicHTTPRequestHandler handler) {
      this->close = handler;
      return this;
    }

    PsychicHTTPServerEndpoint *onConnect(PsychicHTTPWebSocketConnectionHandler handler) {
      this->wsConnect = handler;
      return this;
    }

    PsychicHTTPServerEndpoint *onFrame(PsychicHTTPWebSocketFrameHandler handler) {
      this->wsFrame = handler;
      return this;
    }
};

class PsychicHTTPWebSocketConnection : public PsychicHTTPServerRequest
{
  friend PsychicHTTPServer;

  public:
    PsychicHTTPWebSocketConnection(PsychicHTTPServer *server, mg_connection *nc, mg_http_message *msg);
    virtual ~PsychicHTTPWebSocketConnection();

    virtual bool isWebSocket() { return true; }

    void send(int op, const void *data, size_t len);
    void send(const char *buf) {
      send(WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }

    const mg_addr *getRemoteAddress() {
      return &(_nc->rem);
    }
    const mg_connection *getConnection() {
      return _nc;
    }
};

class PsychicHTTPServer
{
  protected:
    struct mg_connection *nc;
    PsychicHTTPServerEndpoint defaultEndpoint;
    std::list<PsychicHTTPServerEndpoint*> endpoints;

    //static void defaultEventHandler(struct mg_connection *nc, int ev, void *p, void *u);
    //static void endpointEventHandler(struct mg_connection *nc, int ev, void *p, void *u);
    static void eventHandler(struct mg_connection *c, int ev, void *ev_data, void *fn_data);

  public:
    PsychicHTTPServer();
    ~PsychicHTTPServer();

    void begin(uint16_t port);
    void begin(uint16_t port, const char *cert, const char *private_key);

    void poll(int timeout_ms);

    PsychicHTTPServerEndpoint *on(const char* uri);
    PsychicHTTPServerEndpoint *on(const char* uri, HttpRequestMethodComposite method);
    PsychicHTTPServerEndpoint *on(const char* uri, PsychicHTTPRequestHandler onRequest);
    PsychicHTTPServerEndpoint *on(const char* uri, HttpRequestMethodComposite method, PsychicHTTPRequestHandler onRequest);

    void onNotFound(PsychicHTTPRequestHandler fn);

    void reset();

    void sendAll(PsychicHTTPWebSocketConnection *from, const char *endpoint, int op, const void *data, size_t len);

    void sendAll(PsychicHTTPWebSocketConnection *from, int op, const void *data, size_t len) {
      sendAll(from, NULL, op, data, len);
    }
    void sendAll(int op, const void *data, size_t len) {
      sendAll(NULL, NULL, op, data, len);
    }
    void sendAll(PsychicHTTPWebSocketConnection *from, const char *buf) {
      sendAll(from, NULL, WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }
    void sendAll(const char *buf) {
      sendAll(NULL, NULL, WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }
    void sendAll(const char *endpoint, int op, const void *data, size_t len) {
      sendAll(NULL, endpoint, op, data, len);
    }
    void sendAll(PsychicHTTPWebSocketConnection *from, const char *endpoint, const char *buf) {
      sendAll(from, endpoint, WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }
    void sendAll(const char *endpoint, const char *buf) {
      sendAll(NULL, endpoint, WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }
};

#endif /* PsychicHTTP_h */