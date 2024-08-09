#include "PsychicHttpServer.h"
#include "PsychicEndpoint.h"
#include "PsychicHandler.h"
#include "PsychicJson.h"
#include "PsychicStaticFileHandler.h"
#include "PsychicWebHandler.h"
#include "PsychicWebSocket.h"
#include "WiFi.h"

PsychicHttpServer::PsychicHttpServer(uint16_t port) : _onOpen(NULL),
                                                      _onClose(NULL)
{
  maxRequestBodySize = MAX_REQUEST_BODY_SIZE;
  maxUploadSize = MAX_UPLOAD_SIZE;

  defaultEndpoint = new PsychicEndpoint(this, HTTP_GET, "");
  onNotFound(PsychicHttpServer::defaultNotFoundHandler);

  // for a regular server
  config = HTTPD_DEFAULT_CONFIG();
  config.open_fn = PsychicHttpServer::openCallback;
  config.close_fn = PsychicHttpServer::closeCallback;
  config.global_user_ctx = this;
  config.global_user_ctx_free_fn = PsychicHttpServer::destroy;

  // use their matching function internally
  uri_match_fn = httpd_uri_match_wildcard;
  config.uri_match_fn = httpd_uri_match_wildcard;
  // new internal endpoint matching - always match url
  // config.uri_match_fn = [](const char *uri_template, const char *uri_to_match, size_t match_upto) { return true;};

#ifdef ENABLE_ASYNC
  // It is advisable that httpd_config_t->max_open_sockets > MAX_ASYNC_REQUESTS
  // Why? This leaves at least one socket still available to handle
  // quick synchronous requests. Otherwise, all the sockets will
  // get taken by the long async handlers, and your server will no
  // longer be responsive.
  config.max_open_sockets = ASYNC_WORKER_COUNT + 1;
  config.lru_purge_enable = true;
#endif

  setPort(port);
}

PsychicHttpServer::~PsychicHttpServer()
{
  for (auto* client : _clients)
    delete (client);
  _clients.clear();

  for (auto* endpoint : _endpoints)
    delete (endpoint);
  _endpoints.clear();

  for (auto* handler : _handlers)
    delete (handler);
  _handlers.clear();

  delete defaultEndpoint;
}

void PsychicHttpServer::destroy(void* ctx)
{
  // do not release any resource for PsychicHttpServer in order to be able to restart it after stopping
}

void PsychicHttpServer::setPort(uint16_t port)
{
  this->config.server_port = port;
}

esp_err_t PsychicHttpServer::start()
{
  if (_running)
    return ESP_OK;

  esp_err_t ret;

#ifdef ENABLE_ASYNC
  // start workers
  start_async_req_workers();
#endif

  // one URI handler for each http_method
  config.max_uri_handlers = supported_methods.size() + _esp_idf_endpoints.size();

  // fire it up.
  ret = _startServer();
  if (ret != ESP_OK)
  {
    ESP_LOGE(PH_TAG, "Server start failed (%s)", esp_err_to_name(ret));
    return ret;
  }

  // some handlers (aka websockets) need actual endpoints in esp-idf http_server
  for (auto endpoint : _esp_idf_endpoints)
  {
    ESP_LOGD(PH_TAG, "Adding endpoint %s | %s", endpoint.uri, http_method_str((http_method)endpoint.method));

    // Register endpoint with ESP-IDF server
    esp_err_t ret = httpd_register_uri_handler(this->server, &endpoint);
    if (ret != ESP_OK)
      ESP_LOGE(PH_TAG, "Add endpoint failed (%s)", esp_err_to_name(ret));
  }

  // Register a handler for each http_method method - it will match all requests with that URI/method
  for (auto method : supported_methods)
  {
    ESP_LOGD(PH_TAG, "Adding %s meta endpoint", http_method_str((http_method)method));

    httpd_uri_t my_uri{
      .uri = "*",
      .method = method,
      .handler = PsychicHttpServer::requestHandler};

    // Register endpoint with ESP-IDF server
    esp_err_t ret = httpd_register_uri_handler(this->server, &my_uri);
    if (ret != ESP_OK)
      ESP_LOGE(PH_TAG, "Add endpoint failed (%s)", esp_err_to_name(ret));
  }

  // Register handler
  ret = httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, PsychicHttpServer::notFoundHandler);
  if (ret != ESP_OK)
    ESP_LOGE(PH_TAG, "Add 404 handler failed (%s)", esp_err_to_name(ret));

  _running = true;
  return ret;
}

esp_err_t PsychicHttpServer::_startServer()
{
  return httpd_start(&this->server, &this->config);
}

esp_err_t PsychicHttpServer::stop()
{
  if (!_running)
    return ESP_OK;

  esp_err_t ret = _stopServer();
  if (ret != ESP_OK)
  {
    ESP_LOGE(PH_TAG, "Server stop failed (%s)", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(PH_TAG, "Server stopped");
  _running = false;
  return ret;
}

esp_err_t PsychicHttpServer::_stopServer()
{
  return httpd_stop(this->server);
}

PsychicHandler& PsychicHttpServer::addHandler(PsychicHandler* handler)
{
  _handlers.push_back(handler);
  return *handler;
}

void PsychicHttpServer::removeHandler(PsychicHandler* handler)
{
  _handlers.remove(handler);
}

PsychicEndpoint* PsychicHttpServer::on(const char* uri)
{
  return on(uri, HTTP_GET);
}

PsychicEndpoint* PsychicHttpServer::on(const char* uri, int method)
{
  PsychicWebHandler* handler = new PsychicWebHandler();

  return on(uri, method, handler);
}

PsychicEndpoint* PsychicHttpServer::on(const char* uri, PsychicHandler* handler)
{
  return on(uri, HTTP_GET, handler);
}

PsychicEndpoint* PsychicHttpServer::on(const char* uri, int method, PsychicHandler* handler)
{
  // make our endpoint
  PsychicEndpoint* endpoint = new PsychicEndpoint(this, method, uri);

  // set our handler
  endpoint->setHandler(handler);

  // websockets need a real endpoint in esp-idf
  if (handler->isWebSocket())
  {
    Serial.println("Websocket added.");

    // URI handler structure
    httpd_uri_t my_uri{
      .uri = uri,
      .method = HTTP_GET,
      .handler = PsychicEndpoint::requestCallback,
      .user_ctx = endpoint,
      .is_websocket = handler->isWebSocket(),
      .supported_subprotocol = handler->getSubprotocol()};

    // save it to our 'real' handlers for later.
    _esp_idf_endpoints.push_back(my_uri);
  }

  // save it for later
  _endpoints.push_back(endpoint);

  return endpoint;
}

PsychicEndpoint* PsychicHttpServer::on(const char* uri, PsychicHttpRequestCallback fn)
{
  return on(uri, HTTP_GET, fn);
}

PsychicEndpoint* PsychicHttpServer::on(const char* uri, int method, PsychicHttpRequestCallback fn)
{
  // these basic requests need a basic web handler
  PsychicWebHandler* handler = new PsychicWebHandler();
  handler->onRequest(fn);

  return on(uri, method, handler);
}

PsychicEndpoint* PsychicHttpServer::on(const char* uri, PsychicJsonRequestCallback fn)
{
  return on(uri, HTTP_GET, fn);
}

PsychicEndpoint* PsychicHttpServer::on(const char* uri, int method, PsychicJsonRequestCallback fn)
{
  // these basic requests need a basic web handler
  PsychicJsonHandler* handler = new PsychicJsonHandler();
  handler->onRequest(fn);

  return on(uri, method, handler);
}

void PsychicHttpServer::onNotFound(PsychicHttpRequestCallback fn)
{
  PsychicWebHandler* handler = new PsychicWebHandler();
  handler->onRequest(fn == nullptr ? PsychicHttpServer::defaultNotFoundHandler : fn);

  this->defaultEndpoint->setHandler(handler);
}

esp_err_t PsychicHttpServer::requestHandler(httpd_req_t* req)
{
  PsychicHttpServer* server = (PsychicHttpServer*)httpd_get_global_user_ctx(req->handle);
  PsychicRequest request(server, req);

  // ESP_LOGD(PH_TAG, "Request: %s | %s", request.uri(), request.methodStr());

  // loop through our endpoints and see if anyone wants it.
  for (auto* endpoint : server->_endpoints)
  {
    // ESP_LOGD(PH_TAG, "Checking endpoint: %s | %s", endpoint->uri(), http_method_str((http_method)req->method));

    // check urls first
    if (endpoint->matches(req->uri))
    {
      // ESP_LOGD(PH_TAG, "URI Matched");

      // check the http_method next
      if (endpoint->_method == request.method() || endpoint->_method == HTTP_ANY)
      {
        // ESP_LOGD(PH_TAG, "http_method OK");

        // check other filter functions
        PsychicHandler* handler = endpoint->handler();
        if (handler->filter(&request) && handler->canHandle(&request))
        {
          // ESP_LOGD(PH_TAG, "Filter OK");

          // is the handler ok?
          if (handler->canHandle(&request))
          {
            // ESP_LOGD(PH_TAG, "Handler OK");

            // check our credentials
            if (handler->needsAuthentication(&request))
              return handler->authenticate(&request);
            else
              return handler->handleRequest(&request);
          }
        }
      }
    }
  }

  // loop through our global handlers and see if anyone wants it
  for (auto* handler : server->_handlers)
  {
    // are we capable of handling this?
    if (handler->filter(&request) && handler->canHandle(&request))
    {
      // check our credentials
      if (handler->needsAuthentication(&request))
        return handler->authenticate(&request);
      else
        return handler->handleRequest(&request);
    }
  }

  // nothing found, give it to our defaultEndpoint
  PsychicHandler* handler = server->defaultEndpoint->handler();
  if (handler->filter(&request) && handler->canHandle(&request))
    return handler->handleRequest(&request);

  // not sure how we got this far.
  return ESP_ERR_HTTPD_INVALID_REQ;
}

esp_err_t PsychicHttpServer::notFoundHandler(httpd_req_t* req, httpd_err_code_t err)
{
  PsychicHttpServer* server = (PsychicHttpServer*)httpd_get_global_user_ctx(req->handle);
  PsychicRequest request(server, req);

  // pull up our default handler / endpoint
  PsychicHandler* handler = server->defaultEndpoint->handler();
  if (handler->filter(&request) && handler->canHandle(&request))
    return handler->handleRequest(&request);

  // not sure how we got this far.
  return ESP_ERR_HTTPD_INVALID_REQ;
}

esp_err_t PsychicHttpServer::defaultNotFoundHandler(PsychicRequest* request)
{
  request->reply(404, "text/html", "That URI does not exist.");

  return ESP_OK;
}

void PsychicHttpServer::onOpen(PsychicClientCallback handler)
{
  this->_onOpen = handler;
}

esp_err_t PsychicHttpServer::openCallback(httpd_handle_t hd, int sockfd)
{
  ESP_LOGD(PH_TAG, "New client connected %d", sockfd);

  // get our global server reference
  PsychicHttpServer* server = (PsychicHttpServer*)httpd_get_global_user_ctx(hd);

  // lookup our client
  PsychicClient* client = server->getClient(sockfd);
  if (client == NULL)
  {
    client = new PsychicClient(hd, sockfd);
    server->addClient(client);
  }

  // user callback
  if (server->_onOpen != NULL)
    server->_onOpen(client);

  return ESP_OK;
}

void PsychicHttpServer::onClose(PsychicClientCallback handler)
{
  this->_onClose = handler;
}

void PsychicHttpServer::closeCallback(httpd_handle_t hd, int sockfd)
{
  ESP_LOGD(PH_TAG, "Client disconnected %d", sockfd);

  PsychicHttpServer* server = (PsychicHttpServer*)httpd_get_global_user_ctx(hd);

  // lookup our client
  PsychicClient* client = server->getClient(sockfd);
  if (client != NULL)
  {
    // give our handlers a chance to handle a disconnect first
    for (PsychicEndpoint* endpoint : server->_endpoints)
    {
      PsychicHandler* handler = endpoint->handler();
      handler->checkForClosedClient(client);
    }

    // do we have a callback attached?
    if (server->_onClose != NULL)
      server->_onClose(client);

    // remove it from our list
    server->removeClient(client);
  }
  else
    ESP_LOGE(PH_TAG, "No client record %d", sockfd);

  // finally close it out.
  close(sockfd);
}

PsychicStaticFileHandler* PsychicHttpServer::serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_control)
{
  PsychicStaticFileHandler* handler = new PsychicStaticFileHandler(uri, fs, path, cache_control);
  this->addHandler(handler);

  return handler;
}

void PsychicHttpServer::addClient(PsychicClient* client)
{
  _clients.push_back(client);
}

void PsychicHttpServer::removeClient(PsychicClient* client)
{
  _clients.remove(client);
  delete client;
}

PsychicClient* PsychicHttpServer::getClient(int socket)
{
  for (PsychicClient* client : _clients)
    if (client->socket() == socket)
      return client;

  return NULL;
}

PsychicClient* PsychicHttpServer::getClient(httpd_req_t* req)
{
  return getClient(httpd_req_to_sockfd(req));
}

bool PsychicHttpServer::hasClient(int socket)
{
  return getClient(socket) != NULL;
}

const std::list<PsychicClient*>& PsychicHttpServer::getClientList()
{
  return _clients;
}

bool ON_STA_FILTER(PsychicRequest* request)
{
  return WiFi.localIP() == request->client()->localIP();
}

bool ON_AP_FILTER(PsychicRequest* request)
{
  return WiFi.softAPIP() == request->client()->localIP();
}

String urlDecode(const char* encoded)
{
  size_t length = strlen(encoded);
  char* decoded = (char*)malloc(length + 1);
  if (!decoded)
  {
    return "";
  }

  size_t i, j = 0;
  for (i = 0; i < length; ++i)
  {
    if (encoded[i] == '%' && isxdigit(encoded[i + 1]) && isxdigit(encoded[i + 2]))
    {
      // Valid percent-encoded sequence
      int hex;
      sscanf(encoded + i + 1, "%2x", &hex);
      decoded[j++] = (char)hex;
      i += 2; // Skip the two hexadecimal characters
    }
    else if (encoded[i] == '+')
    {
      // Convert '+' to space
      decoded[j++] = ' ';
    }
    else
    {
      // Copy other characters as they are
      decoded[j++] = encoded[i];
    }
  }

  decoded[j] = '\0'; // Null-terminate the decoded string

  String output(decoded);
  free(decoded);

  return output;
}