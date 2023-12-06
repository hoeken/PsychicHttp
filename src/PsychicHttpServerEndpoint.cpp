#include "PsychicCore.h"
#include "PsychicHttpServerEndpoint.h"
#include "PsychicHttpWebsocket.h"

PsychicHttpServerEndpoint::PsychicHttpServerEndpoint() :
  _server(NULL),
  _method(HTTP_GET),
  _uri(""),
  _uploadCallback(NULL),
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
  _uploadCallback(NULL),
  _wsConnectCallback(NULL),
  _wsFrameCallback(NULL),
  isUpload(false),
  isWebsocket(false)
{
}

PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::setHandler(PsychicHandler *handler)
{
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

PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::onUpload(PsychicHttpBasicUploadHandler handler) {
  this->_uploadCallback = handler;
  this->isUpload = true;
  return this;
}

PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::onMultipart(PsychicHttpMultipartUploadHandler handler) {
  this->_multipartCallback = handler;
  this->isUpload = true;
  return this;
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

esp_err_t PsychicHttpServerEndpoint::_uploadHandler(PsychicHttpServerRequest &request)
{
  esp_err_t err = ESP_OK;

  /* File cannot be larger than a limit */
  if (request._req->content_len > this->_server->maxUploadSize)
  {
    ESP_LOGE(PH_TAG, "File too large : %d bytes", request._req->content_len);

    /* Respond with 400 Bad Request */
    char error[50];
    sprintf(error, "File size must be less than %u bytes!", this->_server->maxUploadSize);
    httpd_resp_send_err(request._req, HTTPD_400_BAD_REQUEST, error);

    /* Return failure to close underlying connection else the incoming file content will keep the socket busy */
    return ESP_FAIL;
  }

  //TODO: support for the 100 header
  // if (request.header("Expect").equals("100-continue"))
  // {
  //   TRACE();
  //   char response[] = "100 Continue";
  //   httpd_socket_send(self->server, httpd_req_to_sockfd(req), response, strlen(response), 0);
  // }

  //2 types of upload requests
  if (request.isMultipart())
    err = this->_multipartUploadHandler(request);
  else
    err = this->_basicUploadHandler(request);

  //we can also call onRequest for some final processing and response
  // if (err == ESP_OK)
  // {
  //   if (this->_requestCallback != NULL)
  //     err = this->_requestCallback(&request);
  //   else
  //     err = request.reply(200);
  // }
  // else
    request.reply(500, "text/html", "Error processing upload.");

  return err;
}

esp_err_t PsychicHttpServerEndpoint::_basicUploadHandler(PsychicHttpServerRequest &request)
{
  esp_err_t err = ESP_OK;

  String filename = request.getFilename();

  /* Retrieve the pointer to scratch buffer for temporary storage */
  char *buf = (char *)malloc(FILE_CHUNK_SIZE);
  int received;
  unsigned long index = 0; 

  /* Content length of the request gives the size of the file being uploaded */
  int remaining = request._req->content_len;

  while (remaining > 0)
  {
    ESP_LOGI(PH_TAG, "Remaining size : %d", remaining);

    /* Receive the file part by part into a buffer */
    if ((received = httpd_req_recv(request._req, buf, min(remaining, FILE_CHUNK_SIZE))) <= 0)
    {
      /* Retry if timeout occurred */
      if (received == HTTPD_SOCK_ERR_TIMEOUT)
        continue;
      //bail if we got an error
      else if (received == HTTPD_SOCK_ERR_FAIL)
      {
        ESP_LOGE(PH_TAG, "Socket error");
        err = ESP_FAIL;
        break;
      }
    }

    //call our upload callback here.
    if (this->_uploadCallback != NULL)
    {
      err = this->_uploadCallback(&request, filename, index, (uint8_t *)buf, received);
      if (err != ESP_OK)
        break;
    }
    else
    {
      ESP_LOGE(PH_TAG, "No upload callback specified!");
      err = ESP_FAIL;
      break;
    }

    /* Keep track of remaining size of the file left to be uploaded */
    remaining -= received;
    index += received;
  }

  //dont forget to free our buffer
  free(buf);

  return err;
}

esp_err_t PsychicHttpServerEndpoint::_multipartUploadHandler(PsychicHttpServerRequest &request)
{
  ESP_LOGE(PH_TAG,"Multipart uploads not (yet) supported.");

  /* Respond with 400 Bad Request */
  httpd_resp_send_err(request._req, HTTPD_400_BAD_REQUEST, "Multipart uploads not (yet) supported.");

  /* Return failure to close underlying connection else the incoming file content will keep the socket busy */
  //return ESP_ERR_HTTPD_INVALID_REQ;   
  return ESP_FAIL;
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