#if defined(ENABLE_DEBUG) && !defined(ENABLE_DEBUG_MONGOOSE_HTTP_CLIENT)
#undef ENABLE_DEBUG
#endif

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
  if (ev != MG_EV_POLL) { DBUGF("%s %p: %d", __PRETTY_FUNCTION__, nc, ev); }

  switch (ev)
  {
    case MG_EV_CONNECT: {
      int connect_status = *(int *)p;
      DBUGVAR(connect_status);
      if(0 != connect_status) {
        DBUGF("connect() error: %s\n", strerror(connect_status));
      }
      break;
    }

    case MG_EV_RECV: {
      int num_bytes = *(int *)p;
      DBUGF("MG_EV_RECV, bytes = %d", num_bytes);
      //struct mbuf &io = nc->recv_mbuf;
      //DBUGF("Buffer %p, len %d: \n%.*s", io.buf, io.len, io.len, io.buf);
      break;
    }

    case MG_EV_SEND: {
      int num_bytes = *(int *)p;
      DBUGF("MG_EV_SEND, bytes = %d", num_bytes);
      break;
    }

    case MG_EV_HTTP_CHUNK:
    case MG_EV_HTTP_REPLY:
    {
      char addr[32];
      struct http_message *hm = (struct http_message *) p;
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      DBUGF("HTTP %s from %s, body %zu @ %p",
        MG_EV_HTTP_REPLY == ev ? "reply" : "chunk",
        addr, hm->body.len, hm->body.p);

      MongooseHttpClientResponse response(hm);
      if(MG_EV_HTTP_CHUNK == ev)
      {
        if(request->_onBody) {
          request->_onBody(&response);
          nc->flags |= MG_F_DELETE_CHUNK;
        }
      } else {
        if(request->_onResponse) {
          request->_onResponse(&response);
        }
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      }

      break;
    }

    case MG_EV_CLOSE: {
      DBUGF("Connection %p closed", nc);
      if(request->_onClose) {
        request->_onClose();
      }
      delete request;
      break;
    }
  }
}

void MongooseHttpClient::get(const char *uri, MongooseHttpResponseHandler onResponse, MongooseHttpCloseHandler onClose)
{
  MongooseHttpClientRequest *request = beginRequest(uri);
  request->setMethod(HTTP_GET);
  if(NULL != onResponse) {
    request->onResponse(onResponse);
  }
  if(NULL != onClose) {
    request->onClose(onClose);
  }
  send(request);
}

void MongooseHttpClient::post(const char* uri, const char *contentType, const char *body, MongooseHttpResponseHandler onResponse, MongooseHttpCloseHandler onClose)
{
  MongooseHttpClientRequest *request = beginRequest(uri);
  request->setMethod(HTTP_POST);
  request->setContentType(contentType);
  request->setContent(body);
  if(NULL != onResponse) {
    request->onResponse(onResponse);
  }
  if(NULL != onClose) {
    request->onClose(onClose);
  }
  send(request);
}

MongooseHttpClientRequest *MongooseHttpClient::beginRequest(const char *uri)
{
  return new MongooseHttpClientRequest(this, uri);
}

void MongooseHttpClient::send(MongooseHttpClientRequest *request)
{
  struct mg_connect_opts opts;
  Mongoose.getDefaultOpts(&opts);

  const char *err;
  opts.error_string = &err;

  mg_connection *nc = mg_connect_http_opt(Mongoose.getMgr(), eventHandler, request, opts, request->_uri, request->_extraHeaders, (const char *)request->_body);
  if(nc) {
    request->_nc = nc;
  } else {
    DBUGF("Failed to connect to %s: %s", request->_uri, err);
  }
}

MongooseHttpClientRequest::MongooseHttpClientRequest(MongooseHttpClient *client, const char *uri) :
  _client(client),
  _onResponse(NULL),
  _onBody(NULL),
  _onClose(NULL),
  _uri(uri),
  _method(HTTP_GET),
  _contentType("application/x-www-form-urlencoded"),
  _contentLength(-1),
  _body(NULL),
  _extraHeaders(NULL)
{

}


MongooseHttpClientRequest::~MongooseHttpClientRequest()
{
  if (_extraHeaders) {
    free(_extraHeaders);
    _extraHeaders = NULL;
  }
}

MongooseHttpClientRequest *MongooseHttpClientRequest::setContent(const uint8_t *content, size_t len)
{
  setContentLength(len);
  _body = content;
  return this;
}

bool MongooseHttpClientRequest::addHeader(const char *name, size_t nameLength, const char *value, size_t valueLength)
{
  size_t startLen = _extraHeaders ? strlen(_extraHeaders) : 0;
  size_t newLen = sizeof(": \r\n");
  newLen += nameLength;
  newLen += valueLength;
  size_t len = startLen + newLen;

  char * newBuffer = (char *)realloc(_extraHeaders, len);
  if(newBuffer)
  {
    snprintf(newBuffer + startLen, newLen, "%.*s: %.*s\r\n", (int)nameLength, name, (int)valueLength, value);
    _extraHeaders = newBuffer;
    return true;
  }

  return false;
}

void MongooseHttpClientRequest::abort()
{
  if (_nc) {
    _nc->flags |= MG_F_CLOSE_IMMEDIATELY;
  }
}

size_t MongooseHttpClientResponse::contentLength() {
  MongooseString content_length = headers("Content-Length");
  if(content_length != NULL) {
    return atoll(content_length.c_str());
  }
  return _msg->body.len;
}
