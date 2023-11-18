#if defined(ENABLE_DEBUG) && !defined(ENABLE_DEBUG_MONGOOSE_HTTP_SERVER)
#undef ENABLE_DEBUG
#endif

#include "MongooseHttpServer.h"

MongooseHttpServer::MongooseHttpServer() :
  nc(NULL),
  defaultEndpoint(this, HTTP_ANY)
{

}

MongooseHttpServer::~MongooseHttpServer()
{

}

void MongooseHttpServer::begin(uint16_t port)
{
  char url[30];
  sprintf(url, "http://0.0.0.0:%u", port);

  this->nc = mg_listen(Mongoose.getMgr(), url, http_event_callback, this);
}

#if MG_ENABLE_SSL
  void MongooseHttpServer::begin(uint16_t port, const char *cert, const char *private_key)
  {
    char url[30];
    sprintf(url, "https://0.0.0.0:%u", port);

    this->nc = mg_listen(Mongoose.getMgr(), url, http_event_callback, this);

    //TODO: save our cert and private key for use in the MG_EV_ACCEPT event
    // if (ev == MG_EV_ACCEPT) {
    //   struct mg_tls_opts opts = {.cert = mg_unpacked("/certs/server_cert.pem"),
    //                              .key = mg_unpacked("/certs/server_key.pem")};
    //   mg_tls_init(c, &opts);
    // }
    // bind_opts.ssl_cert = cert;
    // bind_opts.ssl_key = private_key;
    // bind_opts.error_string = &err;
  }
#endif

MongooseHttpServerEndpoint *MongooseHttpServer::on(const char* uri)
{
  return on(uri, HTTP_ANY);
}

MongooseHttpServerEndpoint *MongooseHttpServer::on(const char* uri, MongooseHttpRequestHandler onRequest)
{
  return on(uri, HTTP_ANY)->onRequest(onRequest);
}

MongooseHttpServerEndpoint *MongooseHttpServer::on(const char* uri, HttpRequestMethodComposite method, MongooseHttpRequestHandler onRequest)
{
  return on(uri, method)->onRequest(onRequest);
}

MongooseHttpServerEndpoint *MongooseHttpServer::on(const char* uri, HttpRequestMethodComposite method)
{
  MongooseHttpServerEndpoint *handler = new MongooseHttpServerEndpoint(this, method);

  //TODO: add to a list of our callbacks and loop through them in the callback method.
  //mg_register_http_endpoint(nc, uri, endpointEventHandler, handler);

  return handler;
}

//TODO: add this to the end of our http event handler
void MongooseHttpServer::onNotFound(MongooseHttpRequestHandler fn)
{
  defaultEndpoint.onRequest(fn);
}

//TODO: where is this used / called?
void MongooseHttpServer::defaultEventHandler(struct mg_connection *nc, int ev, void *p, void *u)
{
  MongooseHttpServer *self = (MongooseHttpServer *)u;
  //if (ev != MG_EV_POLL) { DBUGF("defaultEventHandler: nc = %p, self = %p", nc, self); }
  self->eventHandler(nc, ev, p, HTTP_ANY, &(self->defaultEndpoint));
}

//Callback entry point for endpoints from the server
void MongooseHttpServer::endpointEventHandler(struct mg_connection *nc, int ev, void *p, void *u)
{
  MongooseHttpServerEndpoint *self = (MongooseHttpServerEndpoint *)u;
  //if (ev != MG_EV_POLL) { DBUGF("endpointEventHandler: nc = %p,  self = %p,  self->server = %p", nc, self, self->server); }
  self->server->eventHandler(nc, ev, p, self->method, self);
}

//called by the endpoint to handle the request
//TODO: remove this
void MongooseHttpServer::eventHandler(struct mg_connection *nc, int ev, void *p, HttpRequestMethodComposite method, MongooseHttpServerEndpoint *endpoint)
{
}

void MongooseHttpServer::sendAll(MongooseHttpWebSocketConnection *from, const char *endpoint, int op, const void *data, size_t len)
{
  mg_mgr *mgr = Mongoose.getMgr();

  const struct mg_connection *nc = from ? from->getConnection() : NULL;
  struct mg_connection *c;
  for (c = mgr->conns; c != NULL; c = c->next) {
    if (c == nc) { 
      continue; /* Don't send to the sender. */
    }
    if (c->is_websocket)
    {
      MongooseHttpWebSocketConnection *to = (MongooseHttpWebSocketConnection *)c->fn_data;
      if(endpoint && !to->uri().equals(endpoint)) {
        continue;
      }
      DBUGF("%.*s sending to %p", to->uri().length(), to->uri().c_str(), to);
      to->send(op, data, len);
    }
  }

  // const struct mg_connection *nc = from ? from->getConnection() : NULL;
  // struct mg_connection *c;
  // for (c = mg_next(mgr, NULL); c != NULL; c = mg_next(mgr, c)) {
  //   if (c == nc) { 
  //     continue; /* Don't send to the sender. */
  //   }
  //   if (c->flags & MG_F_IS_WEBSOCKET && c->flags & MG_F_IS_MongooseHttpWebSocketConnection)
  //   {
  //     MongooseHttpWebSocketConnection *to = (MongooseHttpWebSocketConnection *)c->user_data;
  //     if(endpoint && !to->uri().equals(endpoint)) {
  //       continue;
  //     }
  //     DBUGF("%.*s sending to %p", to->uri().length(), to->uri().c_str(), to);
  //     to->send(op, data, len);
  //   }
  // }
}

/// MongooseHttpServerRequest object

MongooseHttpServerRequest::MongooseHttpServerRequest(MongooseHttpServer *server, mg_connection *nc, mg_http_message *msg) :
  _server(server),
  _nc(nc),
#if MG_COPY_HTTP_MESSAGE
  _msg(duplicateMessage(msg)),
#else
  _msg(msg),
#endif
  _response(NULL),
  _responseSent(false)
{
  if(0 == mg_vcasecmp(&msg->method, "GET")) {
    _method = HTTP_GET;
  } else if(0 == mg_vcasecmp(&msg->method, "POST")) {
    _method = HTTP_POST;
  } else if(0 == mg_vcasecmp(&msg->method, "DELETE")) {
    _method = HTTP_DELETE;
  } else if(0 == mg_vcasecmp(&msg->method, "PUT")) {
    _method = HTTP_PUT;
  } else if(0 == mg_vcasecmp(&msg->method, "PATCH")) {
    _method = HTTP_PATCH;
  } else if(0 == mg_vcasecmp(&msg->method, "HEAD")) {
    _method = HTTP_HEAD;
  } else if(0 == mg_vcasecmp(&msg->method, "OPTIONS")) {
    _method = HTTP_OPTIONS;
  }
}

MongooseHttpServerRequest::~MongooseHttpServerRequest()
{
  if(_response) {
    delete _response;
    _response = NULL;
  }

#if MG_COPY_HTTP_MESSAGE
  // mg_strfree(&_msg->message);
  // delete _msg;
  // _msg = NULL;
#endif
}

#if MG_COPY_HTTP_MESSAGE
// mg_str mg_mk_str_from_offsets(mg_str &dest, mg_str &src, mg_str &value) {
//   mg_str s;

//   s.ptr = value.ptr ? (dest.ptr + (value.ptr - src.ptr)) : NULL;
//   s.len = value.len;

//   return s;
// }

// mg_http_message *MongooseHttpServerRequest::duplicateMessage(mg_http_message *sm)
// {
//   mg_http_message *nm = new mg_http_message();
//   memset(nm, 0, sizeof(*nm));

//   mg_str headers = mg_mk_str_n(sm->message.p, sm->message.len - sm->body.len);

//   nm->message = mg_strdup_nul(headers);
//   nm->body = sm->body;

//   nm->method = mg_mk_str_from_offsets(nm->message, sm->message, sm->method);
//   nm->uri = mg_mk_str_from_offsets(nm->message, sm->message, sm->uri);
//   nm->proto = mg_mk_str_from_offsets(nm->message, sm->message, sm->proto);

//   nm->resp_code = sm->resp_code;
//   nm->resp_status_msg = mg_mk_str_from_offsets(nm->message, sm->message, sm->resp_status_msg);

//   nm->query_string = mg_mk_str_from_offsets(nm->message, sm->message, sm->query_string);

//   for(int i = 0; i < MG_MAX_HTTP_HEADERS; i++)
//   {
//     nm->header_names[i] = mg_mk_str_from_offsets(nm->message, sm->message, sm->header_names[i]);
//     nm->header_values[i] = mg_mk_str_from_offsets(nm->message, sm->message, sm->header_values[i]);
//   }
  
//   return nm;
// }
#endif

void MongooseHttpServerRequest::redirect(const char *url)
{

}

#ifdef ARDUINO
  void MongooseHttpServerRequest::redirect(const String& url)
  {
    redirect(url.c_str());
  }
#endif

MongooseHttpServerResponseBasic *MongooseHttpServerRequest::beginResponse()
{
  return new MongooseHttpServerResponseBasic();
}

#ifdef ARDUINO
  MongooseHttpServerResponseStream *MongooseHttpServerRequest::beginResponseStream()
  {
    return new MongooseHttpServerResponseStream();
  }
#endif

void MongooseHttpServerRequest::send(MongooseHttpServerResponse *response)
{
  if(_response) {
    delete _response;
    _response = NULL;
  }

  response->sendHeaders(_nc);
  _response = response;
  sendBody();
  _responseSent = true;
}

void MongooseHttpServerRequest::sendBody()
{
  if(_response) 
  {
    size_t free = _nc->send.size - _nc->send.len;
    if(0 == _nc->send.size) {
      free = ARDUINO_MONGOOSE_SEND_BUFFER_SIZE;
    }
    if(free > 0) 
    {
      size_t sent = _response->sendBody(_nc, free);
      DBUGF("Connection %p: sent %u/%u, %lx", _nc, sent, free, _nc->flags & MG_F_SEND_AND_CLOSE);

      if(sent < free) {
        DBUGLN("Response finished");
        delete _response;
        _response = NULL;
      }
    }
  }
}

//what is this?
void MongooseHttpServerRequest::send(int code)
{
  send(code, "text/plain", http_status_reason(code));
}

void MongooseHttpServerRequest::send(int code, const char *contentType, const char *content)
{
  //TODO: add ability to set headers through an easy function (warthog)
  char headers[64];
  mg_snprintf(headers, sizeof(headers), 
      "Connection: close\r\n"
      "Content-Type: %s",
      contentType);

  mg_http_reply(_nc, code, headers, "%s\n", content);
  _nc->is_draining = 1;
  _responseSent = true;
}

#ifdef ARDUINO
  void MongooseHttpServerRequest::send(int code, const String& contentType, const String& content)
  {
    send(code, contentType.c_str(), content.c_str());
  }
#endif

// IMPROVE: add a function to Mongoose to do this
bool MongooseHttpServerRequest::hasParam(const char *name) const
{
  char dst[8];
  int ret = getParam(name, dst, sizeof(dst));
  return ret >= 0 || -3 == ret; 
}

#ifdef ARDUINO
  bool MongooseHttpServerRequest::hasParam(const String& name) const
  {
    char dst[8];
    int ret = getParam(name, dst, sizeof(dst));
    return ret >= 0 || -3 == ret; 
  }
#endif

#ifdef ARDUINO
  bool MongooseHttpServerRequest::hasParam(const __FlashStringHelper * data) const
  {
    char dst[8];
    int ret = getParam(data, dst, sizeof(dst));
    return ret >= 0 || -3 == ret; 
  }
#endif

int MongooseHttpServerRequest::getParam(const char *name, char *dst, size_t dst_len) const
{
  return mg_http_get_var((HTTP_GET == _method) ? (&_msg->query) : (&_msg->body), name, dst, dst_len);
}

#ifdef ARDUINO
int MongooseHttpServerRequest::getParam(const String& name, char *dst, size_t dst_len) const
{
  return getParam(name.c_str(), dst, dst_len);
}
#endif

#ifdef ARDUINO
int MongooseHttpServerRequest::getParam(const __FlashStringHelper * data, char *dst, size_t dst_len) const
{
  PGM_P p = reinterpret_cast<PGM_P>(data);
  size_t n = strlen_P(p);
  char * name = (char*) malloc(n+1);
  if (name) {
    strcpy_P(name, p);   
    int result = getParam(name, dst, dst_len); 
    free(name); 
    return result; 
  } 
  
  return -5; 
}
#endif

#ifdef ARDUINO
String MongooseHttpServerRequest::getParam(const char *name) const
{
  String ret = "";
  char *tempString = new char[ARDUINO_MONGOOSE_PARAM_BUFFER_LENGTH];
  if(tempString)
  {
    if(getParam(name, tempString, ARDUINO_MONGOOSE_PARAM_BUFFER_LENGTH) > 0) {
      ret.concat(tempString);
    }

    delete tempString;
  }
  return ret;
}
#endif

#ifdef ARDUINO
String MongooseHttpServerRequest::getParam(const String& name) const
{
  String ret = "";
  char *tempString = new char[ARDUINO_MONGOOSE_PARAM_BUFFER_LENGTH];
  if(tempString)
  {
    if(getParam(name, tempString, ARDUINO_MONGOOSE_PARAM_BUFFER_LENGTH) > 0) {
      ret.concat(tempString);
    }

    delete tempString;
  }
  return ret;
}
#endif

#ifdef ARDUINO
String MongooseHttpServerRequest::getParam(const __FlashStringHelper * data) const
{
  String ret = "";
  char *tempString = new char[ARDUINO_MONGOOSE_PARAM_BUFFER_LENGTH];
  if(tempString)
  {
    if(getParam(data, tempString, ARDUINO_MONGOOSE_PARAM_BUFFER_LENGTH) > 0) {
      ret.concat(tempString);
    }

    delete tempString;
  }
  return ret;
}
#endif

bool MongooseHttpServerRequest::authenticate(const char * username, const char * password)
{
  DBUGVAR(username);
  DBUGVAR(password);

  char user_buf[64];
  char pass_buf[64];

  mg_http_creds(_msg, user_buf, sizeof(user_buf), pass_buf, sizeof(pass_buf));

  DBUGVAR(user_buf);
  DBUGVAR(pass_buf);

  if(0 == strcmp(username, user_buf) && 0 == strcmp(password, pass_buf))
  {
    return true;
  }

  return false;
}

void MongooseHttpServerRequest::requestAuthentication(const char* realm)
{
  // https://github.com/me-no-dev/ESPAsyncWebServer/blob/master/src/WebRequest.cpp#L852
  // mg_http_send_digest_auth_request

  char headers[64];
  mg_snprintf(headers, sizeof(headers), 
      "WWW-Authenticate: Basic realm=%s",
      realm);

  mg_http_reply(_nc, 401, headers, "", NULL);

  _nc->is_draining = 1;
  _responseSent = true;
}

MongooseHttpServerResponse::MongooseHttpServerResponse() :
  _code(200),
  _contentType(NULL),
  _contentLength(-1),
  _headerBuffer(NULL)
{

}

MongooseHttpServerResponse::~MongooseHttpServerResponse()
{
  if(_contentType) {
    free(_contentType);
    _contentType = NULL;
  }

  if(_headerBuffer) {
    free(_headerBuffer);
    _headerBuffer = NULL;
  }
}
void MongooseHttpServerResponse::sendHeaders(struct mg_connection *nc)
{
  char headers[64];
  // TODO: we need better header generation
  // mg_snprintf(headers, sizeof(headers), 
  //     "Connection: close\r\n"
  //     "Content-Type: %s%s",
  //     _contentType ? _contentType : "text/plain", _headerBuffer ? _headerBuffer : "");

  mg_send(nc, headers, sizeof(headers));
  nc->is_draining = 1;
  //nc->_responseSent = true;
} 

bool MongooseHttpServerResponse::addHeader(const char *name, const char *value)
{
  //TODO: convert this to std::string
  size_t startLen = _headerBuffer ? strlen(_headerBuffer) : 0;
  size_t newLen = sizeof(": \r\n");
  newLen += strlen(name);
  newLen += strlen(value);
  size_t len = startLen + newLen;

  char * newBuffer = (char *)realloc(_headerBuffer, len);
  if(newBuffer)
  {
    snprintf(newBuffer + startLen, newLen, "\r\n%s: %s", name, value);
    _headerBuffer = newBuffer;
    return true;
  }

  return false;
}

#ifdef ARDUINO
bool MongooseHttpServerResponse::addHeader(const String& name, const String& value)
{
  return addHeader(name.c_str(), value.c_str());
}
#endif

void MongooseHttpServerResponse::setContentType(const char *contentType)
{
  size_t len = strlen(contentType);
  char *newPtr = (char *)realloc(_contentType, len + 1);
  if(newPtr) {
    strcpy(newPtr, contentType);
    _contentType = newPtr;
  }
}

#ifdef ARDUINO
void MongooseHttpServerResponse::setContentType(const __FlashStringHelper *contentType)
{
  size_t len = strlen_P((PGM_P)contentType);
  char *newPtr = (char *)realloc(_contentType, len + 1);
  if(newPtr) {
    strcpy_P(newPtr, (PGM_P)contentType);
    _contentType = newPtr;
  }
}
#endif

MongooseHttpServerResponseBasic::MongooseHttpServerResponseBasic() :
  ptr(NULL), len(0)
{

}

void MongooseHttpServerResponseBasic::setContent(const char *content)
{
  setContent((uint8_t *)content, strlen(content));
}

void MongooseHttpServerResponseBasic::setContent(const uint8_t *content, size_t len)
{
  this->ptr = content;
  this->len = len;
  setContentLength(this->len);
}

size_t MongooseHttpServerResponseBasic::sendBody(struct mg_connection *nc, size_t bytes)
{
  size_t send = std::min(len, bytes);

  mg_send(nc, ptr, send);

  ptr += send;
  len -= send;

  if(0 == len) {
    nc->is_draining = 1;
  }

  return send;
}

#ifdef ARDUINO
MongooseHttpServerResponseStream::MongooseHttpServerResponseStream()
{
  mg_iobuf_init(&_content, ARDUINO_MONGOOSE_DEFAULT_STREAM_BUFFER, 64);
}

MongooseHttpServerResponseStream::~MongooseHttpServerResponseStream()
{
  mg_iobuf_free(&_content);
}

size_t MongooseHttpServerResponseStream::write(const uint8_t *data, size_t len)
{
  size_t written = mg_iobuf_add(&_content, _content.len, data, len);
  setContentLength(_content.len);
  return written;
}

size_t MongooseHttpServerResponseStream::write(uint8_t data)
{
  return write(&data, 1);
}

size_t MongooseHttpServerResponseStream::sendBody(struct mg_connection *nc, size_t bytes)
{
  size_t send = std::min(_content.len, bytes);

  mg_send(nc, _content.buf, send);

  mg_iobuf_del(&_content, 0, send);

  if(0 == _content.len) {
    nc->is_draining = 1;
  }

  return send;
}

#endif

//TODO: do we really need this specific flag?
// MongooseHttpWebSocketConnection::MongooseHttpWebSocketConnection(MongooseHttpServer *server, mg_connection *nc, mg_http_message *msg) :
//   MongooseHttpServerRequest(server, nc, msg)
// {
//   nc->flags |= MG_F_IS_MongooseHttpWebSocketConnection;
// }

MongooseHttpWebSocketConnection::~MongooseHttpWebSocketConnection()
{

}

void MongooseHttpWebSocketConnection::send(int op, const void *data, size_t len)
{
  mg_ws_send(_nc, data, len, op);
}

// Mongoose event handler function, gets called by the mg_mgr_poll()
static void http_event_callback(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  struct mg_http_message *hm;

  if (ev != MG_EV_POLL) { DBUGF("%s %p: %d", __PRETTY_FUNCTION__, nc, ev); }

  //what kind of event is it?
  switch (ev)
  {
    // Error - char *error_message
    case MG_EV_ERROR:
      DBUGF("Error: %s", (char *)ev_data);
      break;

    // Connection created
    case MG_EV_OPEN:
      Serial.println("MG_EV_OPEN");
      break;

    // mg_mgr_poll iteration
    case MG_EV_POLL:
      Serial.println("MG_EV_POLL");
      break;

    // Host name is resolved
    case MG_EV_RESOLVE:
      Serial.println("MG_EV_RESOLVE");
      break;

    // Connection established
    case MG_EV_CONNECT:
      Serial.println("MG_EV_CONNECT");
      break;

    // Connection accepted
    case MG_EV_ACCEPT:
      //Serial.println("MG_EV_ACCEPT");
      char addr[32];
      mg_snprintf(addr, sizeof(addr), "%M", mg_print_ip, &c->rem);                          
      DBUGF("Connection %p from %s", nc, addr);
      break;

    // TLS handshake succeeded
    case MG_EV_TLS_HS:
      Serial.println("MG_EV_TLS_HS");
      break;

    // Data received from socket - long *bytes_read
    case MG_EV_READ: {
      int *num_bytes = (int *)ev_data;
      DBUGF("Received %d bytes", *num_bytes);
      break;
    }

    // Data written to socket - long *bytes_written
    case MG_EV_WRITE:
      Serial.println("MG_EV_WRITE");
      //TODO: old code might fit here?
      //     case MG_EV_POLL:
      //     case MG_EV_SEND:
      //     {
      //       if(nc->user_data)
      //       {
      //         MongooseHttpServerRequest *request = (MongooseHttpServerRequest *)nc->user_data;
      //         if(request->responseSent()) {
      //           request->sendBody();
      //         }
      //       }

      //       break;
      //     }
      break;

    // Connection closed
    case MG_EV_CLOSE:
      DBUGF("Connection %p closed", c);
      if(c->fn_data) 
      {
        MongooseHttpServerRequest *request = (MongooseHttpServerRequest *)c->fn_data;

        // TODO: calls a member, but we're an external function 
        // if(endpoint->close) {
        //   endpoint->close(request);
        // }

        delete request;
        c->fn_data = NULL;
      } 

      break;

    // HTTP request/response - struct mg_http_message *
    case MG_EV_HTTP_MSG:
      Serial.println("MG_EV_HTTP_MSG");
      hm = (struct mg_http_message *) ev_data;

      //did we match our websocket endpoint?
      if (mg_http_match_uri(hm, "/ws")) {
        // Upgrade to websocket. From now on, a connection is a full-duplex
        // Websocket connection, which will receive MG_EV_WS_MSG events.
        mg_ws_upgrade(c, hm, NULL);
      }

      //did we match a HTTP endpoint?
      if (mg_http_match_uri(hm, "/api/hello")) {              // On /api/hello requests,
        mg_http_reply(c, 200, "", "{%m:%d}\n",
        MG_ESC("status"), 1);                   // Send dynamic JSON response
      } else {                                                // For all other URIs,
        struct mg_http_serve_opts opts = {.root_dir = "."};   // Serve files
        mg_http_serve_dir(c, hm, &opts);                      // From root_dir
      }
      break;

    // Websocket handshake done     struct mg_http_message *
    case MG_EV_WS_OPEN:
      Serial.println("MG_EV_WS_OPEN");
      hm = (struct mg_http_message *) ev_data;

      //TODO: update this
      // if(endpoint->wsConnect && nc->flags & MG_F_IS_MongooseHttpWebSocketConnection)
      // {
      //   MongooseHttpWebSocketConnection *c = (MongooseHttpWebSocketConnection *)nc->user_data;
      //   endpoint->wsConnect(c);
      // }

      break;

    // Websocket msg, text or bin   struct mg_ws_message *
    case MG_EV_WS_MSG: {
      Serial.println("MG_EV_WS_MSG");
      // Got websocket frame. Received data is wm->data. Echo it back!
      struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
//       if(endpoint->wsFrame && nc->flags & MG_F_IS_MongooseHttpWebSocketConnection)
//       {
//         MongooseHttpWebSocketConnection *c = (MongooseHttpWebSocketConnection *)nc->user_data;
//         struct websocket_message *wm = (struct websocket_message *)p;
//         endpoint->wsFrame(c, wm->flags, wm->data, wm->size);
//       }
    }
      break;

    // Websocket control msg        struct mg_ws_message *
    case MG_EV_WS_CTL: {
      Serial.println("MG_EV_WS_CTL");
      struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
      break;
    }

    // MQTT low-level command       struct mg_mqtt_message *
    case MG_EV_MQTT_CMD: {
      Serial.println("MG_EV_MQTT_CMD");
      struct mg_mqtt_message *mm = (struct mg_mqtt_message *) ev_data;
      break;
    }

    // MQTT PUBLISH received        struct mg_mqtt_message *
    case MG_EV_MQTT_MSG: {
      Serial.println("MG_EV_MQTT_MSG");
      struct mg_mqtt_message *mm = (struct mg_mqtt_message *) ev_data;
      break;
    }

    // MQTT CONNACK received        int *connack_status_code
    case MG_EV_MQTT_OPEN:
      Serial.println("MG_EV_MQTT_OPEN");
      break;

    // SNTP time received           uint64_t *epoch_millis
    case MG_EV_SNTP_TIME:
      Serial.println("MG_EV_SNTP_TIME");
      break;
  }
}