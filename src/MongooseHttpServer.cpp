
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

void MongooseHttpServer::begin(uint16_t port)
{
  char s_http_port[6];
  utoa(port, s_http_port, 10);
  nc = mg_bind(Mongoose.getMgr(), s_http_port, defaultEventHandler, this);

  // Set up HTTP server parameters
  mg_set_protocol_http_websocket(nc);
}

#if MG_ENABLE_SSL
void MongooseHttpServer::begin(uint16_t port, const char *cert, const char *private_key_file, const char *password)
{

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
  static const char *reply_fmt =
      "HTTP/1.1 %d %s\r\n"
      "Connection: close\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "%s\n";

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
        mg_printf(nc, reply_fmt, 404, "Not Found", "File not found");
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
  server(server),
  nc(nc),
  msg(msg)
{
  if(0 == mg_vcasecmp(&msg->method, "GET")) {
    method = HTTP_GET;
  } else if(0 == mg_vcasecmp(&msg->method, "POST")) {
    method = HTTP_POST;
  } else if(0 == mg_vcasecmp(&msg->method, "DELETE")) {
    method = HTTP_DELETE;
  } else if(0 == mg_vcasecmp(&msg->method, "PUT")) {
    method = HTTP_PUT;
  } else if(0 == mg_vcasecmp(&msg->method, "PATCH")) {
    method = HTTP_PATCH;
  } else if(0 == mg_vcasecmp(&msg->method, "HEAD")) {
    method = HTTP_HEAD;
  } else if(0 == mg_vcasecmp(&msg->method, "OPTIONS")) {
    method = HTTP_OPTIONS;
  }
}

MongooseHttpServerRequest::~MongooseHttpServerRequest()
{

}

#ifdef ARDUINO
void MongooseHttpServerRequest::redirect(const String& url)
{

}
#endif

#ifdef ARDUINO
void MongooseHttpServerRequest::send(int code, const String& contentType, const String& content)
{
  static const char *reply_fmt =
      "HTTP/1.1 %d %s\r\n"
      "Connection: close\r\n"
      "Content-Type: %s\r\n"
      "\r\n"
      "%s\n";

  mg_printf(nc, reply_fmt, code, "", contentType.c_str(), content.c_str());
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
  return mg_get_http_var((HTTP_GET == method) ? (&msg->query_string) : (&msg->body), name, dst, dst_len);
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
