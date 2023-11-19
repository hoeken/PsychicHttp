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

  this->nc = mg_http_listen(Mongoose.getMgr(), url, eventHandler, this);
}

#if MG_ENABLE_SSL
  void MongooseHttpServer::begin(uint16_t port, const char *cert, const char *private_key)
  {
    char url[30];
    sprintf(url, "https://0.0.0.0:%u", port);

    this->nc = mg_http_listen(Mongoose.getMgr(), url, eventHandler, this);

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

  //save our uri
  handler->uri = new MongooseString(uri);

  //keep a list of our handlers
  endpoints.push_back(handler);

  return handler;
}

//TODO: add this to the end of our http event handler
void MongooseHttpServer::onNotFound(MongooseHttpRequestHandler fn)
{
  defaultEndpoint.onRequest(fn);
}

//TODO: where is this used / called?
// void MongooseHttpServer::defaultEventHandler(struct mg_connection *nc, int ev, void *p, void *u)
// {
//   MongooseHttpServer *self = (MongooseHttpServer *)u;
//   //if (ev != MG_EV_POLL) { DBUGF("defaultEventHandler: nc = %p, self = %p", nc, self); }
//   self->eventHandler(nc, ev, p, HTTP_ANY, &(self->defaultEndpoint));
// }

//Callback entry point for endpoints from the server
// void MongooseHttpServer::endpointEventHandler(struct mg_connection *nc, int ev, void *p, void *u)
// {
//   MongooseHttpServerEndpoint *self = (MongooseHttpServerEndpoint *)u;
//   //if (ev != MG_EV_POLL) { DBUGF("endpointEventHandler: nc = %p,  self = %p,  self->server = %p", nc, self, self->server); }
//   self->server->eventHandler(nc, ev, p, self->method, self);
// }

void MongooseHttpServer::eventHandler(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  MongooseHttpServer *self = (MongooseHttpServer *)fn_data;
  struct mg_http_message *hm;

  // if (ev != MG_EV_POLL)
  //   DBUGF("[mg] %s %p: %d", __PRETTY_FUNCTION__, c, ev);

  //what kind of event is it?
  switch (ev)
  {
    // Error - char *error_message
    case MG_EV_ERROR:
      DBUGF("MG_EV_ERROR: %s", (char *)ev_data);
      break;

    // Connection created
    case MG_EV_OPEN:
      //Serial.println("MG_EV_OPEN");
      break;

    // mg_mgr_poll iteration
    // dont do anything.
    case MG_EV_POLL:
      //Serial.println("MG_EV_POLL");
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
      DBUGF("Connection %p from %s", c, addr);
      break;

    // TLS handshake succeeded
    case MG_EV_TLS_HS:
      Serial.println("MG_EV_TLS_HS");
      break;

    // Data received from socket - long *bytes_read
    // case MG_EV_READ: {
    //   Serial.println("MG_EV_READ");
    //   int *num_bytes = (int *)ev_data;
    //   DBUGF("Received %d bytes", *num_bytes);
    //   break;
    // }

    // Data written to socket - long *bytes_written
    // case MG_EV_WRITE:
    //   Serial.println("MG_EV_WRITE");
    //   //TODO: old code might fit here?
    //   //     case MG_EV_POLL:
    //   //     case MG_EV_SEND:
    //   //     {
    //   //       if(nc->user_data)
    //   //       {
    //   //         MongooseHttpServerRequest *request = (MongooseHttpServerRequest *)nc->user_data;
    //   //         if(request->responseSent()) {
    //   //           request->sendBody();
    //   //         }
    //   //       }
    //   //       break;
    //   //     }
    //   break;

    // Connection closed
    case MG_EV_CLOSE:
      DBUGF("Connection %p closed", c);
      // TODO: we will need to handle this for websockets
      // if(c->fn_data) 
      // {
      //   MongooseHttpServerRequest *request = (MongooseHttpServerRequest *)c->fn_data;
      //   // TODO: calls a member, but we're an external function 
      //   // if(endpoint->close) {
      //   //   endpoint->close(request);
      //   // }
      //   delete request;
      //   c->fn_data = NULL;
      // } 
      break;

    // HTTP request/response - struct mg_http_message *
    case MG_EV_HTTP_MSG: {
      Serial.println("MG_EV_HTTP_MSG");
      hm = (struct mg_http_message *) ev_data;

      //look to see if we have a request handler
      bool found = false;
      for (MongooseHttpServerEndpoint* endpoint : self->endpoints)
      {
        if(mg_http_match_uri(hm, endpoint->uri->c_str()))
        {
          Serial.printf("%s matches\n", endpoint->uri->c_str());

          if (endpoint->request != NULL)
          {
            found = true;
            MongooseHttpServerRequest* request = new MongooseHttpServerRequest(self, c, hm);
            endpoint->request(request);
            delete request;
          }
        }
      }

      // //did we match our websocket endpoint?
      // if (mg_http_match_uri(hm, "/ws")) {
      //   // Upgrade to websocket. From now on, a connection is a full-duplex
      //   // Websocket connection, which will receive MG_EV_WS_MSG events.
      //   mg_ws_upgrade(c, hm, NULL);
      // }

      //custom handler for not found.
      if (!found)
      {
        MongooseHttpServerRequest* request = new MongooseHttpServerRequest(self, c, hm);
        self->defaultEndpoint.request(request);
      }

      break;
    }

    // Websocket handshake done     struct mg_http_message *
    case MG_EV_WS_OPEN: {
      Serial.println("MG_EV_WS_OPEN");
      hm = (struct mg_http_message *) ev_data;

      //TODO: update this
      // if(endpoint->wsConnect && nc->flags & MG_F_IS_MongooseHttpWebSocketConnection)
      // {
      //   MongooseHttpWebSocketConnection *c = (MongooseHttpWebSocketConnection *)nc->user_data;
      //   endpoint->wsConnect(c);
      // }

      break;
    }

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
      break;
    }

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
//}
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
  _response(NULL)
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
mg_http_message *MongooseHttpServerRequest::duplicateMessage(mg_http_message *sm)
{
  mg_http_message *nm = new mg_http_message();

  nm->method = mg_strdup(sm->method);
  nm->uri = mg_strdup(sm->method);
  nm->query = mg_strdup(sm->method);
  nm->proto = mg_strdup(sm->proto);
  nm->body = mg_strdup(sm->body);
  nm->message = mg_strdup(sm->message);

  for(int i = 0; i < MG_MAX_HTTP_HEADERS; i++)
  {
    nm->headers[i].name = mg_strdup(sm->headers[i].name);
    nm->headers[i].value = mg_strdup(sm->headers[i].value);
  }

  return nm;
}
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

MongooseHttpServerResponse *MongooseHttpServerRequest::beginResponse()
{
  return new MongooseHttpServerResponse();
}

#ifdef ARDUINO
  MongooseHttpServerResponseStream *MongooseHttpServerRequest::beginResponseStream()
  {
    return new MongooseHttpServerResponseStream();
  }
#endif

void MongooseHttpServerRequest::send(MongooseHttpServerResponse *response)
{
  response->send(this->_nc);
}

// void MongooseHttpServerRequest::sendBody()
// {
//   if(_response) 
//   {
//     size_t free = _nc->send.size - _nc->send.len;
//     if(0 == _nc->send.size) {
//       free = ARDUINO_MONGOOSE_SEND_BUFFER_SIZE;
//     }
//     if(free > 0) 
//     {
//       //TODO: change to the new way of sending stuff.
//       //size_t sent = _response->sendBody(_nc, free);
//       //DBUGF("Connection %p: sent %u/%u, %lx", _nc, sent, free, _nc->is_draining);
//       // if(sent < free) {
//       //   DBUGLN("Response finished");
//       //   delete _response;
//       //   _response = NULL;
//       // }
//     }
//   }
// }

//what is this?
void MongooseHttpServerRequest::send(int code)
{
  send(code, "text/plain", http_status_reason(code));
}

void MongooseHttpServerRequest::send(int code, const char *contentType, const char *content)
{
  MongooseHttpServerResponse *response = this->beginResponse();

  response->setCode(code);
  response->setContentType(contentType);
  response->setContent(content);
  
  this->send(response);
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
}



MongooseHttpServerResponse::MongooseHttpServerResponse() :
  _code(200)
{

}

MongooseHttpServerResponse::~MongooseHttpServerResponse()
{
}

void MongooseHttpServerResponse::addHeader(const char *name, const char *value)
{
  mg_http_header header;
  header.name = mg_str(name);
  header.value = mg_str(value);
  headers.push_back(header);
}

#ifdef ARDUINO
void MongooseHttpServerResponse::addHeader(const String& name, const String& value)
{
  return addHeader(name.c_str(), value.c_str());
}
#endif

void MongooseHttpServerResponse::setContentType(const char *contentType)
{
  addHeader("Content-Type", contentType);
}

void MongooseHttpServerResponse::setContent(const char *content)
{
  this->body = content;
  setContentLength(strlen(content));
  //this->body = mg_str(content);
  //setContentLength(this->body.len);
}

void MongooseHttpServerResponse::setContent(const uint8_t *content, size_t len)
{
  this->body = (char *)content;
  setContentLength(len);
  //this->body = mg_str_n((char *)content, len);
  //setContentLength(this->body.len);
}

const char * MongooseHttpServerResponse::getHeaderString()
{
  std::string h = "";

  for (mg_http_header header : this->headers)
  {
    h.append(header.name.ptr, header.name.len);
    h += ": ";
    h.append(header.value.ptr, header.value.len);
    h += "\r\n";
  }

  return h.c_str();
}

void MongooseHttpServerResponse::send(struct mg_connection *nc)
{
  DUMP(_contentLength);
  DUMP(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));

  if (false || _contentLength > heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT))
  {
    char temp[MG_IO_SIZE+1];
    temp[MG_IO_SIZE] = 0;
    size_t idx = 0;
    size_t remaining;

    mg_printf(nc, "HTTP/1.1 %u %s\r\n", _code, http_status_reason(_code));
    mg_printf(nc, getHeaderString());
    mg_printf(nc, "Content-Length: %u\r\n\r\n", _contentLength);

    for (idx = 0; idx<_contentLength; idx += MG_IO_SIZE)
    {
      remaining = _contentLength - idx;
      DUMP(remaining);
      if (remaining > MG_IO_SIZE)
      {
        strncpy(temp, body+idx, MG_IO_SIZE);
        temp[MG_IO_SIZE] = 0;
        mg_send(nc, temp, MG_IO_SIZE);
      }
      else
      {
        strncpy(temp, body+idx, remaining);
        temp[remaining] = 0;
        mg_send(nc, temp, remaining);
      }

      write_conn(nc);
    }
    
    nc->is_resp = 0;
    nc->is_draining = 1;
  }
  else
  {
    //mg_http_reply(nc, _code, getHeaderString(), body);
    mg_printf(nc, "HTTP/1.1 %u %s\r\n", _code, http_status_reason(_code));
    mg_printf(nc, getHeaderString());
    mg_printf(nc, "Content-Length: %u\r\n\r\n", _contentLength);
    mg_send(nc, body, _contentLength);
  }
}

#ifdef ARDUINO
MongooseHttpServerResponseStream::MongooseHttpServerResponseStream()
{
  mg_iobuf_init(&_content, ARDUINO_MONGOOSE_DEFAULT_STREAM_BUFFER, ARDUINO_MONGOOSE_DEFAULT_STREAM_BUFFER);
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

void MongooseHttpServerResponseStream::send(struct mg_connection *nc)
{
  const char *headers = getHeaderString();

  mg_http_reply(nc, _code, headers, body);
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