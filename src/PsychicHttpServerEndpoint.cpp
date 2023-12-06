#include "PsychicCore.h"
#include "PsychicHttpServerEndpoint.h"
#include "PsychicHttpWebsocket.h"

PsychicHttpServerEndpoint::PsychicHttpServerEndpoint() :
  _server(NULL),
  _method(HTTP_GET),
  _uri(""),
  _wsConnectCallback(NULL),
  _wsFrameCallback(NULL),
  isUpload(false),
  isWebsocket(false)
{
}

PsychicHttpServerEndpoint::PsychicHttpServerEndpoint(PsychicHttpServer *server, http_method method, const char * uri) :
  _server(server),
  _method(method),
  _uri(uri),
  _handler(NULL),
  _wsConnectCallback(NULL),
  _wsFrameCallback(NULL),
  isUpload(false),
  isWebsocket(false)
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

PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::onConnect(PsychicHttpWebSocketRequestHandler handler) {
  this->_wsConnectCallback = handler;
  this->isWebsocket = true;
  return this;
}

PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::onFrame(PsychicHttpWebSocketFrameHandler handler) {
  this->_wsFrameCallback = handler;
  this->isWebsocket = true;
  return this;
}

PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::onClose(PsychicHttpConnectionHandler handler) {
  this->_wsCloseCallback = handler;
  this->isWebsocket = true;
  return this;
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

  // if (self->isWebsocket)
  // {
  //   PsychicHttpWebSocketRequest connection(self->_server, req);
  //   err = self->_websocketHandler(connection);
  // }
  // else
  // {
  //   PsychicHttpServerRequest request(self->_server, req);

  //   //is this a file upload?
  //   if (self->isUpload)
  //     err = self->_uploadHandler(request);
  //   //no, its a regular request
  //   else
  //     err = self->_requestHandler(request);
  // }

  return err;
}

esp_err_t PsychicHttpServerEndpoint::_websocketHandler(PsychicHttpWebSocketRequest &request)
{
  // beginning of the ws URI handler and our onConnect hook
  if (request._req->method == HTTP_GET)
  {
    if (this->_wsConnectCallback != NULL)  
      this->_wsConnectCallback(&request);

    //add it to our list of connections
    this->websocketConnections.push_back(httpd_req_to_sockfd(request._req));

    return ESP_OK;
  }

  //init our memory for storing the packet
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;
  uint8_t *buf = NULL;

  /* Set max_len = 0 to get the frame len */
  esp_err_t ret = httpd_ws_recv_frame(request._req, &ws_pkt, 0);
  if (ret != ESP_OK) {
    ESP_LOGE(PH_TAG, "httpd_ws_recv_frame failed to get frame len with %s", esp_err_to_name(ret));
    return ret;
  }

  //okay, now try to load the packet
  ESP_LOGI(PH_TAG, "frame len is %d", ws_pkt.len);
  if (ws_pkt.len) {
    /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
    buf = (uint8_t*) calloc(1, ws_pkt.len + 1);
    if (buf == NULL) {
      ESP_LOGE(PH_TAG, "Failed to calloc memory for buf");
      return ESP_ERR_NO_MEM;
    }
    ws_pkt.payload = buf;
    /* Set max_len = ws_pkt.len to get the frame payload */
    ret = httpd_ws_recv_frame(request._req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK) {
      ESP_LOGE(PH_TAG, "httpd_ws_recv_frame failed with %s", esp_err_to_name(ret));
      free(buf);
      return ret;
    }
    ESP_LOGI(PH_TAG, "Got packet with message: %s", ws_pkt.payload);
  }

  // Text messages are our payload.
  if (ws_pkt.type == HTTPD_WS_TYPE_TEXT)
  {
    if (this->_wsFrameCallback != NULL)
      ret = this->_wsFrameCallback(&request, &ws_pkt);
  }

  //logging housekeeping
  if (ret != ESP_OK)
    ESP_LOGE(PH_TAG, "httpd_ws_send_frame failed with %s", esp_err_to_name(ret));
  ESP_LOGI(PH_TAG, "ws_handler: httpd_handle_t=%p, sockfd=%d, client_info:%d", req->handle,
    httpd_req_to_sockfd(req), httpd_ws_get_fd_info(req->handle, httpd_req_to_sockfd(req)));

  //dont forget to release our buffer memory
  free(buf);

  return ret;
}