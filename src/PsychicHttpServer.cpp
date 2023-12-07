#include "PsychicHttpServer.h"
#include "PsychicHttpServerEndpoint.h"
#include "PsychicHandler.h"
#include "PsychicWebHandler.h"
#include "PsychicStaticFileHandler.h"
#include "PsychicHttpWebsocket.h"
#include "WiFi.h"

PsychicHttpServer::PsychicHttpServer()
{
  maxRequestBodySize = MAX_REQUEST_BODY_SIZE;
  maxUploadSize = MAX_UPLOAD_SIZE;

  _onOpen = NULL;
  _onClose = NULL;

  defaultEndpoint = new PsychicHttpServerEndpoint(this, HTTP_GET, "");
  onNotFound(PsychicHttpServer::defaultNotFoundHandler);
  
  //for a regular server
  config = HTTPD_DEFAULT_CONFIG();
  config.open_fn = PsychicHttpServer::openCallback;
  config.close_fn = PsychicHttpServer::closeCallback;
  config.uri_match_fn = httpd_uri_match_wildcard;
  config.global_user_ctx = this;
  config.global_user_ctx_free_fn = destroy;

  //for a SSL server
  ssl_config = HTTPD_SSL_CONFIG_DEFAULT();
  ssl_config.httpd.open_fn = PsychicHttpServer::openCallback;
  ssl_config.httpd.close_fn = PsychicHttpServer::closeCallback;
  ssl_config.httpd.uri_match_fn = httpd_uri_match_wildcard;
  ssl_config.httpd.global_user_ctx = this;
  ssl_config.httpd.global_user_ctx_free_fn = destroy;
  
  // each SSL connection takes about 45kb of heap
  // a barebones sketch with PsychicHttp has ~150kb of heap available
  // if we set it higher than 2 and use all the connections, we get lots of memory errors.
  // not to mention there is no heap left over for the program itself.
  ssl_config.httpd.max_open_sockets = 2;

  #ifdef ENABLE_ASYNC
    config.lru_purge_enable = true;

    // It is advisable that httpd_config_t->max_open_sockets > MAX_ASYNC_REQUESTS
    // Why? This leaves at least one socket still available to handle
    // quick synchronous requests. Otherwise, all the sockets will
    // get taken by the long async handlers, and your server will no
    // longer be responsive.
    //config.max_open_sockets = ASYNC_WORKER_COUNT + 1;
  #endif
}

PsychicHttpServer::~PsychicHttpServer()
{
  for (auto *client : _clients)
    delete(client);
  _clients.clear();

  for (auto *endpoint : _endpoints)
    delete(endpoint);
  _endpoints.clear();

  for (auto *handler : _handlers)
    delete(handler);
  _handlers.clear();

  delete defaultEndpoint;
}

void PsychicHttpServer::destroy(void *ctx)
{
  PsychicHttpServer *temp = (PsychicHttpServer *)ctx;
  delete temp;
}

esp_err_t PsychicHttpServer::listen(uint16_t port)
{
  this->_use_ssl = false;
  this->config.server_port = port;

  return this->_start();
}

esp_err_t PsychicHttpServer::listen(uint16_t port, const char *cert, const char *private_key)
{
  this->_use_ssl = true;

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
  if (this->_use_ssl)
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
  if (this->_use_ssl)
    httpd_ssl_stop(this->server);
  else
    httpd_stop(this->server);
}

PsychicHandler& PsychicHttpServer::addHandler(PsychicHandler* handler){
  _handlers.push_back(handler);
  return *handler;
}

void PsychicHttpServer::removeHandler(PsychicHandler *handler){
  _handlers.remove(handler);
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
  //these basic requests need a basic web handler
  PsychicWebHandler *handler = new PsychicWebHandler();
  handler->onRequest(fn);

  return on(uri, method, handler);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, http_method method)
{
  PsychicWebHandler *handler = new PsychicWebHandler();

  return on(uri, method, handler);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, PsychicHandler *handler)
{
  return on(uri, HTTP_GET, handler);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, http_method method, PsychicHandler *handler)
{
  //make our endpoint
  PsychicHttpServerEndpoint *endpoint = new PsychicHttpServerEndpoint(this, method, uri);

  //set our handler
  endpoint->setHandler(handler);

  // URI handler structure
  httpd_uri_t my_uri {
    .uri      = uri,
    .method   = method,
    .handler  = PsychicHttpServerEndpoint::requestCallback,
    .user_ctx = endpoint,
    .is_websocket = handler->isWebsocket()
  };
  
  // Register endpoint with ESP-IDF server
  esp_err_t ret = httpd_register_uri_handler(this->server, &my_uri);
  if (ret != ESP_OK)
    ESP_LOGE(PH_TAG, "Add endpoint failed (%s)", esp_err_to_name(ret));

  //save it for later
  _endpoints.push_back(endpoint);

  return endpoint;
}

void PsychicHttpServer::onNotFound(PsychicHttpRequestHandler fn)
{
  PsychicWebHandler *handler = new PsychicWebHandler();
  handler->onRequest(fn);

  this->defaultEndpoint->setHandler(handler);
}

esp_err_t PsychicHttpServer::notFoundHandler(httpd_req_t *req, httpd_err_code_t err)
{
  PsychicHttpServer *server = (PsychicHttpServer*)httpd_get_global_user_ctx(req->handle);
  PsychicHttpServerRequest request(server, req);

  //loop through our global handlers and see if anyone wants it
  for(auto *handler: server->_handlers)
  {
    //are we capable of handling this?
    if (handler->filter(&request) && handler->canHandle(&request))
    {
      //check our credentials
       if (handler->needsAuthentication(&request))
        return handler->authenticate(&request);
      else
        return handler->handleRequest(&request);
    }
  }

  //nothing found, give it to our defaultEndpoint
  PsychicHandler *handler = server->defaultEndpoint->handler();
  if (handler->filter(&request) && handler->canHandle(&request))
    return handler->handleRequest(&request);

  //not sure how we got this far.
  return ESP_ERR_HTTPD_INVALID_REQ;
}

esp_err_t PsychicHttpServer::defaultNotFoundHandler(PsychicHttpServerRequest *request)
{
  request->reply(404, "text/html", "That URI does not exist.");

  return ESP_OK;
}

void PsychicHttpServer::onOpen(PsychicClientCallback handler) {
  this->_onOpen = handler;
}

esp_err_t PsychicHttpServer::openCallback(httpd_handle_t hd, int sockfd)
{
  ESP_LOGI(PH_TAG, "New client connected %d", sockfd);

  //do we have a callback attached?
  PsychicHttpServer *server = (PsychicHttpServer*)httpd_get_global_user_ctx(hd);

  //lookup our client
  PsychicClient *client = server->getClient(sockfd);
  if (client == NULL)
  {
    client = new PsychicClient(hd, sockfd);
    server->addClient(client);
  }

  //user callback
  if (server->_onOpen != NULL)
    server->_onOpen(client);

  return ESP_OK;
}

void PsychicHttpServer::onClose(PsychicClientCallback handler) {
  this->_onClose = handler;
}

void PsychicHttpServer::closeCallback(httpd_handle_t hd, int sockfd)
{
  ESP_LOGI(PH_TAG, "Client disconnected %d", sockfd);

  PsychicHttpServer *server = (PsychicHttpServer*)httpd_get_global_user_ctx(hd);

  //lookup our client
  PsychicClient *client = server->getClient(sockfd);
  if (client != NULL)
  {
    //remove it from our connections list and do callback if needed
    for (PsychicHttpServerEndpoint * endpoint : server->_endpoints)
    {
      PsychicHandler *handler = endpoint->handler();
      handler->closeCallback(client);
    }

    //do we have a callback attached?
    if (server->_onClose != NULL)
      server->_onClose(client);

    //remove it from our list
    server->removeClient(client);
  }
  else
    ESP_LOGE(PH_TAG, "No client record %d", sockfd);

  //finally close it out.
  close(sockfd);
}

PsychicStaticFileHandler* PsychicHttpServer::serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_control)
{
  PsychicStaticFileHandler* handler = new PsychicStaticFileHandler(uri, fs, path, cache_control);
  this->addHandler(handler);

  return handler;
}

void PsychicHttpServer::addClient(PsychicClient *client) {
  _clients.push_back(client);
}

void PsychicHttpServer::removeClient(PsychicClient *client) {
  _clients.remove(client);
  delete client;
}

PsychicClient * PsychicHttpServer::getClient(int socket) {
  for (PsychicClient * client : _clients)
    if (client->socket() == socket)
      return client;

  return NULL;
}

PsychicClient * PsychicHttpServer::getClient(httpd_req_t *req) {
  return getClient(httpd_req_to_sockfd(req));
}

bool PsychicHttpServer::hasClient(int socket) {
  return getClient(socket) != NULL;
}

bool ON_STA_FILTER(PsychicHttpServerRequest *request) {
  return WiFi.localIP() == request->client()->localIP();
}

bool ON_AP_FILTER(PsychicHttpServerRequest *request) {
  return WiFi.softAPIP() == request->client()->localIP();
}

String urlDecode(const char* encoded)
{
  size_t length = strlen(encoded);
  char* decoded = (char*)malloc(length + 1);
  if (!decoded) {
    return "";
  }

  size_t i, j = 0;
  for (i = 0; i < length; ++i) {
      if (encoded[i] == '%' && isxdigit(encoded[i + 1]) && isxdigit(encoded[i + 2])) {
          // Valid percent-encoded sequence
          int hex;
          sscanf(encoded + i + 1, "%2x", &hex);
          decoded[j++] = (char)hex;
          i += 2;  // Skip the two hexadecimal characters
      } else if (encoded[i] == '+') {
          // Convert '+' to space
          decoded[j++] = ' ';
      } else {
          // Copy other characters as they are
          decoded[j++] = encoded[i];
      }
  }

  decoded[j] = '\0';  // Null-terminate the decoded string

  String output(decoded);
  free(decoded);

  return output;
}