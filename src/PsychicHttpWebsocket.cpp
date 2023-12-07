#include "PsychicHttpWebsocket.h"

/*************************************/
/*  PsychicWebSocketRequest      */
/*************************************/

PsychicWebSocketRequest::PsychicWebSocketRequest(PsychicHttpServerRequest *req, PsychicWebSocketClient *client) :
  PsychicHttpServerRequest(req->server(), req->request()),
  wsclient(client)
{
}

PsychicWebSocketRequest::~PsychicWebSocketRequest()
{
}

esp_err_t PsychicWebSocketRequest::reply(httpd_ws_frame_t * ws_pkt)
{
  return httpd_ws_send_frame(this->_req, ws_pkt);
} 

esp_err_t PsychicWebSocketRequest::reply(httpd_ws_type_t op, const void *data, size_t len)
{
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  ws_pkt.payload = (uint8_t*)data;
  ws_pkt.len = len;
  ws_pkt.type = op;

  return this->reply(&ws_pkt);
}

esp_err_t PsychicWebSocketRequest::reply(const char *buf)
{
  return this->reply(HTTPD_WS_TYPE_TEXT, buf, strlen(buf));
}

/*************************************/
/*  PsychicWebSocketClient   */
/*************************************/

PsychicWebSocketClient::PsychicWebSocketClient(PsychicClient *client)
  : PsychicClient(client->server(), client->socket())
{
}

esp_err_t PsychicWebSocketClient::sendMessage(httpd_ws_frame_t * ws_pkt)
{
  return httpd_ws_send_frame_async(this->server(), this->socket(), ws_pkt);
} 

esp_err_t PsychicWebSocketClient::sendMessage(httpd_ws_type_t op, const void *data, size_t len)
{
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  ws_pkt.payload = (uint8_t*)data;
  ws_pkt.len = len;
  ws_pkt.type = op;

  return this->sendMessage(&ws_pkt);
}

esp_err_t PsychicWebSocketClient::sendMessage(const char *buf)
{
  return this->sendMessage(HTTPD_WS_TYPE_TEXT, buf, strlen(buf));
}

PsychicWebsocketHandler::PsychicWebsocketHandler() :
  PsychicHandler(),
  _onOpen(NULL),
  _onFrame(NULL),
  _onClose(NULL)
  {
  }

PsychicWebsocketHandler::~PsychicWebsocketHandler() {
  for (auto *client : _clients)
    delete(client);
  _clients.clear();
}

void PsychicWebsocketHandler::addClient(PsychicWebSocketClient *client) {
  _clients.push_back(client);
}

void PsychicWebsocketHandler::removeClient(PsychicWebSocketClient *client) {
  _clients.remove(client);
  delete client;
}

PsychicWebSocketClient * PsychicWebsocketHandler::getClient(PsychicClient *socket) {
  for (PsychicWebSocketClient * client : _clients)
    if (client->socket() == socket->socket())
      return client;

  return NULL;
}

bool PsychicWebsocketHandler::hasClient(PsychicClient *socket) {
  return getClient(socket) != NULL;
}

bool PsychicWebsocketHandler::isWebsocket() {
  return true;
}

bool PsychicWebsocketHandler::canHandle(PsychicHttpServerRequest *request) {
  return true;
}

esp_err_t PsychicWebsocketHandler::handleRequest(PsychicHttpServerRequest *request)
{
  //lookup our client
  PsychicWebSocketClient *client = getClient(request->client());
  if (client == NULL)
  {
    client = new PsychicWebSocketClient(request->client());
    addClient(client);
  }

  PsychicWebSocketRequest wsRequest(request, client);

  // beginning of the ws URI handler and our onConnect hook
  if (wsRequest.method() == HTTP_GET)
  {
    if (this->_onOpen != NULL)  
      this->_onOpen(wsRequest.wsclient);

    return ESP_OK;
  }

  //init our memory for storing the packet
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;
  uint8_t *buf = NULL;

  /* Set max_len = 0 to get the frame len */
  esp_err_t ret = httpd_ws_recv_frame(wsRequest.request(), &ws_pkt, 0);
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
    ret = httpd_ws_recv_frame(wsRequest.request(), &ws_pkt, ws_pkt.len);
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
    if (this->_onFrame != NULL)
      ret = this->_onFrame(&wsRequest, &ws_pkt);
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

PsychicWebsocketHandler * PsychicWebsocketHandler::onOpen(PsychicWebsocketClientCallback fn) {
  _onOpen = fn;
  return this;
}

PsychicWebsocketHandler * PsychicWebsocketHandler::onFrame(PsychicWebSocketFrameCallback fn) {
  _onFrame = fn;
  return this;
}

PsychicWebsocketHandler * PsychicWebsocketHandler::onClose(PsychicWebsocketClientCallback fn) {
  _onClose = fn;
  return this;
}

void PsychicWebsocketHandler::closeCallback(PsychicClient *client) {
  PsychicWebSocketClient *wsclient = getClient(client);
  if (wsclient != NULL)
  {
    if (_onClose != NULL)
      _onClose(wsclient);

    removeClient(wsclient);
  }
}

void PsychicWebsocketHandler::sendAll(httpd_ws_frame_t * ws_pkt)
{
  for (PsychicWebSocketClient * client : _clients)
  {
    ESP_LOGI(PH_TAG, "Active client (fd=%d) -> sending async message", client->socket());

    if (client->sendMessage(ws_pkt) != ESP_OK)
      break;
  }
}

void PsychicWebsocketHandler::sendAll(httpd_ws_type_t op, const void *data, size_t len)
{
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  ws_pkt.payload = (uint8_t*)data;
  ws_pkt.len = len;
  ws_pkt.type = op;

  this->sendAll(&ws_pkt);
}

void PsychicWebsocketHandler::sendAll(const char *buf)
{
  this->sendAll(HTTPD_WS_TYPE_TEXT, buf, strlen(buf));
}