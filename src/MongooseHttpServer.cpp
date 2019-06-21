
#ifdef ARDUINO
#include <Arduino.h>
#else
#define utoa(i, buf, base) sprintf(buf, "%u", i)
#endif

#include <MicroDebug.h>

#include "MongooseCore.h"
#include "MongooseHttpServer.h"

#ifndef ARDUINO_MONGOOSE_PARAM_BUFFER_LENGTH
#define ARDUINO_MONGOOSE_PARAM_BUFFER_LENGTH 128
#endif

#ifndef ARDUINO_MONGOOSE_DEFAULT_STREAM_BUFFER
#define ARDUINO_MONGOOSE_DEFAULT_STREAM_BUFFER 128
#endif

#ifndef ARDUINO_MONGOOSE_SEND_BUFFER_SIZE
#define ARDUINO_MONGOOSE_SEND_BUFFER_SIZE 256
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

struct MongooseHttpServerHandler
{ 
  MongooseHttpServer *server;
  HttpRequestMethodComposite method;
  ArRequestHandlerFunction handler;
};

MongooseHttpServer::MongooseHttpServer() :
  nc(NULL)
{

}

MongooseHttpServer::~MongooseHttpServer()
{

}

bool MongooseHttpServer::begin(uint16_t port)
{
  char s_http_port[6];
  utoa(port, s_http_port, 10);
  nc = mg_bind(Mongoose.getMgr(), s_http_port, defaultEventHandler, this);
  if(nc)
  {
    // Set up HTTP server parameters
    mg_set_protocol_http_websocket(nc);

    return true;
  }

  return false;
}

#if MG_ENABLE_SSL
bool MongooseHttpServer::begin(uint16_t port, const char *cert, const char *private_key)
{
  char s_http_port[6];
  utoa(port, s_http_port, 10);

  struct mg_bind_opts bind_opts;
  const char *err;

  memset(&bind_opts, 0, sizeof(bind_opts));
  bind_opts.ssl_cert = cert;
  bind_opts.ssl_key = private_key;
  bind_opts.error_string = &err;

  nc = mg_bind_opt(Mongoose.getMgr(), s_http_port, defaultEventHandler, this, bind_opts);
  if(nc)
  {
    // Set up HTTP server parameters
    mg_set_protocol_http_websocket(nc);

    return true;
  }

  return false;
}
#endif

void MongooseHttpServer::on(const char* uri, ArRequestHandlerFunction onRequest)
{
  on(uri, HTTP_ANY, onRequest);
}

void MongooseHttpServer::on(const char* uri, HttpRequestMethodComposite method, ArRequestHandlerFunction onRequest)
{
  MongooseHttpServerHandler *handler = new MongooseHttpServerHandler;
  handler->server = this;
  handler->handler = onRequest;

  mg_register_http_endpoint(nc, uri, endpointEventHandler, handler);
}

void MongooseHttpServer::onNotFound(ArRequestHandlerFunction fn)
{
  fnNotFound = fn;
}

void MongooseHttpServer::defaultEventHandler(struct mg_connection *nc, int ev, void *p, void *u)
{
  MongooseHttpServer *self = (MongooseHttpServer *)u;
  self->eventHandler(nc, ev, p, HTTP_ANY, self->fnNotFound);
}

void MongooseHttpServer::endpointEventHandler(struct mg_connection *nc, int ev, void *p, void *u)
{
  MongooseHttpServerHandler *self = (MongooseHttpServerHandler *)u;
  self->server->eventHandler(nc, ev, p, self->method, self->handler);
}

void MongooseHttpServer::eventHandler(struct mg_connection *nc, int ev, void *p, HttpRequestMethodComposite method, ArRequestHandlerFunction onRequest)
{
  //DBUGF("Connection %p: %x", nc, ev);
  switch (ev) {
    case MG_EV_ACCEPT: {
      char addr[32];
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      DBUGF("Connection %p from %s", nc, addr);
      break;
    }
    case MG_EV_HTTP_REQUEST: {
      char addr[32];
      struct http_message *hm = (struct http_message *) p;
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      DBUGF("HTTP request from %s: %.*s %.*s", addr, (int) hm->method.len,
             hm->method.p, (int) hm->uri.len, hm->uri.p);

      if(nc->user_connection_data) {
        delete (MongooseHttpServerRequest *)nc->user_connection_data;
        nc->user_connection_data = NULL;
      }
      MongooseHttpServerRequest *request = new MongooseHttpServerRequest(this, nc, hm);
      if(request)
      {
        nc->user_connection_data = request;

        if(onRequest) {
          onRequest(request);
        } else {
          mg_http_send_error(nc, 404, NULL);
        }
      } else {
        mg_http_send_error(nc, 500, NULL);
      }
      break;
    }

    case MG_EV_POLL:
    case MG_EV_SEND:
    {
      if(nc->user_connection_data)
      {
        MongooseHttpServerRequest *request = (MongooseHttpServerRequest *)nc->user_connection_data;
        request->sendBody();
      }

      break;
    }

    case MG_EV_CLOSE: {
      DBUGF("Connection %p closed", nc);
      if(nc->user_connection_data) {
        delete (MongooseHttpServerRequest *)nc->user_connection_data;
        nc->user_connection_data = NULL;
      }
      break;
    }
  }
}


/// MongooseHttpServerRequest object

MongooseHttpServerRequest::MongooseHttpServerRequest(MongooseHttpServer *server, mg_connection *nc, http_message *msg) :
  _server(server),
  _nc(nc),
  _msg(msg),
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
}

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
}

void MongooseHttpServerRequest::sendBody()
{
  if(_response) 
  {
    size_t free = _nc->send_mbuf.size - _nc->send_mbuf.len;
    if(0 == _nc->send_mbuf.size) {
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

extern "C" const char *mg_status_message(int status_code);

void MongooseHttpServerRequest::send(int code)
{
  send(code, "text/plain", mg_status_message(code));
}

void MongooseHttpServerRequest::send(int code, const char *contentType, const char *content)
{
  char headers[64], *pheaders = headers;
  mg_asprintf(&pheaders, sizeof(headers), 
      "Connection: close\r\n"
      "Content-Type: %s",
      contentType);

  mg_send_head(_nc, code, strlen(content), pheaders);
  mg_send(_nc, content, strlen(content));
  _nc->flags |= MG_F_SEND_AND_CLOSE;

  if (pheaders != headers) free(pheaders);
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
  return -4 != getParam(name, dst, sizeof(dst));
}

#ifdef ARDUINO
bool MongooseHttpServerRequest::hasParam(const String& name) const
{
  char dst[8];
  return -4 != getParam(name, dst, sizeof(dst));
}
#endif

#ifdef ARDUINO
bool MongooseHttpServerRequest::hasParam(const __FlashStringHelper * data) const
{
  char dst[8];
  return -4 != getParam(data, dst, sizeof(dst));
}
#endif

int MongooseHttpServerRequest::getParam(const char *name, char *dst, size_t dst_len) const
{
  return mg_get_http_var((HTTP_GET == _method) ? (&_msg->query_string) : (&_msg->body), name, dst, dst_len);
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
  return true;
}

void MongooseHttpServerRequest::requestAuthentication(const char* realm)
{

}

MongooseHttpServerResponse::MongooseHttpServerResponse() :
  _code(200),
  _contentType("text/plain"),
  _contentLength(-1)
{

}

void MongooseHttpServerResponse::sendHeaders(struct mg_connection *nc)
{
  char headers[64], *pheaders = headers;
  mg_asprintf(&pheaders, sizeof(headers), 
      "Connection: close\r\n"
      "Content-Type: %s",
      _contentType);

  mg_send_head(nc, _code, _contentLength, pheaders);

  if (pheaders != headers) free(pheaders);
}

bool MongooseHttpServerResponse::addHeader(const char *name, const char *value) const
{
  return false;
}

#ifdef ARDUINO
bool MongooseHttpServerResponse::addHeader(const String& name, const String& value) const
{
  return false;
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
  size_t send = min(len, bytes);

  mg_send(nc, ptr, send);

  ptr += send;
  len -= send;

  if(0 == len) {
    nc->flags |= MG_F_SEND_AND_CLOSE;
  }

  return send;
}

#ifdef ARDUINO
MongooseHttpServerResponseStream::MongooseHttpServerResponseStream()
{
  mbuf_init(&_content, ARDUINO_MONGOOSE_DEFAULT_STREAM_BUFFER);
}

MongooseHttpServerResponseStream::~MongooseHttpServerResponseStream()
{
  mbuf_free(&_content);
}

size_t MongooseHttpServerResponseStream::write(const uint8_t *data, size_t len)
{
  size_t written = mbuf_append(&_content, data, len);
  setContentLength(_content.len);
  return written;
}

size_t MongooseHttpServerResponseStream::write(uint8_t data)
{
  return write(&data, 1);
}

size_t MongooseHttpServerResponseStream::sendBody(struct mg_connection *nc, size_t bytes)
{
  size_t send = min(_content.len, bytes);

  mg_send(nc, _content.buf, send);

  mbuf_remove(&_content, send);

  if(0 == _content.len) {
    nc->flags |= MG_F_SEND_AND_CLOSE;
  }

  return send;
}

#endif