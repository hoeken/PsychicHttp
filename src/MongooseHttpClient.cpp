#ifdef ARDUINO
#include <Arduino.h>
#else
#define utoa(i, buf, base) sprintf(buf, "%u", i)
#endif

#include <MicroDebug.h>

#include "MongooseCore.h"
#include "MongooseHttpClient.h"

MongooseHttpClient::MongooseHttpClient() 
{

}

MongooseHttpClient::~MongooseHttpClient()
{

}

void MongooseHttpClient::eventHandler(struct mg_connection *nc, int ev, void *p, void *u)
{
  MongooseHttpClientRequest *request = (MongooseHttpClientRequest *)u;
  request->_client ->eventHandler(nc, request, ev, p);
}

void MongooseHttpClient::eventHandler(struct mg_connection *nc, MongooseHttpClientRequest *request, int ev, void *p)
{
  //DBUGF("Connection %p: %x", nc, ev);
  switch (ev) {
    case MG_EV_ACCEPT: {
      char addr[32];
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      DBUGF("Connection %p to %s", nc, addr);
      break;
    }
    case MG_EV_HTTP_REPLY: {
      char addr[32];
      struct http_message *hm = (struct http_message *) p;
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      DBUGF("HTTP reply from %s", addr);

      if(nc->user_connection_data) {
        delete (MongooseHttpClientResponse *)nc->user_connection_data;
        nc->user_connection_data = NULL;
      }
      MongooseHttpClientResponse *response = new MongooseHttpClientResponse(hm);
      if(response)
      {
        nc->user_connection_data = response;
        request->_onResponse(response);
      }
      break;
    }

//    case MG_EV_POLL:
//    case MG_EV_SEND:
//    {
//      if(nc->user_connection_data)
//      {
//        MongooseHttpClientResponse *request = (MongooseHttpClientResponse *)nc->user_connection_data;
//        request->sendBody();
//      }
//
//      break;
//    }

    case MG_EV_CLOSE: {
      DBUGF("Connection %p closed", nc);
      if(nc->user_connection_data) {
        delete (MongooseHttpClientResponse *)nc->user_connection_data;
        nc->user_connection_data = NULL;
      }
      break;
    }
  }
}

void MongooseHttpClient::get(const char *uri, MongooseHttpResponseHandler onResponse)
{
  MongooseHttpClientRequest *request = beginRequest(uri);
  request->setMethod(HTTP_GET);
  request->onResponse(onResponse);
  send(request);
}

void MongooseHttpClient::post(const char* uri, const char *contentType, const char *body, MongooseHttpResponseHandler onResponse)
{
  MongooseHttpClientRequest *request = beginRequest(uri);
  request->setMethod(HTTP_POST);
  request->setContentType(contentType);
  request->setContent(body);
  request->onResponse(onResponse);
  send(request);
}

MongooseHttpClientRequest *MongooseHttpClient::beginRequest(const char *uri)
{
  return new MongooseHttpClientRequest(this, uri);
}

void MongooseHttpClient::send(MongooseHttpClientRequest *request)
{
  struct mg_connect_opts opts;
  memset(&opts, 0, sizeof(opts));
  const char *err;
  opts.error_string = &err;
  mg_connection *nc = mg_connect_http_opt(Mongoose.getMgr(), eventHandler, request, opts, request->_uri, NULL, (const char *)request->_body);
  if(!nc) {
    DBUGF("Failed to connect to %s: %s", request->_uri, err);
  }
}

MongooseHttpClientRequest::MongooseHttpClientRequest(MongooseHttpClient *client, const char *uri) :
  _client(client), _onResponse(NULL), _uri(uri), _method(HTTP_GET),
  _contentType("application/x-www-form-urlencoded"), _contentLength(-1),
  _body(NULL)
{

}

void MongooseHttpClientRequest::setContent(const uint8_t *content, size_t len)
{
  setContentLength(len);
  _body = content;
}

bool MongooseHttpClientRequest::addHeader(const char *name, const char *value)
{
  return false;
}
