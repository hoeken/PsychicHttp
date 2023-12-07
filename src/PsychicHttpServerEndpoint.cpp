#include "PsychicHttpServerEndpoint.h"

PsychicHttpServerEndpoint::PsychicHttpServerEndpoint() :
  _server(NULL),
  _method(HTTP_GET),
  _uri(""),
  _handler(NULL)
{
}

PsychicHttpServerEndpoint::PsychicHttpServerEndpoint(PsychicHttpServer *server, http_method method, const char * uri) :
  _server(server),
  _method(method),
  _uri(uri),
  _handler(NULL)
{
}

PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::setHandler(PsychicHandler *handler)
{
  //clean up old / default handler
  if (_handler != NULL)
    delete _handler;

  _handler = handler;
  return this;
}

PsychicHandler * PsychicHttpServerEndpoint::handler()
{
  return _handler;
}

String PsychicHttpServerEndpoint::uri() {
  return _uri;
}

esp_err_t PsychicHttpServerEndpoint::requestCallback(httpd_req_t *req)
{
  esp_err_t err = ESP_OK;

  #ifdef ENABLE_ASYNC
    if (is_on_async_worker_thread() == false) {
      // submit
      if (submit_async_req(req, PsychicHttpServerEndpoint::requestCallback) == ESP_OK) {
        return ESP_OK;
      } else {
        httpd_resp_set_status(req, "503 Busy");
        httpd_resp_sendstr(req, "No workers available. Server busy.</div>");
        return ESP_OK;
      }
    }
  #endif

  PsychicHttpServerEndpoint *self = (PsychicHttpServerEndpoint *)req->user_ctx;
  PsychicHandler *handler = self->handler();
  PsychicHttpServerRequest request(self->_server, req);

  //make sure we have a handler
  if (handler != NULL)
  {
    // if((_username != "" && _password != "") && !request->authenticate(_username.c_str(), _password.c_str()))
    //   return request->requestAuthentication();

    if (handler->filter(&request) && handler->canHandle(&request))
    {
      err = handler->handleRequest(&request);
      if (err != ESP_OK)
        return err;

      //TODO: possibly add middleware parsing here?
      //err = request->middleWare();
      // if (err != ESP_OK)
      //   return err;
      //err = request->sendResponse();
    }
    else
      return request.reply(500, "text/html", "Handler cannot handle.");
  }
  else
    return request.reply(500, "text/html", "No handler registered.");

  return err;
}