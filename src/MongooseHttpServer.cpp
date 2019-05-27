#include <Arduino.h>
#include <MicroDebug.h>

#include "MongooseCore.h"
#include "MongooseHttpServer.h"

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

}

MongooseHttpServerRequest::~MongooseHttpServerRequest()
{

}

void MongooseHttpServerRequest::redirect(const String& url)
{

}

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

bool MongooseHttpServerRequest::hasParam(const char *name) const
{
  return false;
}

bool MongooseHttpServerRequest::hasParam(const String& name) const
{
  return false;
}

bool MongooseHttpServerRequest::hasParam(const __FlashStringHelper * data) const
{
  return false;
}

bool MongooseHttpServerRequest::getParam(const char *name, char *dst, size_t dst_len) const
{
  return false;
}

bool MongooseHttpServerRequest::getParam(const String& name, char *dst, size_t dst_len) const
{
  return false;
}

bool MongooseHttpServerRequest::getParam(const __FlashStringHelper * data, char *dst, size_t dst_len) const
{
  return false;
}

String MongooseHttpServerRequest::getParam(const char *name) const
{
  return "";
}

String MongooseHttpServerRequest::getParam(const String& name) const
{
  return "";
}

String MongooseHttpServerRequest::getParam(const __FlashStringHelper * data) const
{
  return "";
}
