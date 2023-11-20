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

//TODO: implement this.
void PsychicHttpServer::onNotFound(PsychicHttpRequestHandler fn)
{
  //httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
  //defaultEndpoint.onRequest(fn);
}

void PsychicHttpServer::sendAll(PsychicHttpWebSocketConnection *from, const char *endpoint, int op, const void *data, size_t len)
{
  // mg_mgr *mgr = Mongoose.getMgr();

  // const struct mg_connection *nc = from ? from->getConnection() : NULL;
  // struct mg_connection *c;
  // for (c = mgr->conns; c != NULL; c = c->next) {
  //   if (c == nc) { 
  //     continue; /* Don't send to the sender. */
  //   }
  //   if (c->is_websocket)
  //   {
  //     PsychicHttpWebSocketConnection *to = (PsychicHttpWebSocketConnection *)c->fn_data;
  //     if(endpoint && !to->uri().equals(endpoint)) {
  //       continue;
  //     }
  //     DBUGF("%.*s sending to %p", to->uri().length(), to->uri().c_str(), to);
  //     to->send(op, data, len);
  //   }
  // }
}

void PsychicHttpServer::sendAll(PsychicHttpWebSocketConnection *from, int op, const void *data, size_t len) {
  //sendAll(from, NULL, op, data, len);
}
void PsychicHttpServer::sendAll(int op, const void *data, size_t len) {
  //sendAll(NULL, NULL, op, data, len);
}
void PsychicHttpServer::sendAll(PsychicHttpWebSocketConnection *from, const char *buf) {
  //sendAll(from, NULL, WEBSOCKET_OP_TEXT, buf, strlen(buf));
}
void PsychicHttpServer::sendAll(const char *buf) {
  //sendAll(NULL, NULL, WEBSOCKET_OP_TEXT, buf, strlen(buf));
}
void PsychicHttpServer::sendAll(const char *endpoint, int op, const void *data, size_t len) {
  //sendAll(NULL, endpoint, op, data, len);
}
void PsychicHttpServer::sendAll(PsychicHttpWebSocketConnection *from, const char *endpoint, const char *buf) {
  //sendAll(from, endpoint, WEBSOCKET_OP_TEXT, buf, strlen(buf));
}
void PsychicHttpServer::sendAll(const char *endpoint, const char *buf) {
  //sendAll(NULL, endpoint, WEBSOCKET_OP_TEXT, buf, strlen(buf));
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

PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::onClose(PsychicHttpRequestHandler handler) {
  this->close = handler;
  return this;
}

esp_err_t PsychicHttpServerEndpoint::endpointRequestHandler(httpd_req_t *req)
{
  DUMP(req->uri);

  PsychicHttpServerEndpoint *self = (PsychicHttpServerEndpoint *)req->user_ctx;
  PsychicHttpServerRequest* request = new PsychicHttpServerRequest(self->server, req);
  esp_err_t err = self->request(request);
  delete request;

  return err;
}

/*************************************/
/*  PsychicHttpServerRequest         */
/*************************************/

PsychicHttpServerRequest::PsychicHttpServerRequest(PsychicHttpServer *server, httpd_req_t *req) :
  _server(server),
  _req(req),
  _response(NULL),
  _header(NULL),
  //_body(NULL),
  _query(NULL)
{
  //TODO: maybe automatically look up the data / read the connection?
  TRACE();
  this->loadBody();
  TRACE();
}

PsychicHttpServerRequest::~PsychicHttpServerRequest()
{
  if(_response) {
    delete _response;
    _response = NULL;
  }

  if (_header) {
    delete _header;
    _header = NULL;
    _header_len = 0;
  }

  // if (_body) {
  //   delete _body;
  //   _body = NULL;
  //   _body_len = 0;
  // }

  if (_query) {
    delete _query;
    _query = NULL;
    _query_len = 0;
  }
}

void PsychicHttpServerRequest::loadBody()
{
  TRACE();
  DUMP(this->_req->content_len);

  this->_body = "";

  /* Get header value string length and allocate memory for length + 1,
    * extra byte for null termination */
  if (this->_req->content_len > 0)
  {
    TRACE();
    //buf = (char *)malloc(buf_len);
    char buf[this->_req->content_len+1];
    TRACE();

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

    TRACE();
    this->_body.concat(buf);
    TRACE();
    //free(buf);
    TRACE();
  }
}

http_method PsychicHttpServerRequest::method() {
  return (http_method)this->_req->method;
}

const char * PsychicHttpServerRequest::methodStr() {
  return http_method_str((http_method)this->_req->method);
}

const char * PsychicHttpServerRequest::uri() {
  return this->_req->uri;
}

const char * PsychicHttpServerRequest::queryString() {
  TRACE();
  //delete the old one if we have it
  if (this->_query != NULL)
    delete this->_query;
  TRACE();

  //find our size
  this->_query_len = httpd_req_get_url_query_len(this->_req);
  TRACE();

  //if we've got one, allocated it and load it
  if (this->_query_len)
  {
    TRACE();
    this->_query = new char[this->_query_len+1];
    this->_query[this->_query_len] = 0;
    httpd_req_get_url_query_str(this->_req, this->_query, this->_query_len);
    TRACE();

    return this->_query;
  }
  else
    return "";
}

const char * PsychicHttpServerRequest::headers(const char *name)
{
  return this->header(name);
}

const char * PsychicHttpServerRequest::header(const char *name)
{
  //delete the old one if we have it
  if (this->_header != NULL)
    delete this->_header;

  //find our size
  this->_header_len = httpd_req_get_hdr_value_len(this->_req, name);

  //if we've got one, allocated it and load it
  if (this->_header_len)
  {
    this->_header = new char[this->_header_len+1];
    this->_header[this->_header_len] = 0;

    httpd_req_get_hdr_value_str(this->_req, name, this->_header, this->_header_len);

    return this->_header;
  }
  else
    return "";
}

const char * PsychicHttpServerRequest::host() {
  return this->header("Host");
}

const char * PsychicHttpServerRequest::contentType() {
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
  DUMP(key);
  //a small array should be fine.
  char value[2];
  esp_err_t err = this->getParam(key, value);
  DUMP((int)err);
  //did we get anything?
  if (err == ESP_OK || err == ESP_ERR_HTTPD_RESULT_TRUNC)
    return true;
  else
    return false;
}

esp_err_t PsychicHttpServerRequest::getParam(const char *key, char *value)
{
  TRACE();
  const char *query = this->queryString();
  DUMP(query);
  return httpd_query_key_value(query, key, value, this->_query_len);
}

std::string PsychicHttpServerRequest::getParam(const char *key)
{
  std::string ret;

  char value[this->_query_len];
  TRACE();
  esp_err_t err = this->getParam(key, value);
  TRACE();
  ret.append(value);
  TRACE();

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
  //TRACE();
  //DUMP(response->getContent());
  DUMP(response->getContentLength());
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
  DUMP(this->_status);
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
  DUMP(this->body);
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

PsychicHttpWebSocketConnection::~PsychicHttpWebSocketConnection()
{

}

void PsychicHttpWebSocketConnection::send(int op, const void *data, size_t len)
{
  //mg_ws_send(_nc, data, len, op);
}