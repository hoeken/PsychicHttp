#include <Arduino.h>
#include <MicroDebug.h>

#include "MongooseCore.h"
#include "MongooseHttpServer.h"


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
  nc = mg_bind(Mongoose.getMgr(), s_http_port, eventHandler, this);

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

}

void MongooseHttpServer::on(const char* uri, HttpRequestMethodComposite method, ArRequestHandlerFunction onRequest)
{

}

void MongooseHttpServer::onNotFound(ArRequestHandlerFunction fn)
{

}

void MongooseHttpServer::eventHandler(struct mg_connection *nc, int ev, void *p, void *u)
{
  MongooseHttpServer *self = (MongooseHttpServer *)u;
  self->eventHandler(nc, ev, p);
}

void MongooseHttpServer::eventHandler(struct mg_connection *nc, int ev, void *p)
{
  static const char *reply_fmt =
      "HTTP/1.0 200 OK\r\n"
      "Connection: close\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "Hello %s\n";

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
      mg_printf(nc, reply_fmt, addr);
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

MongooseHttpServerRequest::MongooseHttpServerRequest(MongooseHttpServer*)
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

}

bool MongooseHttpServerRequest::hasParam(const char *name) const
{

}

bool MongooseHttpServerRequest::hasParam(const String& name) const
{

}

bool MongooseHttpServerRequest::hasParam(const __FlashStringHelper * data) const
{

}

bool MongooseHttpServerRequest::getParam(const char *name, char *dst, size_t dst_len) const
{

}

bool MongooseHttpServerRequest::getParam(const String& name, char *dst, size_t dst_len) const
{

}

bool MongooseHttpServerRequest::getParam(const __FlashStringHelper * data, char *dst, size_t dst_len) const
{

}

String MongooseHttpServerRequest::getParam(const char *name) const
{

}

String MongooseHttpServerRequest::getParam(const String& name) const
{

}

String MongooseHttpServerRequest::getParam(const __FlashStringHelper * data) const
{

}
