
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

      if(onRequest) {
        MongooseHttpServerRequest request(this, nc, hm);
        onRequest(&request);
      } else {
        mg_http_send_error(nc, 404, NULL);
      }
      nc->flags |= MG_F_SEND_AND_CLOSE;
      break;
    }
    case MG_EV_CLOSE: {
      DBUGF("Connection %p closed", nc);
      break;
    }
  }
}


/// MongooseHttpServerRequest object

MongooseHttpServerRequest::MongooseHttpServerRequest(MongooseHttpServer *server, mg_connection *nc, http_message *msg) :
  _server(server),
  _nc(nc),
  _msg(msg)
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

MongooseHttpServerResponse *MongooseHttpServerRequest::beginResponse(const char *contentType)
{
  return NULL;
}

MongooseHttpServerResponse *MongooseHttpServerRequest::beginResponse(int code, const char *contentType, const char *content)
{
  return NULL;
}

#ifdef ARDUINO
MongooseHttpServerResponse *MongooseHttpServerRequest::beginResponse(const String& contentType)
{
  return beginResponse(contentType.c_str());
}

MongooseHttpServerResponse *MongooseHttpServerRequest::beginResponse(int code, const String& contentType, const String& content)
{
  return beginResponse(code, contentType.c_str(), content.c_str());
}

MongooseHttpServerResponseStream *MongooseHttpServerRequest::beginResponseStream(const char *contentType)
{
  return NULL;
}

MongooseHttpServerResponseStream *MongooseHttpServerRequest::beginResponseStream(const String& contentType)
{
  return beginResponseStream(contentType.c_str());
}

#endif

void MongooseHttpServerRequest::send(MongooseHttpServerResponse *response)
{

}

void MongooseHttpServerRequest::send(int code)
{
  send(code, "text/plain", "");
}

void MongooseHttpServerRequest::send(int code, const char *contentType, const char *content)
{
  static const char *reply_fmt =
      "HTTP/1.1 %d %s\r\n"
      "Connection: close\r\n"
      "Content-Type: %s\r\n"
      "\r\n"
      "%s\n";

  mg_printf(_nc, reply_fmt, code, "", contentType, content);
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


void MongooseHttpServerResponse::setCode(int code)
{

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

#ifdef ARDUINO
size_t MongooseHttpServerResponseStream::write(const uint8_t *data, size_t len)
{
  return len;
}

size_t MongooseHttpServerResponseStream::write(uint8_t data)
{
  return 1;
}

#endif