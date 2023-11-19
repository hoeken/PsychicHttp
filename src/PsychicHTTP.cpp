#include "PsychicHTTP.h"

PsychicHTTPServer::PsychicHTTPServer() :
  nc(NULL),
  defaultEndpoint(this, HTTP_ANY)
{

}

PsychicHTTPServer::~PsychicHTTPServer()
{

}

void PsychicHTTPServer::begin(uint16_t port)
{
  char url[30];
  sprintf(url, "http://0.0.0.0:%u", port);

  this->nc = mg_http_listen(Mongoose.getMgr(), url, eventHandler, this);
}

void PsychicHTTPServer::begin(uint16_t port, const char *cert, const char *private_key)
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

PsychicHTTPServerEndpoint *PsychicHTTPServer::on(const char* uri)
{
  return on(uri, HTTP_ANY);
}

PsychicHTTPServerEndpoint *PsychicHTTPServer::on(const char* uri, PsychicHTTPRequestHandler onRequest)
{
  return on(uri, HTTP_ANY)->onRequest(onRequest);
}

PsychicHTTPServerEndpoint *PsychicHTTPServer::on(const char* uri, HttpRequestMethodComposite method, PsychicHTTPRequestHandler onRequest)
{
  return on(uri, method)->onRequest(onRequest);
}

PsychicHTTPServerEndpoint *PsychicHTTPServer::on(const char* uri, HttpRequestMethodComposite method)
{
  PsychicHTTPServerEndpoint *handler = new PsychicHTTPServerEndpoint(this, method);

  //save our uri
  handler->uri = new MongooseString(uri);

  //keep a list of our handlers
  endpoints.push_back(handler);

  return handler;
}

//TODO: add this to the end of our http event handler
void PsychicHTTPServer::onNotFound(PsychicHTTPRequestHandler fn)
{
  defaultEndpoint.onRequest(fn);
}

void PsychicHTTPServer::eventHandler(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  PsychicHTTPServer *self = (PsychicHTTPServer *)fn_data;
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
    //   //         PsychicHTTPServerRequest *request = (PsychicHTTPServerRequest *)nc->user_data;
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
      //   PsychicHTTPServerRequest *request = (PsychicHTTPServerRequest *)c->fn_data;
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
      for (PsychicHTTPServerEndpoint* endpoint : self->endpoints)
      {
        if(mg_http_match_uri(hm, endpoint->uri->c_str()))
        {
          Serial.printf("%s matches\n", endpoint->uri->c_str());

          if (endpoint->request != NULL)
          {
            found = true;
            PsychicHTTPServerRequest* request = new PsychicHTTPServerRequest(self, c, hm);
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
        PsychicHTTPServerRequest* request = new PsychicHTTPServerRequest(self, c, hm);
        self->defaultEndpoint.request(request);
      }

      break;
    }

    // Websocket handshake done     struct mg_http_message *
    case MG_EV_WS_OPEN: {
      Serial.println("MG_EV_WS_OPEN");
      hm = (struct mg_http_message *) ev_data;

      //TODO: update this
      // if(endpoint->wsConnect && nc->flags & MG_F_IS_PsychicHTTPWebSocketConnection)
      // {
      //   PsychicHTTPWebSocketConnection *c = (PsychicHTTPWebSocketConnection *)nc->user_data;
      //   endpoint->wsConnect(c);
      // }

      break;
    }

    // Websocket msg, text or bin   struct mg_ws_message *
    case MG_EV_WS_MSG: {
      Serial.println("MG_EV_WS_MSG");
      // Got websocket frame. Received data is wm->data. Echo it back!
      struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
//       if(endpoint->wsFrame && nc->flags & MG_F_IS_PsychicHTTPWebSocketConnection)
//       {
//         PsychicHTTPWebSocketConnection *c = (PsychicHTTPWebSocketConnection *)nc->user_data;
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

void PsychicHTTPServer::sendAll(PsychicHTTPWebSocketConnection *from, const char *endpoint, int op, const void *data, size_t len)
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
      PsychicHTTPWebSocketConnection *to = (PsychicHTTPWebSocketConnection *)c->fn_data;
      if(endpoint && !to->uri().equals(endpoint)) {
        continue;
      }
      DBUGF("%.*s sending to %p", to->uri().length(), to->uri().c_str(), to);
      to->send(op, data, len);
    }
  }
}

PsychicHTTPServerRequest::PsychicHTTPServerRequest(PsychicHTTPServer *server, mg_connection *nc, mg_http_message *msg) :
  _server(server),
  _nc(nc),
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

PsychicHTTPServerRequest::~PsychicHTTPServerRequest()
{
  if(_response) {
    delete _response;
    _response = NULL;
  }

}

void PsychicHTTPServerRequest::redirect(const char *url)
{

}

PsychicHTTPServerResponse *PsychicHTTPServerRequest::beginResponse()
{
  return new PsychicHTTPServerResponse();
}

void PsychicHTTPServerRequest::send(PsychicHTTPServerResponse *response)
{
  response->send(this->_nc);
}

//what is this?
void PsychicHTTPServerRequest::send(int code)
{
  send(code, "text/plain", http_status_reason(code));
}

void PsychicHTTPServerRequest::send(int code, const char *contentType, const char *content)
{
  PsychicHTTPServerResponse *response = this->beginResponse();

  response->setCode(code);
  response->setContentType(contentType);
  response->setContent(content);
  
  this->send(response);
}

// IMPROVE: add a function to Mongoose to do this
bool PsychicHTTPServerRequest::hasParam(const char *name) const
{
  char dst[8];
  int ret = getParam(name, dst, sizeof(dst));
  return ret >= 0 || -3 == ret; 
}


int PsychicHTTPServerRequest::getParam(const char *name, char *dst, size_t dst_len) const
{
  return mg_http_get_var((HTTP_GET == _method) ? (&_msg->query) : (&_msg->body), name, dst, dst_len);
}

bool PsychicHTTPServerRequest::authenticate(const char * username, const char * password)
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

void PsychicHTTPServerRequest::requestAuthentication(const char* realm)
{
  // https://github.com/me-no-dev/ESPAsyncWebServer/blob/master/src/WebRequest.cpp#L852
  // mg_http_send_digest_auth_request

  char headers[64];
  mg_snprintf(headers, sizeof(headers), 
      "WWW-Authenticate: Basic realm=%s",
      realm);

  mg_http_reply(_nc, 401, headers, "", NULL);
}



PsychicHTTPServerResponse::PsychicHTTPServerResponse() :
  _code(200)
{

}

PsychicHTTPServerResponse::~PsychicHTTPServerResponse()
{
}

void PsychicHTTPServerResponse::addHeader(const char *name, const char *value)
{
  mg_http_header header;
  header.name = mg_str(name);
  header.value = mg_str(value);
  headers.push_back(header);
}

#ifdef ARDUINO
void PsychicHTTPServerResponse::addHeader(const String& name, const String& value)
{
  return addHeader(name.c_str(), value.c_str());
}
#endif

void PsychicHTTPServerResponse::setContentType(const char *contentType)
{
  addHeader("Content-Type", contentType);
}

void PsychicHTTPServerResponse::setContent(const char *content)
{
  this->body = content;
  setContentLength(strlen(content));
}

void PsychicHTTPServerResponse::setContent(const uint8_t *content, size_t len)
{
  this->body = (char *)content;
  setContentLength(len);
}

const char * PsychicHTTPServerResponse::getHeaderString()
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

void PsychicHTTPServerResponse::send(struct mg_connection *nc)
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
PsychicHTTPServerResponseStream::PsychicHTTPServerResponseStream()
{
  mg_iobuf_init(&_content, ARDUINO_MONGOOSE_DEFAULT_STREAM_BUFFER, ARDUINO_MONGOOSE_DEFAULT_STREAM_BUFFER);
}

PsychicHTTPServerResponseStream::~PsychicHTTPServerResponseStream()
{
  mg_iobuf_free(&_content);
}

size_t PsychicHTTPServerResponseStream::write(const uint8_t *data, size_t len)
{
  size_t written = mg_iobuf_add(&_content, _content.len, data, len);
  setContentLength(_content.len);
  return written;
}

size_t PsychicHTTPServerResponseStream::write(uint8_t data)
{
  return write(&data, 1);
}

void PsychicHTTPServerResponseStream::send(struct mg_connection *nc)
{
  const char *headers = getHeaderString();

  mg_http_reply(nc, _code, headers, body);
}

#endif

PsychicHTTPWebSocketConnection::~PsychicHTTPWebSocketConnection()
{

}

void PsychicHTTPWebSocketConnection::send(int op, const void *data, size_t len)
{
  mg_ws_send(_nc, data, len, op);
}