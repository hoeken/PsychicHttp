#include "PsychicHttpServer.h"
#include "PsychicHttpServerEndpoint.h"
#include "PsychicHandler.h"
#include "PsychicWebHandler.h"
#include "PsychicStaticFileHandler.h"
#include "PsychicHttpWebsocket.h"

PsychicHttpServer::PsychicHttpServer() :
  _handlers(LinkedList<PsychicHandler*>([](PsychicHandler* h){ delete h; }))
{
  //defaults
  this->maxRequestBodySize = 16 * 1024; //maximum non-upload request body size (16kb)
  this->maxUploadSize = 256 * 1024; //maximum file upload size (256kb)

  this->openHandler = NULL;
  this->closeHandler = NULL;
  this->staticHandler = NULL;

  this->defaultEndpoint = new PsychicHttpServerEndpoint(this, HTTP_GET, "");
  this->onNotFound(PsychicHttpServer::defaultNotFoundHandler);
  
  //for a regular server
  this->config = HTTPD_DEFAULT_CONFIG();
  this->config.open_fn = PsychicHttpServer::openCallback;
  this->config.close_fn = PsychicHttpServer::closeCallback;
  this->config.uri_match_fn = httpd_uri_match_wildcard;
  this->config.global_user_ctx = this;
  this->config.global_user_ctx_free_fn = this->destroy;

  //for a SSL server
  this->ssl_config = HTTPD_SSL_CONFIG_DEFAULT();
  this->ssl_config.httpd.open_fn = PsychicHttpServer::openCallback;
  this->ssl_config.httpd.close_fn = PsychicHttpServer::closeCallback;
  this->ssl_config.httpd.uri_match_fn = httpd_uri_match_wildcard;
  this->ssl_config.httpd.global_user_ctx = this;
  this->ssl_config.httpd.global_user_ctx_free_fn = this->destroy;
  
  // each SSL connection takes about 45kb of heap
  // a barebones sketch with PsychicHttp has ~150kb of heap available
  // if we set it higher than 2 and use all the connections, we get lots of memory errors.
  // not to mention there is no heap left over for the program itself.
  this->ssl_config.httpd.max_open_sockets = 2;

  #ifdef ENABLE_ASYNC
    this->config.lru_purge_enable = true;

    // It is advisable that httpd_config_t->max_open_sockets > MAX_ASYNC_REQUESTS
    // Why? This leaves at least one socket still available to handle
    // quick synchronous requests. Otherwise, all the sockets will
    // get taken by the long async handlers, and your server will no
    // longer be responsive.
    //this->config.max_open_sockets = ASYNC_WORKER_COUNT + 1;
  #endif
}

PsychicHttpServer::~PsychicHttpServer()
{
  _handlers.free();

  for (PsychicHttpServerEndpoint * endpoint : this->endpoints)
    delete(endpoint);

  if (staticHandler != NULL)
    delete staticHandler;

  delete defaultEndpoint;
}

void PsychicHttpServer::destroy(void *ctx)
{
  PsychicHttpServer *temp = (PsychicHttpServer *)ctx;
  delete temp;
}

esp_err_t PsychicHttpServer::listen(uint16_t port)
{
  this->use_ssl = false;
  this->config.server_port = port;

  return this->_start();
}

esp_err_t PsychicHttpServer::listen(uint16_t port, const char *cert, const char *private_key)
{
  this->use_ssl = true;

  this->ssl_config.port_secure = port;
  this->ssl_config.cacert_pem = (uint8_t *)cert;
  this->ssl_config.cacert_len = strlen(cert)+1;
  this->ssl_config.prvtkey_pem = (uint8_t *)private_key;
  this->ssl_config.prvtkey_len = strlen(private_key)+1;

  return this->_start();
}

esp_err_t PsychicHttpServer::_start()
{
  #ifdef ENABLE_ASYNC
    // start workers
    start_async_req_workers();
  #endif

  //what mode to start in?
  esp_err_t err;
  if (this->use_ssl)
    err = httpd_ssl_start(&this->server, &this->ssl_config);
  else
    err = httpd_start(&this->server, &this->config);

  // Register handler
  esp_err_t ret = httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, PsychicHttpServer::notFoundHandler);
  if (ret != ESP_OK)
    ESP_LOGE(PH_TAG, "Add 404 handler failed (%s)", esp_err_to_name(ret)); 

  return ret;
}

void PsychicHttpServer::stop()
{
  //Stop our http server
  if (this->use_ssl)
    httpd_ssl_stop(this->server);
  else
    httpd_stop(this->server);
}

PsychicHandler& PsychicHttpServer::addHandler(PsychicHandler* handler){
  _handlers.add(handler);
  return *handler;
}

bool PsychicHttpServer::removeHandler(PsychicHandler *handler){
  return _handlers.remove(handler);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri) {
  return on(uri, HTTP_GET);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, PsychicHttpRequestHandler fn)
{
  return on(uri, HTTP_GET, fn);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, http_method method, PsychicHttpRequestHandler fn)
{
  PsychicWebHandler *handler = new PsychicWebHandler();
  handler->onRequest(fn);

  PsychicHttpServerEndpoint *endpoint = on(uri, method);
  endpoint->setHandler(handler);

  return endpoint;
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, http_method method)
{
  PsychicHttpServerEndpoint *endpoint = new PsychicHttpServerEndpoint(this, method, uri);

  // URI handler structure
  httpd_uri_t my_uri {
    .uri      = uri,
    .method   = method,
    .handler  = PsychicHttpServerEndpoint::requestCallback,
    .user_ctx = endpoint
  };

  // Register endpoint with ESP-IDF server
  esp_err_t ret = httpd_register_uri_handler(this->server, &my_uri);
  if (ret != ESP_OK)
    ESP_LOGE(PH_TAG, "Add endpoint failed (%s)", esp_err_to_name(ret));

  //set a basic handler
  PsychicWebHandler *handler = new PsychicWebHandler();
  endpoint->setHandler(handler);

  //save it for later
  this->endpoints.push_back(endpoint);

  return endpoint;
}

PsychicHttpServerEndpoint *PsychicHttpServer::websocket(const char* uri)
{
  PsychicHttpServerEndpoint *handler = new PsychicHttpServerEndpoint(this, HTTP_GET, uri);
  handler->isWebsocket = true;
  this->endpoints.push_back(handler);

  // URI handler structure
  httpd_uri_t my_uri {
    .uri      = uri,
    .method   = HTTP_GET,
    .handler  = PsychicHttpServerEndpoint::requestCallback,
    .user_ctx = handler,
    .is_websocket = true
  };

  // Register handler
  esp_err_t ret = httpd_register_uri_handler(this->server, &my_uri);
  if (ret != ESP_OK)
    ESP_LOGE(PH_TAG, "Add websocket handler failed (%s)", esp_err_to_name(ret));

  return handler;
}

void PsychicHttpServer::onNotFound(PsychicHttpRequestHandler fn)
{
  //TODO: sort this out
  //this->defaultEndpoint->getHandler()->onRequest(fn);
}

esp_err_t PsychicHttpServer::notFoundHandler(httpd_req_t *req, httpd_err_code_t err)
{
  PsychicHttpServer *server = (PsychicHttpServer*)httpd_get_global_user_ctx(req->handle);
  PsychicHttpServerRequest request(server, req);

  esp_err_t result = ESP_OK;

  // //do we have a static handler?
  // if (server->staticHandler != NULL)
  // {
  //   if (server->staticHandler->canHandle(&request))
  //     result = server->staticHandler->handleRequest(&request);
  //   else
  //     result = server->defaultEndpoint->_requestCallback(&request);
  // }
  // //nope, just give them the default
  // else
  //   result = server->defaultEndpoint->_requestCallback(&request);

  return result;
}

esp_err_t PsychicHttpServer::defaultNotFoundHandler(PsychicHttpServerRequest *request)
{
  request->reply(404, "text/html", "That URI does not exist.");

  return ESP_OK;
}

void PsychicHttpServer::onOpen(PsychicHttpConnectionHandler handler) {
  this->openHandler = handler;
}

esp_err_t PsychicHttpServer::openCallback(httpd_handle_t hd, int sockfd)
{
  ESP_LOGI(PH_TAG, "New client connected %d", sockfd);

  if (httpd_ws_get_fd_info(hd, sockfd) == HTTPD_WS_CLIENT_WEBSOCKET)
  {
    ESP_LOGI(PH_TAG, "Websocket connected %d", sockfd);
  }

  //do we have a callback attached?
  PsychicHttpServer *server = (PsychicHttpServer*)httpd_get_global_user_ctx(hd);
  if (server->openHandler != NULL)
    server->openHandler(server, sockfd);

  return ESP_OK;
}

void PsychicHttpServer::onClose(PsychicHttpConnectionHandler handler) {
  this->closeHandler = handler;
}

void PsychicHttpServer::closeCallback(httpd_handle_t hd, int sockfd)
{
  ESP_LOGI(PH_TAG, "Client disconnected %d", sockfd);

  PsychicHttpServer *server = (PsychicHttpServer*)httpd_get_global_user_ctx(hd);

  //remove it from our connections list and do callback if needed
  for (PsychicHttpServerEndpoint * endpoint : server->endpoints)
  {
    if (endpoint->isWebsocket)
    {
      // Check if the value is in the list
      auto it = std::find(endpoint->websocketConnections.begin(), endpoint->websocketConnections.end(), sockfd);
      if (it != endpoint->websocketConnections.end())
      {
        endpoint->websocketConnections.erase(it);

        //callback?
        if (endpoint->_wsCloseCallback != NULL)
          endpoint->_wsCloseCallback(server, sockfd);
      }    
    }
  }

  //do we have a callback attached?
  if (server->closeHandler != NULL)
    server->closeHandler(server, sockfd);

  //we need to close our own socket here!
  close(sockfd);
}

PsychicStaticFileHandler& PsychicHttpServer::serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_control)
{
  PsychicStaticFileHandler* handler = new PsychicStaticFileHandler(uri, fs, path, cache_control);
  this->staticHandler = handler;

  return *handler;
}

void PsychicHttpServer::sendAll(httpd_ws_frame_t * ws_pkt)
{
  size_t max_clients = this->config.max_open_sockets;
  size_t clients = max_clients;
  int    client_fds[max_clients];

  if (httpd_get_client_list(this->server, &clients, client_fds) == ESP_OK)
  {
    for (size_t i=0; i < clients; ++i)
    {
      int sock = client_fds[i];
      if (httpd_ws_get_fd_info(this->server, sock) == HTTPD_WS_CLIENT_WEBSOCKET)
      {
        ESP_LOGI(PH_TAG, "Active client (fd=%d) -> sending async message", sock);

        PsychicHttpWebSocketConnection connection(this->server, sock);
        if (connection.queueMessage(ws_pkt) != ESP_OK)
          break;
      }
    }
  } else {
    ESP_LOGE(PH_TAG, "httpd_get_client_list failed!");
    return;
  }
}

void PsychicHttpServer::sendAll(httpd_ws_type_t op, const void *data, size_t len)
{
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  ws_pkt.payload = (uint8_t*)data;
  ws_pkt.len = len;
  ws_pkt.type = op;

  this->sendAll(&ws_pkt);
}

void PsychicHttpServer::sendAll(const char *buf)
{
  this->sendAll(HTTPD_WS_TYPE_TEXT, buf, strlen(buf));
}

bool ON_STA_FILTER(PsychicHttpServerRequest *request) {
  //return WiFi.localIP() == request->client()->localIP();
  return true;
}

bool ON_AP_FILTER(PsychicHttpServerRequest *request) {
  //return WiFi.localIP() != request->client()->localIP();
  return true;
}