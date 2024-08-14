#include "PsychicEndpoint.h"
#include "PsychicHttpServer.h"

PsychicEndpoint::PsychicEndpoint() : _server(NULL),
                                     _uri(""),
                                     _method(HTTP_GET),
                                     _handler(NULL)
{
}

PsychicEndpoint::PsychicEndpoint(PsychicHttpServer* server, int method, const char* uri) : _server(server),
                                                                                           _uri(uri),
                                                                                           _method(method),
                                                                                           _handler(NULL)
{
}

PsychicEndpoint* PsychicEndpoint::setHandler(PsychicHandler* handler)
{
  // clean up old / default handler
  if (_handler != NULL)
    delete _handler;

  // get our new pointer
  _handler = handler;

  // keep a pointer to the server
  _handler->_server = _server;

  return this;
}

PsychicHandler* PsychicEndpoint::handler()
{
  return _handler;
}

String PsychicEndpoint::uri()
{
  return _uri;
}

esp_err_t PsychicEndpoint::requestCallback(httpd_req_t* req)
{
#ifdef ENABLE_ASYNC
  if (is_on_async_worker_thread() == false) {
    if (submit_async_req(req, PsychicEndpoint::requestCallback) == ESP_OK) {
      return ESP_OK;
    } else {
      httpd_resp_set_status(req, "503 Busy");
      httpd_resp_sendstr(req, "No workers available. Server busy.</div>");
      return ESP_OK;
    }
  }
#endif

  PsychicEndpoint* self = (PsychicEndpoint*)req->user_ctx;
  PsychicHandler* handler = self->handler();
  PsychicRequest request(self->_server, req);
  PsychicResponse response(&request);

  esp_err_t err = self->process(&request, &response);

  if (err == ESP_ERR_NOT_SUPPORTED) {
    return PsychicHttpServer::requestHandler(req);
  }

  return err;
}

bool PsychicEndpoint::matches(const char* uri)
{
  // we only want to match the path, no GET strings
  char* ptr;
  size_t position = 0;

  // look for a ? and set our path length to that,
  ptr = strchr(uri, '?');
  if (ptr != NULL)
    position = (size_t)(int)(ptr - uri);
  // or use the whole uri if not found
  else
    position = strlen(uri);

  // do we have a per-endpoint match function
  if (this->getURIMatchFunction() != NULL) {
    // ESP_LOGD(PH_TAG, "Match? %s == %s (%d)", _uri.c_str(), uri, position);
    return this->getURIMatchFunction()(_uri.c_str(), uri, (size_t)position);
  }
  // do we have a global match function
  if (_server->getURIMatchFunction() != NULL) {
    // ESP_LOGD(PH_TAG, "Match? %s == %s (%d)", _uri.c_str(), uri, position);
    return _server->getURIMatchFunction()(_uri.c_str(), uri, (size_t)position);
  } else {
    ESP_LOGE(PH_TAG, "No uri matching function set");
    return false;
  }
}

httpd_uri_match_func_t PsychicEndpoint::getURIMatchFunction()
{
  return _uri_match_fn;
}

void PsychicEndpoint::setURIMatchFunction(httpd_uri_match_func_t match_fn)
{
  _uri_match_fn = match_fn;
}

PsychicEndpoint* PsychicEndpoint::setFilter(PsychicRequestFilterFunction fn)
{
  _handler->setFilter(fn);
  return this;
}

PsychicEndpoint* PsychicEndpoint::addMiddleware(PsychicMiddleware* middleware)
{
  _handler->addMiddleware(middleware);
  return this;
}

PsychicEndpoint* PsychicEndpoint::addMiddleware(PsychicMiddlewareFunction fn)
{
  _handler->addMiddleware(fn);
  return this;
}

bool PsychicEndpoint::removeMiddleware(PsychicMiddleware* middleware)
{
  return _handler->removeMiddleware(middleware);
}

esp_err_t PsychicEndpoint::process(PsychicRequest* request, PsychicResponse* response)
{
  // make sure we have a handler
  if (_handler != NULL) {
    if (_handler->canHandle(request)) {
      return _handler->process(request, response);
    }
    // pass it to our generic handlers
    else
      return ESP_ERR_NOT_SUPPORTED;
  } else
    return request->reply(500, "text/html", "No handler registered.");

  return ESP_ERR_HTTPD_INVALID_REQ;
}
