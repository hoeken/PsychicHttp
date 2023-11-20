#include "PsychicHttp.h"

/*************************************/
/*  PsychicHttpServer                */
/*************************************/

PsychicHttpServer::PsychicHttpServer()
{
  //some configs
  this->config = HttpD_DEFAULT_CONFIG();

  this->config.global_user_ctx = this;
  this->config.global_user_ctx_free_fn = this->destroy;

  //this->defaultEndpoint(this, Http_GET);
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
  if (httpd_start(&this->server, &this->config) != ESP_OK) {
    return true;
  }

  return false;
}

bool PsychicHttpServer::begin(uint16_t port, const char *cert, const char *private_key)
{
  this->config.server_port = port;

  /* Start the httpd server */
  if (httpd_start(&this->server, &this->config) == ESP_OK) {
    return true;
  }

  return false;
}

void PsychicHttpServer::stop()
{
  httpd_stop(this->server);
}

// PsychicHttpServerEndpoint::PsychicHttpServerEndpoint(PsychicHttpServer *server, httpd_method_t method)
// {
//   this->server = server;
//   this->method = method;
// }

// PsychicHttpServerEndpoint::~PsychicHttpServerEndpoint()
// {

// }

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri)
{
  return on(uri, Http_GET);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, PsychicHttpRequestHandler onRequest)
{
  return on(uri, Http_GET)->onRequest(onRequest);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, httpd_method_t method, PsychicHttpRequestHandler onRequest)
{
  return on(uri, method)->onRequest(onRequest);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, httpd_method_t method)
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
    Serial.println("Handler failed");
  }  
}

void PsychicHttpServer::onNotFound(PsychicHttpRequestHandler fn)
{
  //defaultEndpoint.onRequest(fn);
}

// void PsychicHttpServer::eventHandler(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
// {
//   //what kind of event is it?
//   switch (ev)
//   {
//     // Connection accepted
//     case MG_EV_ACCEPT:
//       //Serial.println("MG_EV_ACCEPT");
//       char addr[32];
//       mg_snprintf(addr, sizeof(addr), "%M", mg_print_ip, &c->rem);                          
//       DBUGF("Connection %p from %s", c, addr);
//       break;

//     // TLS handshake succeeded
//     case MG_EV_TLS_HS:
//       Serial.println("MG_EV_TLS_HS");
//       break;

//     // Data received from socket - long *bytes_read
//     // case MG_EV_READ: {
//     //   Serial.println("MG_EV_READ");
//     //   int *num_bytes = (int *)ev_data;
//     //   DBUGF("Received %d bytes", *num_bytes);
//     //   break;
//     // }

//     // Data written to socket - long *bytes_written
//     // case MG_EV_WRITE:
//     //   Serial.println("MG_EV_WRITE");
//     //   //TODO: old code might fit here?
//     //   //     case MG_EV_POLL:
//     //   //     case MG_EV_SEND:
//     //   //     {
//     //   //       if(nc->user_data)
//     //   //       {
//     //   //         PsychicHttpServerRequest *request = (PsychicHttpServerRequest *)nc->user_data;
//     //   //         if(request->responseSent()) {
//     //   //           request->sendBody();
//     //   //         }
//     //   //       }
//     //   //       break;
//     //   //     }
//     //   break;

//     // Connection closed
//     case MG_EV_CLOSE:
//       DBUGF("Connection %p closed", c);
//       // TODO: we will need to handle this for websockets
//       // if(c->fn_data) 
//       // {
//       //   PsychicHttpServerRequest *request = (PsychicHttpServerRequest *)c->fn_data;
//       //   // TODO: calls a member, but we're an external function 
//       //   // if(endpoint->close) {
//       //   //   endpoint->close(request);
//       //   // }
//       //   delete request;
//       //   c->fn_data = NULL;
//       // } 
//       break;

//       // //did we match our websocket endpoint?
//       // if (mg_http_match_uri(hm, "/ws")) {
//       //   // Upgrade to websocket. From now on, a connection is a full-duplex
//       //   // Websocket connection, which will receive MG_EV_WS_MSG events.
//       //   mg_ws_upgrade(c, hm, NULL);
//       // }

//       //custom handler for not found.
//       if (!found)
//       {
//         PsychicHttpServerRequest* request = new PsychicHttpServerRequest(self, c, hm);
//         self->defaultEndpoint.request(request);
//       }

//       break;
//     }

//     // Websocket handshake done     struct mg_http_message *
//     case MG_EV_WS_OPEN: {
//       Serial.println("MG_EV_WS_OPEN");
//       hm = (struct mg_http_message *) ev_data;

//       //TODO: update this
//       // if(endpoint->wsConnect && nc->flags & MG_F_IS_PsychicHttpWebSocketConnection)
//       // {
//       //   PsychicHttpWebSocketConnection *c = (PsychicHttpWebSocketConnection *)nc->user_data;
//       //   endpoint->wsConnect(c);
//       // }

//       break;
//     }

//     // Websocket msg, text or bin   struct mg_ws_message *
//     case MG_EV_WS_MSG: {
//       Serial.println("MG_EV_WS_MSG");
//       // Got websocket frame. Received data is wm->data. Echo it back!
//       struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
// //       if(endpoint->wsFrame && nc->flags & MG_F_IS_PsychicHttpWebSocketConnection)
// //       {
// //         PsychicHttpWebSocketConnection *c = (PsychicHttpWebSocketConnection *)nc->user_data;
// //         struct websocket_message *wm = (struct websocket_message *)p;
// //         endpoint->wsFrame(c, wm->flags, wm->data, wm->size);
// //       }
//       break;
//     }

//     // Websocket control msg        struct mg_ws_message *
//     case MG_EV_WS_CTL: {
//       Serial.println("MG_EV_WS_CTL");
//       struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
//       break;
//     }
// //}
//}

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

PsychicHttpServerEndpoint::PsychicHttpServerEndpoint(PsychicHttpServer *server, httpd_method_t method) :
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
  _response(NULL)
{
  // if(0 == mg_vcasecmp(&msg->method, "GET")) {
  //   _method = Http_GET;
  // } else if(0 == mg_vcasecmp(&msg->method, "POST")) {
  //   _method = Http_POST;
  // } else if(0 == mg_vcasecmp(&msg->method, "DELETE")) {
  //   _method = Http_DELETE;
  // } else if(0 == mg_vcasecmp(&msg->method, "PUT")) {
  //   _method = Http_PUT;
  // } else if(0 == mg_vcasecmp(&msg->method, "PATCH")) {
  //   _method = Http_PATCH;
  // } else if(0 == mg_vcasecmp(&msg->method, "HEAD")) {
  //   _method = Http_HEAD;
  // } else if(0 == mg_vcasecmp(&msg->method, "OPTIONS")) {
  //   _method = Http_OPTIONS;
  // }
}

PsychicHttpServerRequest::~PsychicHttpServerRequest()
{
  if(_response) {
    delete _response;
    _response = NULL;
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
  //delete the old one if we have it
  if (this->_query != NULL)
    delete this->_query;

  //find our size
  this->_query_len = httpd_req_get_url_query_len(this->_req);

  //if we've got one, allocated it and load it
  if (this->_query_len)
  {
    this->_query = new char[this->_query_len];
    httpd_req_get_url_query_str(this->_req, this->_query, this->_query_len);

    return this->_query;
  }
  else
    return "";
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
    this->_header = new char[this->_header_len];
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

const char * PsychicHttpServerRequest::body()
{
  this->_body_len = this->_req->content_len;

  //if we've got one, allocated it and load it
  if (this->_body_len)
  {
    this->_body = new char[this->_body_len];

    int ret = httpd_req_recv(this->_req, this->_body, this->_body_len);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
      /* Check if timeout occurred */
      if (ret == HttpD_SOCK_ERR_TIMEOUT) {
          /* In case of timeout one can choose to retry calling
          * httpd_req_recv(), but to keep it simple, here we
          * respond with an Http 408 (Request Timeout) error */
          httpd_resp_send_408(this->_req);
      }
      /* In case of error, returning ESP_FAIL will
      * ensure that the underlying socket is closed */
      //TODO: how do we handle returning values from the request?
      //return ESP_FAIL;
    }
    else
      return this->_body;
  }
  else
    return "";
}

//TODO: implement
void PsychicHttpServerRequest::redirect(const char *url)
{

}

//TODO: implement
bool PsychicHttpServerRequest::hasParam(const char *name) const
{
  // char dst[8];
  // int ret = getParam(name, dst, sizeof(dst));
  // return ret >= 0 || -3 == ret; 
}

//TODO: implement
int PsychicHttpServerRequest::getParam(const char *name, char *dst, size_t dst_len) const
{
  //return mg_http_get_var((Http_GET == _method) ? (&_msg->query) : (&_msg->body), name, dst, dst_len);
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

  // return false;
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
  return new PsychicHttpServerResponse();
}

void PsychicHttpServerRequest::send(PsychicHttpServerResponse *response)
{
  httpd_resp_send(this->_req, response->getContent(), response->getContentLength());
}

//what is this?
void PsychicHttpServerRequest::send(int code)
{
  PsychicHttpServerResponse *response = this->beginResponse();

  response->setCode(code);
  response->setContentType("text/plain");
  response->setContent(http_status_reason(code));
  
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

PsychicHttpServerResponse::PsychicHttpServerResponse() :
  _code(200)
{

}

PsychicHttpServerResponse::~PsychicHttpServerResponse()
{
}

//TODO: implement
void PsychicHttpServerResponse::addHeader(const char *name, const char *value)
{
  // mg_http_header header;
  // header.name = mg_str(name);
  // header.value = mg_str(value);
  // headers.push_back(header);
}

void PsychicHttpServerResponse::setContentType(const char *contentType)
{
  addHeader("Content-Type", contentType);
}

void PsychicHttpServerResponse::setContent(const char *content)
{
  this->body = content;
  setContentLength(strlen(content));
}

void PsychicHttpServerResponse::setContent(const uint8_t *content, size_t len)
{
  this->body = (char *)content;
  setContentLength(len);
}

//TODO: implement
const char * PsychicHttpServerResponse::getHeaderString()
{
  // std::string h = "";

  // for (mg_http_header header : this->headers)
  // {
  //   h.append(header.name.ptr, header.name.len);
  //   h += ": ";
  //   h.append(header.value.ptr, header.value.len);
  //   h += "\r\n";
  // }

  // return h.c_str();
}

const char * PsychicHttpServerResponse::getContent()
{
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
// PsychicHttpServerResponseStream::PsychicHttpServerResponseStream()
// {
//   mg_iobuf_init(&_content, ARDUINO_MONGOOSE_DEFAULT_STREAM_BUFFER, ARDUINO_MONGOOSE_DEFAULT_STREAM_BUFFER);
// }

// PsychicHttpServerResponseStream::~PsychicHttpServerResponseStream()
// {
//   mg_iobuf_free(&_content);
// }

// size_t PsychicHttpServerResponseStream::write(const uint8_t *data, size_t len)
// {
//   size_t written = mg_iobuf_add(&_content, _content.len, data, len);
//   setContentLength(_content.len);
//   return written;
// }

// size_t PsychicHttpServerResponseStream::write(uint8_t data)
// {
//   return write(&data, 1);
// }

// void PsychicHttpServerResponseStream::send(struct mg_connection *nc)
// {
//   const char *headers = getHeaderString();

//   mg_http_reply(nc, _code, headers, body);
// }

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