#include "PsychicHttp.h"

/*************************************/
/*  PsychicHttpServer                */
/*************************************/

PsychicHttpServer::PsychicHttpServer()
{
  //some configs
  this->config = HTTPD_DEFAULT_CONFIG();

  this->config.global_user_ctx = this;
  this->config.global_user_ctx_free_fn = this->destroy;

  this->config.max_uri_handlers = 10;
  this->config.stack_size = 10000;

  //this->defaultEndpoint(this, HTTP_GET);
}

PsychicHttpServer::~PsychicHttpServer()
{

}

void PsychicHttpServer::destroy(void *ctx)
{
  PsychicHttpServer *temp = (PsychicHttpServer *)ctx;
  delete temp;
}

bool PsychicHttpServer::begin(uint16_t port)
{
  this->config.server_port = port;

  /* Start the httpd server */
  if (httpd_start(&this->server, &this->config) != ESP_OK)
    return false;
  else
    return true;
}

bool PsychicHttpServer::begin(uint16_t port, const char *cert, const char *private_key)
{
  this->config.server_port = port;

  /* Start the httpd server */
  if (httpd_start(&this->server, &this->config) == ESP_OK)
    return false;
  else
    return true;
}

void PsychicHttpServer::stop()
{
  httpd_stop(this->server);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri)
{
  return on(uri, HTTP_GET);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, PsychicHttpRequestHandler onRequest)
{
  return on(uri, HTTP_GET)->onRequest(onRequest);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, http_method method, PsychicHttpRequestHandler onRequest)
{
  return on(uri, method)->onRequest(onRequest);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, http_method method)
{
  PsychicHttpServerEndpoint *handler = new PsychicHttpServerEndpoint(this, method);
  
  // URI handler structure
  httpd_uri_t my_uri {
    .uri      = uri,
    .method   = method,
    .handler  = handler->endpointRequestHandler,
    .user_ctx = handler
  };

  // Register handler
  if (httpd_register_uri_handler(this->server, &my_uri) != ESP_OK) {
    Serial.println("PsychicHttp add endpoint failed");
  }

  return handler;
}

PsychicHttpServerEndpoint *PsychicHttpServer::websocket(const char* uri)
{
  TRACE();

  PsychicHttpServerEndpoint *handler = new PsychicHttpServerEndpoint(this, HTTP_GET);
  
  // URI handler structure
  httpd_uri_t my_uri {
    .uri      = uri,
    .method   = HTTP_GET,
    .handler  = handler->websocketRequestHandler,
    .user_ctx = handler,
    .is_websocket = true
  };

  // Register handler
  if (httpd_register_uri_handler(this->server, &my_uri) != ESP_OK) {
    Serial.println("PsychicHttp add websocket failed");
  }

  return handler;
}

//TODO: implement this.
void PsychicHttpServer::onNotFound(PsychicHttpRequestHandler fn)
{
  //httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
  //defaultEndpoint.onRequest(fn);
}

//TODO: no idea how to send a dynamic message.
void PsychicHttpServer::sendAll(httpd_ws_frame_t * ws_pkt)
{
  //DUMP((char *)ws_pkt->payload);
  size_t max_clients = this->config.max_open_sockets;
  size_t clients = max_clients;
  int    client_fds[max_clients];

  if (httpd_get_client_list(this->server, &clients, client_fds) == ESP_OK)
  {
    for (size_t i=0; i < clients; ++i)
    {
      int sock = client_fds[i];
      if (httpd_ws_get_fd_info(this->server, sock) == HTTPD_WS_CLIENT_WEBSOCKET)
      {
        ESP_LOGI(TAG, "Active client (fd=%d) -> sending async message", sock);

        async_resp_arg *resp_arg = (async_resp_arg *)malloc(sizeof(struct async_resp_arg));
        resp_arg->hd = this->server;
        resp_arg->fd = sock;
        resp_arg->ws_pkt = ws_pkt;

        if (httpd_queue_work(resp_arg->hd, this->sendAsync, resp_arg) != ESP_OK) {
          ESP_LOGE(TAG, "httpd_queue_work failed!");
          break;
        }
      }
    }
  } else {
      ESP_LOGE(TAG, "httpd_get_client_list failed!");
      return;
  }
}

void PsychicHttpServer::sendAll(httpd_ws_type_t op, const void *data, size_t len)
{
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  ws_pkt.payload = (uint8_t*)data;
  ws_pkt.len = len;
  ws_pkt.type = op;

  this->sendAll(&ws_pkt);
}

void PsychicHttpServer::sendAll(const char *buf)
{
  this->sendAll(HTTPD_WS_TYPE_TEXT, buf, strlen(buf));
}

//TODO: no idea.
void PsychicHttpServer::sendAsync(void *arg)
{
  TRACE();
  async_resp_arg *resp_arg = (async_resp_arg *)arg;

  // static const char data[resp_arg->ws_pkt->len+1];
  // strlcpy(data, resp_arg->ws_pkt->payload, sizeof(data));

  httpd_handle_t hd = resp_arg->hd;
  int fd = resp_arg->fd;

  // httpd_ws_frame_t ws_pkt;
  // memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  // ws_pkt.payload = (uint8_t*)data;
  // ws_pkt.len = strlen(data);
  // ws_pkt.type = HTTPD_WS_TYPE_TEXT;

  // httpd_ws_send_frame_async(hd, fd, &ws_pkt);
  // free(resp_arg);

  DUMP(resp_arg->ws_pkt->len);

  httpd_ws_send_frame_async(hd, fd, resp_arg->ws_pkt);
}

PsychicHttpServerEndpoint::PsychicHttpServerEndpoint(PsychicHttpServer *server, http_method method) :
  server(server),
  method(method),
  request(NULL),
  //upload(NULL),
  close(NULL),
  wsConnect(NULL),
  wsFrame(NULL)
{
}

PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::onRequest(PsychicHttpRequestHandler handler) {
  this->request = handler;
  return this;
}

// PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::onUpload(PsychicHttpUploadHandler handler) {
//   this->upload = handler;
//   return this;
// }

PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::onConnect(PsychicHttpWebSocketConnectionHandler handler) {
  this->wsConnect = handler;
  return this;
}

PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::onFrame(PsychicHttpWebSocketFrameHandler handler) {
  this->wsFrame = handler;
  return this;
}

//TODO: figure out close hooks
// PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::onClose(PsychicHttpRequestHandler handler) {
//   this->close = handler;
//   return this;
// }

esp_err_t PsychicHttpServerEndpoint::endpointRequestHandler(httpd_req_t *req)
{
  DUMP(req->uri);

  PsychicHttpServerEndpoint *self = (PsychicHttpServerEndpoint *)req->user_ctx;
  PsychicHttpServerRequest* request = new PsychicHttpServerRequest(self->server, req);
  esp_err_t err = self->request(request);
  delete request;

  return err;
}

esp_err_t PsychicHttpServerEndpoint::websocketRequestHandler(httpd_req_t *req)
{
  PsychicHttpServerEndpoint *self = (PsychicHttpServerEndpoint *)req->user_ctx;
  PsychicHttpWebSocketConnection connection(self->server, req);

  // beginning of the ws URI handler
  if (req->method == HTTP_GET) {

      //connection hook?
      if (self->wsConnect != NULL)
        self->wsConnect(&connection);
      return ESP_OK;
  }

  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;

  uint8_t *buf = NULL;
  /* Set max_len = 0 to get the frame len */
  esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
  if (ret != ESP_OK) {
      ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
      return ret;
  }
  ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
  if (ws_pkt.len) {
    /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
    buf = (uint8_t*) calloc(1, ws_pkt.len + 1);
    if (buf == NULL) {
      ESP_LOGE(TAG, "Failed to calloc memory for buf");
      return ESP_ERR_NO_MEM;
    }
    ws_pkt.payload = buf;
    /* Set max_len = ws_pkt.len to get the frame payload */
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
      free(buf);
      return ret;
    }
    ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
  }
  
  // DUMP(ws_pkt.final);
  // DUMP(ws_pkt.fragmented);
  // DUMP(ws_pkt.len);
  // DUMP(ws_pkt.type);
  // DUMP((char *)ws_pkt.payload);

  // If it was a PONG, update the keep-alive
  // if (ws_pkt.type == HTTPD_WS_TYPE_PONG) {
  //     ESP_LOGD(TAG, "Received PONG message");
  //     free(buf);
  //     return wss_keep_alive_client_is_active(httpd_get_global_user_ctx(req->handle), httpd_req_to_sockfd(req));
  // }

  //okay, call our frame handler.
  if (self->wsFrame != NULL)
    ret = self->wsFrame(&connection, &ws_pkt);

  free(buf);

  return ret;
 
  // PsychicHttpServerRequest* request = new PsychicHttpServerRequest(self->server, req);
  // esp_err_t err = self->request(request);
  // delete request;

  // return err;
}

/*************************************/
/*  PsychicHttpServerRequest         */
/*************************************/

PsychicHttpServerRequest::PsychicHttpServerRequest(PsychicHttpServer *server, httpd_req_t *req) :
  _server(server),
  _req(req),
  _response(NULL)
  //_header(NULL),
  //_body(NULL),
  //_query(NULL)
{
  this->loadBody();
}

PsychicHttpServerRequest::~PsychicHttpServerRequest()
{
  if(_response) {
    delete _response;
    _response = NULL;
  }
}

void PsychicHttpServerRequest::loadBody()
{
  this->_body = String();

  /* Get header value string length and allocate memory for length + 1,
    * extra byte for null termination */
  if (this->_req->content_len > 0)
  {
    char buf[this->_req->content_len+1];

    int ret = httpd_req_recv(this->_req, buf, this->_req->content_len);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
      /* Check if timeout occurred */
      if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
        /* In case of timeout one can choose to retry calling
        * httpd_req_recv(), but to keep it simple, here we
        * respond with an Http 408 (Request Timeout) error */
        //httpd_resp_send_408(this->_req);
        TRACE();
      }
      /* In case of error, returning ESP_FAIL will
      * ensure that the underlying socket is closed */
      //TODO: how do we handle returning values from the request?
      //return ESP_FAIL;
      TRACE();
      //return;
    }
    buf[this->_req->content_len] = '\0';

    this->_body.concat(buf);
  }
}

http_method PsychicHttpServerRequest::method() {
  return (http_method)this->_req->method;
}

String PsychicHttpServerRequest::methodStr() {
  return String(http_method_str((http_method)this->_req->method));
}

String PsychicHttpServerRequest::uri() {
  return String(this->_req->uri);
}

String PsychicHttpServerRequest::queryString() {
  size_t query_len = httpd_req_get_url_query_len(this->_req);

  //if we've got one, allocated it and load it
  if (query_len)
  {
    char query[query_len+1];
    httpd_req_get_url_query_str(this->_req, query, sizeof(query));

    return String(query);
  }
  else
    return String();
}

String PsychicHttpServerRequest::headers(const char *name)
{
  return this->header(name);
}

String PsychicHttpServerRequest::header(const char *name)
{
  size_t header_len = httpd_req_get_hdr_value_len(this->_req, name);

  //if we've got one, allocated it and load it
  if (header_len)
  {
    char header[header_len+1];
    httpd_req_get_hdr_value_str(this->_req, name, header, sizeof(header));
    return String(header);
  }
  else
    return String();
}

String PsychicHttpServerRequest::host() {
  return String(this->header("Host"));
}

String PsychicHttpServerRequest::contentType() {
  return header("Content-Type");
}

size_t PsychicHttpServerRequest::contentLength() {
  return this->_req->content_len;
}

String PsychicHttpServerRequest::body()
{
  return this->_body;
}

void PsychicHttpServerRequest::redirect(const char *url)
{
  PsychicHttpServerResponse *response = this->beginResponse();

  response->setCode(301);
  response->addHeader("Location", url);
  
  this->send(response);
}

bool PsychicHttpServerRequest::hasParam(const char *key)
{
  String query = this->queryString();
  char value[query.length()];
  esp_err_t err = httpd_query_key_value(query.c_str(), key, value, query.length());

  //did we get anything?
  if (err == ESP_OK || err == ESP_ERR_HTTPD_RESULT_TRUNC)
    return true;
  else
    return false;
}

esp_err_t PsychicHttpServerRequest::getParam(const char *key, char *value)
{
  String query = this->queryString();

  return httpd_query_key_value(query.c_str(), key, value, query.length());
}

String PsychicHttpServerRequest::getParam(const char *key)
{
  String ret;

  String query = this->queryString();
  char value[query.length()];
  esp_err_t err = this->getParam(key, value);
  ret.concat(value);

  return ret;
}

//TODO: implement
bool PsychicHttpServerRequest::authenticate(const char * username, const char * password)
{
  // DBUGVAR(username);
  // DBUGVAR(password);

  // char user_buf[64];
  // char pass_buf[64];

  // mg_http_creds(_msg, user_buf, sizeof(user_buf), pass_buf, sizeof(pass_buf));

  // DBUGVAR(user_buf);
  // DBUGVAR(pass_buf);

  // if(0 == strcmp(username, user_buf) && 0 == strcmp(password, pass_buf))
  // {
  //   return true;
  // }

  return false;
}

//TODO: implement
void PsychicHttpServerRequest::requestAuthentication(const char* realm)
{
  // https://github.com/me-no-dev/ESPAsyncWebServer/blob/master/src/WebRequest.cpp#L852
  // mg_http_send_digest_auth_request

  // char headers[64];
  // mg_snprintf(headers, sizeof(headers), 
  //     "WWW-Authenticate: Basic realm=%s",
  //     realm);

  // mg_http_reply(_nc, 401, headers, "", NULL);
}

PsychicHttpServerResponse *PsychicHttpServerRequest::beginResponse()
{
  return new PsychicHttpServerResponse(this->_req);
}

#ifdef ARDUINO
  PsychicHttpServerResponseStream *PsychicHttpServerRequest::beginResponseStream()
  {
    return new PsychicHttpServerResponseStream(this->_req);
  }
#endif

void PsychicHttpServerRequest::send(PsychicHttpServerResponse *response)
{
  httpd_resp_send(this->_req, response->getContent(), response->getContentLength());
}

void PsychicHttpServerRequest::send(int code)
{
  PsychicHttpServerResponse *response = this->beginResponse();

  response->setCode(code);
  response->setContentType("text/plain");
  response->setContent(http_status_reason(code));
  
  this->send(response);
}

void PsychicHttpServerRequest::send(const char *content)
{
  PsychicHttpServerResponse *response = this->beginResponse();

  response->setCode(200);
  response->setContentType("text/plain");
  response->setContent(content);
  
  this->send(response);
}

void PsychicHttpServerRequest::send(int code, const char *contentType, const char *content)
{
  PsychicHttpServerResponse *response = this->beginResponse();

  response->setCode(code);
  response->setContentType(contentType);
  response->setContent(content);
  
  this->send(response);
}


/*************************************/
/*  PsychicHttpServerResponse        */
/*************************************/

PsychicHttpServerResponse::PsychicHttpServerResponse(httpd_req_t *request) :
  _req(request)
{
  this->setCode(200);
}

PsychicHttpServerResponse::~PsychicHttpServerResponse()
{
}

void PsychicHttpServerResponse::addHeader(const char *name, const char *value)
{
  httpd_resp_set_hdr(this->_req, name, value);
}

void PsychicHttpServerResponse::setCode(int code)
{
  //esp-idf makes you set the whole status.
  sprintf(this->_status, "%u %s", code, http_status_reason(code));

  httpd_resp_set_status(this->_req, this->_status);
}

void PsychicHttpServerResponse::setContentType(const char *contentType)
{
  httpd_resp_set_type(this->_req, contentType);
}

void PsychicHttpServerResponse::setContent(const char *content)
{
  //DUMP(content);
  this->body = content;
  //DUMP(this->body);
  setContentLength(strlen(content));
}

void PsychicHttpServerResponse::setContent(const uint8_t *content, size_t len)
{
  this->body = (char *)content;

  setContentLength(len);
}

const char * PsychicHttpServerResponse::getContent()
{
  //DUMP(this->body);
  return this->body;
}

size_t PsychicHttpServerResponse::getContentLength()
{
  return this->_contentLength;
}

/*************************************/
/*  PsychicHttpServerResponseStream  */
/*************************************/

// #ifdef ARDUINO
  PsychicHttpServerResponseStream::PsychicHttpServerResponseStream(httpd_req_t *request) :
    PsychicHttpServerResponse(request)
  {
  }

  PsychicHttpServerResponseStream::~PsychicHttpServerResponseStream()
  {
  }

  //TODO: figure out how to make this class actually write to the network directly instead of this hack.
  size_t PsychicHttpServerResponseStream::write(const uint8_t *data, size_t len)
  {
    this->_content.append((char *)data);
    this->setContentLength(this->getContentLength()+len);

    return len;
  }

  size_t PsychicHttpServerResponseStream::write(uint8_t data)
  {
    return this->write(&data, 1);
  }
// #endif


/*************************************/
/*  PsychicHttpWebSocketConnection   */
/*************************************/

PsychicHttpWebSocketConnection::PsychicHttpWebSocketConnection(PsychicHttpServer *server, httpd_req_t *req) :
  PsychicHttpServerRequest(server, req)
{
  this->_server = req->handle;
  this->_fd = httpd_req_to_sockfd(req);
}

PsychicHttpWebSocketConnection::~PsychicHttpWebSocketConnection()
{

}

esp_err_t PsychicHttpWebSocketConnection::send(httpd_ws_frame_t * ws_pkt)
{
  //TRACE();

  return httpd_ws_send_frame(this->_req, ws_pkt);

  // this->_packet = ws_pkt;
  //return httpd_queue_work(this->_server, this->ws_async_send, this);
} 

esp_err_t PsychicHttpWebSocketConnection::send(httpd_ws_type_t op, const void *data, size_t len)
{
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  ws_pkt.payload = (uint8_t*)data;
  ws_pkt.len = len;
  ws_pkt.type = op;

  return this->send(&ws_pkt);
}

esp_err_t PsychicHttpWebSocketConnection::send(const char *buf)
{
  return this->send(HTTPD_WS_TYPE_TEXT, buf, strlen(buf));
}

void PsychicHttpWebSocketConnection::ws_async_send(void *arg)
{
  PsychicHttpWebSocketConnection * conn = (PsychicHttpWebSocketConnection *)arg;

  httpd_ws_send_frame_async(conn->_server, conn->_fd, conn->_packet);
}
