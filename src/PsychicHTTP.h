#ifndef PsychicHTTP_h
#define PsychicHTTP_h

#include <ArduinoTrace.h>
#include <esp_http_server.h>
#include <http_status.h>
#include <list>
#include <string>

class PsychicHTTPServer;
class PsychicHTTPServerRequest;
class PsychicHTTPServerResponse;
//class PsychicHTTPServerResponseStream;
class PsychicHTTPWebSocketConnection;

class PsychicHTTPServerRequest {
  friend PsychicHTTPServer;

  protected:
    PsychicHTTPServer *_server;
    httpd_method_t _method;
    httpd_req_t *_req;
    PsychicHTTPServerResponse *_response;

    char * _header;
    size_t _header_len;
    char * _body;
    size_t _body_len;
    char * _query;
    size_t _query_len;

  public:
    PsychicHTTPServerRequest(PsychicHTTPServer *server, httpd_req_t *req);
    virtual ~PsychicHTTPServerRequest();

    virtual bool isUpload() { return false; }
    virtual bool isWebSocket() { return false; }

    http_method method() {
      return (http_method)this->_req->method;
    }

    const char * methodStr() {
      return http_method_str((http_method)this->_req->method);
    }

    const char * uri() {
      return this->_req->uri;
    }

    const char * queryString() {
      //delete the old one if we have it
      if (this->_query != NULL)
        delete this->_query;

      //find our size
      this->_query_len = httpd_req_get_url_query_len(this->_req);

      //if we've got one, allocated it and load it
      if (this->_query_len)
      {
        this->_query = new char[this->_query_len];
        httpd_req_get_hdr_value_str(this->_req, name, this->_query, this->_query_len);
  
        return this->_query;
      }
      else
        return "";
    }

    const char * header(const char *name)
    {
      //delete the old one if we have it
      if (this->_header != NULL)
        delete this->_header;

      //find our size
      this->_header_len = httpd_req_get_hdr_value_len(this->_req, name);

      //if we've got one, allocated it and load it
      if (this->_header_len)
      {
        this->_header = new char[this->_header_len];
        httpd_req_get_hdr_value_str(this->_req, name, this->_header, this->_header_len);
  
        return this->_header;
      }
      else
        return "";
    }

    const char * host() {
      return this->header("Host");
    }

    const char * contentType() {
      return header("Content-Type");
    }

    size_t contentLength() {
      return this->_req->content_len;
    }

    void redirect(const char *url);

    const char * body()
    {
      this->_body_len = this->_req->content_len;

      //if we've got one, allocated it and load it
      if (this->_body_len)
      {
        this->_body = new char[this->_body_len];
        httpd_req_get_hdr_value_str(this->_req, name, this->_body, this->_body_len);
  
        int ret = httpd_req_recv(this->_req, this->_body, this->_body_len);
        if (ret <= 0) {  /* 0 return value indicates connection closed */
          /* Check if timeout occurred */
          if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
              /* In case of timeout one can choose to retry calling
              * httpd_req_recv(), but to keep it simple, here we
              * respond with an HTTP 408 (Request Timeout) error */
              httpd_resp_send_408(this->_req);
          }
          /* In case of error, returning ESP_FAIL will
          * ensure that the underlying socket is closed */
          //TODO: how do we handle returning values from the request?
          //return ESP_FAIL;
        }

        return this->_body;
      }
      else
        return "";
    }

    PsychicHTTPServerResponse *beginResponse();

    // Takes ownership of `response`, will delete when finished. Do not use `response` after calling
    void send(PsychicHTTPServerResponse *response);
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
    //std::list<mg_http_header> headers;
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

    void send(struct mg_connection *nc);
};

// #ifdef ARDUINO
//   class PsychicHTTPServerResponseStream:
//     public PsychicHTTPServerResponse,
//     public Print
//   {
//     private:
//       mg_iobuf _content;

//     public:
//       PsychicHTTPServerResponseStream();
//       virtual ~PsychicHTTPServerResponseStream();

//       size_t write(const uint8_t *data, size_t len);
//       size_t write(uint8_t data);

//       virtual void send(struct mg_connection *nc);
//   };
// #endif

typedef std::function<void(PsychicHTTPServerRequest *request)> PsychicHTTPRequestHandler;
//typedef std::function<size_t(PsychicHTTPServerRequest *request, int ev, MongooseString filename, uint64_t index, uint8_t *data, size_t len)> PsychicHTTPUploadHandler;
typedef std::function<void(PsychicHTTPWebSocketConnection *connection)> PsychicHTTPWebSocketConnectionHandler;
typedef std::function<void(PsychicHTTPWebSocketConnection *connection, int flags, uint8_t *data, size_t len)> PsychicHTTPWebSocketFrameHandler;

class PsychicHTTPServerEndpoint
{
  friend PsychicHTTPServer;

  private:
    PsychicHTTPServer *server;
    std::string uri;
    httpd_method_t method;
    PsychicHTTPRequestHandler request;
    //PsychicHTTPUploadHandler upload;
    PsychicHTTPRequestHandler close;
    PsychicHTTPWebSocketConnectionHandler wsConnect;
    PsychicHTTPWebSocketFrameHandler wsFrame;
  public:
    PsychicHTTPServerEndpoint(PsychicHTTPServer *server, httpd_method_t method) :
      server(server),
      method(method),
      request(NULL),
      //upload(NULL),
      close(NULL),
      wsConnect(NULL),
      wsFrame(NULL)
    {
    }

    PsychicHTTPServerEndpoint *onRequest(PsychicHTTPRequestHandler *handler) {
      this->request = handler;
      return this;
    }

    // PsychicHTTPServerEndpoint *onUpload(PsychicHTTPUploadHandler handler) {
    //   this->upload = handler;
    //   return this;
    // }

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

    static esp_err_t endpointRequestHandler(httpd_req_t *req)
    {
      PsychicHTTPServerEndpoint *self = (PsychicHTTPServerEndpoint *)req->user_ctx;
      PsychicHTTPServerRequest* request = new PsychicHTTPServerRequest(self->server, req);
      self->request(request);
      delete request;
    }
};

class PsychicHTTPWebSocketConnection : public PsychicHTTPServerRequest
{
  friend PsychicHTTPServer;

  public:
    PsychicHTTPWebSocketConnection(PsychicHTTPServer *server, httpd_req_t *req);
    virtual ~PsychicHTTPWebSocketConnection();

    virtual bool isWebSocket() { return true; }

    void send(int op, const void *data, size_t len);
    void send(const char *buf) {
      //send(WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }

    const mg_addr *getRemoteAddress() {
      //return &(_nc->rem);
    }
    const mg_connection *getConnection() {
      //return _nc;
    }
};

class PsychicHTTPServer
{
  protected:
    httpd_handle_t server;
    httpd_config_t config;

    PsychicHTTPServerEndpoint defaultEndpoint;

    static void eventHandler(struct mg_connection *c, int ev, void *ev_data, void *fn_data);

  public:
    PsychicHTTPServer();
    ~PsychicHTTPServer();

    static void destroy(void *ctx);

    bool begin(uint16_t port);
    bool begin(uint16_t port, const char *cert, const char *private_key);

    void poll(int timeout_ms);

    PsychicHTTPServerEndpoint *on(const char* uri);
    PsychicHTTPServerEndpoint *on(const char* uri, httpd_method_t method);
    PsychicHTTPServerEndpoint *on(const char* uri, PsychicHTTPRequestHandler onRequest);
    PsychicHTTPServerEndpoint *on(const char* uri, httpd_method_t method, PsychicHTTPRequestHandler onRequest);

    static esp_err_t endpointEventHandler(httpd_req_t *req);

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