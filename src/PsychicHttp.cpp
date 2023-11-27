#include "PsychicHttp.h"

/*************************************/
/*  PsychicHttpServer                */
/*************************************/

#if !CONFIG_HTTPD_WS_SUPPORT
  #error This library cannot be used unless HTTPD_WS_SUPPORT is enabled in esp-http-server component configuration
#endif

// SemaphoreHandle_t xFileSemaphore = NULL;

PsychicHttpServer::PsychicHttpServer()
{
  this->openHandler = NULL;
  this->closeHandler = NULL;
  this->staticHandler = NULL;

  this->defaultEndpoint = PsychicHttpServerEndpoint(this, HTTP_GET);
  this->onNotFound(PsychicHttpServer::defaultNotFoundHandler);
  
  //for a regular server
  this->config = HTTPD_DEFAULT_CONFIG();
  this->config.max_uri_handlers = 8;
  this->config.stack_size = 10000;
  this->config.open_fn = PsychicHttpServer::openCallback;
  this->config.close_fn = PsychicHttpServer::closeCallback;
  this->config.uri_match_fn = httpd_uri_match_wildcard;

  #ifdef ENABLE_ASYNC
    this->config.lru_purge_enable = true;

    // It is advisable that httpd_config_t->max_open_sockets > MAX_ASYNC_REQUESTS
    // Why? This leaves at least one socket still available to handle
    // quick synchronous requests. Otherwise, all the sockets will
    // get taken by the long async handlers, and your server will no
    // longer be responsive.
    //this->config.max_open_sockets = ASYNC_WORKER_COUNT + 1;
  #endif

  this->config.global_user_ctx = this;
  this->config.global_user_ctx_free_fn = this->destroy;
  
  //for a SSL server
  this->ssl_config = HTTPD_SSL_CONFIG_DEFAULT();
  this->ssl_config.httpd = this->config;

  // xFileSemaphore = xSemaphoreCreateMutex();
}

PsychicHttpServer::~PsychicHttpServer()
{
  for (PsychicHttpServerEndpoint * endpoint : this->endpoints)
    delete(endpoint);

  if (staticHandler != NULL)
    delete staticHandler;
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

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri) {
  return on(uri, HTTP_GET);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, PsychicHttpRequestHandler onRequest)
{
  return on(uri, HTTP_GET)->onRequest(onRequest);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, http_method method, PsychicHttpRequestHandler onRequest)
{
  return on(uri, method)->onRequest(onRequest);
}

PsychicHttpServerEndpoint *PsychicHttpServer::on(const char* uri, http_method method)
{
  PsychicHttpServerEndpoint *handler = new PsychicHttpServerEndpoint(this, method);
  this->endpoints.push_back(handler);
  
  // URI handler structure
  httpd_uri_t my_uri {
    .uri      = uri,
    .method   = method,
    .handler  = PsychicHttpServerEndpoint::requestHandler,
    .user_ctx = handler
  };

  // Register handler
  esp_err_t ret = httpd_register_uri_handler(this->server, &my_uri);
  if (ret != ESP_OK)
    ESP_LOGE(PH_TAG, "Add request handler failed (%s)", esp_err_to_name(ret));

  return handler;
}

PsychicHttpServerEndpoint *PsychicHttpServer::websocket(const char* uri)
{
  PsychicHttpServerEndpoint *handler = new PsychicHttpServerEndpoint(this, HTTP_GET);
  handler->isWebsocket = true;
  this->endpoints.push_back(handler);

  // URI handler structure
  httpd_uri_t my_uri {
    .uri      = uri,
    .method   = HTTP_GET,
    .handler  = PsychicHttpServerEndpoint::requestHandler,
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
  this->defaultEndpoint.onRequest(fn);
}

esp_err_t PsychicHttpServer::notFoundHandler(httpd_req_t *req, httpd_err_code_t err)
{
  PsychicHttpServer *server = (PsychicHttpServer*)httpd_get_global_user_ctx(req->handle);
  PsychicHttpServerRequest request(server, req);

  esp_err_t result = ESP_OK;

  //do we have a static handler?
  if (server->staticHandler != NULL)
  {
    //wait for our file semaphore
    // if (xSemaphoreTake(xFileSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE)
    // {
      if (server->staticHandler->canHandle(&request))
        result = server->staticHandler->handleRequest(&request);
      else
        result = server->defaultEndpoint._requestCallback(&request);
    // }
    // else
    // {
    //   ESP_LOGE(PH_TAG, "Failed to get file semaphore", esp_err_to_name(err));
    //   result = request.reply(503, "text/html", "Unable to process request.");
    //   //httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get send file");
    // }

    //release our semaphore on the file lock
    // xSemaphoreGive(xFileSemaphore);
  }
  //nope, just give them the default
  else
    result = server->defaultEndpoint._requestCallback(&request);

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

/*************************************/
/*  PsychicHttpServerEndpoint        */
/*************************************/

PsychicHttpServerEndpoint::PsychicHttpServerEndpoint() :
  server(NULL),
  method(HTTP_GET),
  _requestCallback(NULL),
  _uploadCallback(NULL),
  _wsConnectCallback(NULL),
  _wsFrameCallback(NULL),
  isUpload(false),
  isWebsocket(false)
{
}

PsychicHttpServerEndpoint::PsychicHttpServerEndpoint(PsychicHttpServer *server, http_method method) :
  server(server),
  method(method),
  _requestCallback(NULL),
  _uploadCallback(NULL),
  _wsConnectCallback(NULL),
  _wsFrameCallback(NULL),
  isUpload(false),
  isWebsocket(false)
{
}

PsychicHttpServerEndpoint * PsychicHttpServerEndpoint::onRequest(PsychicHttpRequestHandler handler) {
  this->_requestCallback = handler;
  return this;
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

esp_err_t PsychicHttpServerEndpoint::requestHandler(httpd_req_t *req)
{
  esp_err_t err = ESP_OK;

  #ifdef ENABLE_ASYNC
    if (is_on_async_worker_thread() == false) {
      // submit
      if (submit_async_req(req, PsychicHttpServerEndpoint::requestHandler) == ESP_OK) {
        return ESP_OK;
      } else {
        httpd_resp_set_status(req, "503 Busy");
        httpd_resp_sendstr(req, "No workers available. Server busy.</div>");
        return ESP_OK;
      }
    }
  #endif
  
  PsychicHttpServerEndpoint *self = (PsychicHttpServerEndpoint *)req->user_ctx;

  if (self->isWebsocket)
  {
    PsychicHttpWebSocketRequest connection(self->server, req);
    err = self->_websocketHandler(connection);
  }
  else
  {
    PsychicHttpServerRequest request(self->server, req);

    //is this a file upload?
    if (self->isUpload)
      err = self->_uploadHandler(request);
    //no, its a regular request
    else
      err = self->_requestHandler(request);
  }

  return err;
}

esp_err_t PsychicHttpServerEndpoint::_requestHandler(PsychicHttpServerRequest &request)
{
  /* Request body cannot be larger than a limit */
  if (request._req->content_len > this->server->maxRequestBodySize)
  {
    ESP_LOGE(PH_TAG, "Request body too large : %d bytes", request._req->content_len);

    /* Respond with 400 Bad Request */
    char error[60];
    sprintf(error, "Request body must be less than %u bytes!", this->server->maxRequestBodySize);
    httpd_resp_send_err(request._req, HTTPD_400_BAD_REQUEST, error);

    /* Return failure to close underlying connection else the incoming file content will keep the socket busy */
    return ESP_FAIL;
  }

  //get our body loaded up.
  esp_err_t err = request.loadBody();
  if (err != ESP_OK)
    return err;

  //okay, pass on to our callback.
  if (this->_requestCallback != NULL)
    err = this->_requestCallback(&request);
  else
    err = request.reply(500, "text/html", "No onRequest callback specififed.");

  return err;
}

esp_err_t PsychicHttpServerEndpoint::_uploadHandler(PsychicHttpServerRequest &request)
{
  esp_err_t err = ESP_OK;

  /* File cannot be larger than a limit */
  if (request._req->content_len > this->server->maxUploadSize)
  {
    ESP_LOGE(PH_TAG, "File too large : %d bytes", request._req->content_len);

    /* Respond with 400 Bad Request */
    char error[50];
    sprintf(error, "File size must be less than %u bytes!", this->server->maxUploadSize);
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
  if (err == ESP_OK)
  {
    if (this->_requestCallback != NULL)
      err = this->_requestCallback(&request);
    else
      err = request.reply(200);
  }
  else
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

/*************************************/
/*  PsychicHttpServerRequest         */
/*************************************/

PsychicHttpServerRequest::PsychicHttpServerRequest(PsychicHttpServer *server, httpd_req_t *req) :
  _server(server),
  _req(req),
  _tempObject(NULL)
{
  //handle our session data
  if (req->sess_ctx != NULL)
    this->_session = (SessionData *)req->sess_ctx;
  else
  {
    this->_session = new SessionData();
    req->sess_ctx = this->_session;
  }

  //callback for freeing the session later
  req->free_ctx = this->freeSession;

  //load up some data
  this->_uri = String(this->_req->uri);

  //did we get a query string?
  size_t query_len = httpd_req_get_url_query_len(this->_req);
  if (query_len)
  {
    char query[query_len+1];
    httpd_req_get_url_query_str(this->_req, query, sizeof(query));
    this->_query.concat(query);
  }
}

PsychicHttpServerRequest::~PsychicHttpServerRequest()
{
  // if(this->_response) {
  //   delete this->_response;
  //   this->_response = NULL;
  // }

  if (_tempObject != NULL)
    free(_tempObject);
}

void PsychicHttpServerRequest::freeSession(void *ctx)
{
  if (ctx != NULL)
  {
    SessionData *session = (SessionData*)ctx;
    delete session;
  }
}

// bool PsychicHttpServerRequest::isUpload()
// {
//   if (this->header("Expect").equals("100-continue"))
//     return true;
    
//   if (this->hasHeader("Content-Disposition"))
//     return true;

//   if (this->isMultipart())
//     return true;

//   return false;
// }

const String PsychicHttpServerRequest::getFilename()
{
  //parse the content-disposition header
  if (this->hasHeader("Content-Disposition"))
  {
    ContentDisposition cd = this->getContentDisposition();
    if (cd.filename != "")
      return cd.filename;
  }

  //fall back to passed in query string
  if (this->hasParam("_filename"))
    return this->getParam("_filename");

  //fall back to parsing it from url (useful for wildcard uploads)
  String uri = this->uri();
  int filenameStart = uri.lastIndexOf('/') + 1;
  String filename = uri.substring(filenameStart);
  if (filename != "")
    return filename;

  //finally, unknown.
  ESP_LOGE(PH_TAG, "Did not get a valid filename from the upload.");
  return "unknown.txt";
}

const ContentDisposition PsychicHttpServerRequest::getContentDisposition()
{
  ContentDisposition cd;
  String header = this->header("Content-Disposition");
  int start;
  int end;

  if (header.indexOf("form-data") == 0)
    cd.disposition = FORM_DATA;
  else if (header.indexOf("attachment") == 0)
    cd.disposition = ATTACHMENT;
  else if (header.indexOf("inline") == 0)
    cd.disposition = INLINE;
  else 
    cd.disposition = NONE;

  start = header.indexOf("filename=");
  if (start)
  {
    end = header.indexOf('"', start+10);
    cd.filename = header.substring(start+10, end-1);
    DUMP(start);
    DUMP(end);
    DUMP(cd.filename);
  }

  start = header.indexOf("name=");
  if (start)
  {
    end = header.indexOf('"', start+6);
    cd.name = header.substring(start+6, end-1);

    DUMP(start);
    DUMP(end);
    DUMP(cd.name);
  }

  return cd;
}


esp_err_t PsychicHttpServerRequest::loadBody()
{
  esp_err_t err = ESP_OK;

  this->_body = String();

  //Get header value string length and allocate memory for length + 1, extra byte for null termination
  size_t remaining = this->_req->content_len;
  char *buf = (char *)malloc(remaining+1);

  //while loop for retries
  while (remaining > 0)
  {
    //read our data from the socket
    int received = httpd_req_recv(this->_req, buf, this->_req->content_len);

    //Retry if timeout occurred
    if (received == HTTPD_SOCK_ERR_TIMEOUT)
      continue;
    //bail if we got an error
    else if (received == HTTPD_SOCK_ERR_FAIL)
    {
      err = ESP_FAIL;
      break;
    }

    //keep track of our 
    remaining -= received;
  }

  //null terminate and make our string
  buf[this->_req->content_len] = '\0';
  this->_body = String(buf);

  //keep track of that pesky memory
  free(buf);

  return err;
}

http_method PsychicHttpServerRequest::method() {
  return (http_method)this->_req->method;
}

const String PsychicHttpServerRequest::methodStr() {
  return String(http_method_str((http_method)this->_req->method));
}

const String& PsychicHttpServerRequest::uri() {
  return this->_uri;
}

const String PsychicHttpServerRequest::queryString() {
  return this->_query;
}

// no way to get list of headers yet....
// int PsychicHttpServerRequest::headers()
// {
// }

const String PsychicHttpServerRequest::header(const char *name)
{
  size_t header_len = httpd_req_get_hdr_value_len(this->_req, name);

  //if we've got one, allocated it and load it
  if (header_len)
  {
    char header[header_len+1];
    httpd_req_get_hdr_value_str(this->_req, name, header, sizeof(header));
    return String(header);
  }
  else
    return "";
}

bool PsychicHttpServerRequest::hasHeader(const char *name)
{
  return httpd_req_get_hdr_value_len(this->_req, name) > 0;
}

const String PsychicHttpServerRequest::host() {
  return this->header("Host");
}

const String PsychicHttpServerRequest::contentType() {
  return header("Content-Type");
}

size_t PsychicHttpServerRequest::contentLength() {
  return this->_req->content_len;
}

const String& PsychicHttpServerRequest::body()
{
  return this->_body;
}

bool PsychicHttpServerRequest::isMultipart()
{
  const String& type = this->contentType();

  return (this->contentType().indexOf("multipart/form-data") >= 0);
}

esp_err_t PsychicHttpServerRequest::redirect(const char *url)
{
  PsychicHttpServerResponse response(this);
  response.setCode(301);
  response.addHeader("Location", url);
  
  return response.send();
}


bool PsychicHttpServerRequest::hasCookie(const char *key)
{
  char cookie[MAX_COOKIE_SIZE];
  size_t cookieSize = MAX_COOKIE_SIZE;
  esp_err_t err = httpd_req_get_cookie_val(this->_req, key, cookie, &cookieSize);

  //did we get anything?
  if (err == ESP_OK)
    return true;
  else if (err == ESP_ERR_HTTPD_RESULT_TRUNC)
    ESP_LOGE(PH_TAG, "cookie too large (%d bytes).\n", cookieSize);

  return false;
}

const String PsychicHttpServerRequest::getCookie(const char *key)
{
  char cookie[MAX_COOKIE_SIZE];
  size_t cookieSize = MAX_COOKIE_SIZE;
  esp_err_t err = httpd_req_get_cookie_val(this->_req, key, cookie, &cookieSize);

  //did we get anything?
  if (err == ESP_OK)
    return String(cookie);
  else
    return "";
}

bool PsychicHttpServerRequest::hasParam(const char *key)
{
  String query = this->queryString();
  char value[query.length()];
  esp_err_t err = httpd_query_key_value(query.c_str(), key, value, query.length());

  //did we get anything?
  if (err == ESP_OK || err == ESP_ERR_HTTPD_RESULT_TRUNC)
    return true;
  else
    return false;
}

const String PsychicHttpServerRequest::getParam(const char *key)
{
  esp_err_t err;

  //TODO: we need urldecoding here.
  //POST parameters are in the body:
  if (this->method() == HTTP_POST && this->contentType() == "application/x-www-form-urlencoded")
  {
    String body = this->body();
    char pvalue[body.length()];
    err = httpd_query_key_value(body.c_str(), key, pvalue, body.length());
    if (err == ESP_OK)
      return urlDecode(pvalue);
  }
  
  //okay, look for it in our query string.
  String query = this->queryString();
  char gvalue[query.length()];
  err = httpd_query_key_value(query.c_str(), key, gvalue, query.length());
  if (err == ESP_OK)
    return urlDecode(gvalue);

  return "";
}

bool PsychicHttpServerRequest::hasSessionKey(const String& key)
{
  return this->_session->find(key) != this->_session->end();
}

const String PsychicHttpServerRequest::getSessionKey(const String& key)
{
  auto it = this->_session->find(key);
  if (it != this->_session->end())
    return it->second;
  else
    return "";
}

void PsychicHttpServerRequest::setSessionKey(const String& key, const String& value)
{
  this->_session->insert(std::pair<String, String>(key, value));
}

static const String md5str(const String &in){
  MD5Builder md5 = MD5Builder();
  md5.begin();
  md5.add(in);
  md5.calculate();
  return md5.toString();
}

bool PsychicHttpServerRequest::authenticate(const char * username, const char * password)
{
  if(hasHeader("Authorization"))
  {
    String authReq = header("Authorization");
    if(authReq.startsWith("Basic")){
      authReq = authReq.substring(6);
      authReq.trim();
      char toencodeLen = strlen(username)+strlen(password)+1;
      char *toencode = new char[toencodeLen + 1];
      if(toencode == NULL){
        authReq = "";
        return false;
      }
      char *encoded = new char[base64_encode_expected_len(toencodeLen)+1];
      if(encoded == NULL){
        authReq = "";
        delete[] toencode;
        return false;
      }
      sprintf(toencode, "%s:%s", username, password);
      if(base64_encode_chars(toencode, toencodeLen, encoded) > 0 && authReq.equalsConstantTime(encoded)) {
        authReq = "";
        delete[] toencode;
        delete[] encoded;
        return true;
      }
      delete[] toencode;
      delete[] encoded;
    }
    else if(authReq.startsWith(F("Digest")))
    {
      authReq = authReq.substring(7);
      String _username = _extractParam(authReq,F("username=\""),'\"');
      if(!_username.length() || _username != String(username)) {
        authReq = "";
        return false;
      }
      // extracting required parameters for RFC 2069 simpler Digest
      String _realm    = _extractParam(authReq, F("realm=\""),'\"');
      String _nonce    = _extractParam(authReq, F("nonce=\""),'\"');
      String _uri      = _extractParam(authReq, F("uri=\""),'\"');
      String _resp = _extractParam(authReq, F("response=\""),'\"');
      String _opaque   = _extractParam(authReq, F("opaque=\""),'\"');

      if((!_realm.length()) || (!_nonce.length()) || (!_uri.length()) || (!_resp.length()) || (!_opaque.length())) {
        authReq = "";
        return false;
      }
      if((_opaque != this->getSessionKey("opaque")) || (_nonce != this->getSessionKey("nonce")) || (_realm != this->getSessionKey("realm")))
      {
        authReq = "";
        return false;
      }
      // parameters for the RFC 2617 newer Digest
      String _nc,_cnonce;
      if(authReq.indexOf("qop=auth") != -1 || authReq.indexOf("qop=\"auth\"") != -1) {
        _nc = _extractParam(authReq, F("nc="), ',');
        _cnonce = _extractParam(authReq, F("cnonce=\""),'\"');
      }
      String _H1 = md5str(String(username) + ':' + _realm + ':' + String(password));
      ESP_LOGD(PH_TAG, "Hash of user:realm:pass=%s", _H1);
      String _H2 = "";
      if(_method == HTTP_GET){
          _H2 = md5str(String(F("GET:")) + _uri);
      }else if(_method == HTTP_POST){
          _H2 = md5str(String(F("POST:")) + _uri);
      }else if(_method == HTTP_PUT){
          _H2 = md5str(String(F("PUT:")) + _uri);
      }else if(_method == HTTP_DELETE){
          _H2 = md5str(String(F("DELETE:")) + _uri);
      }else{
          _H2 = md5str(String(F("GET:")) + _uri);
      }
      ESP_LOGD(PH_TAG, "Hash of GET:uri=%s", _H2);
      String _responsecheck = "";
      if(authReq.indexOf("qop=auth") != -1 || authReq.indexOf("qop=\"auth\"") != -1) {
          _responsecheck = md5str(_H1 + ':' + _nonce + ':' + _nc + ':' + _cnonce + F(":auth:") + _H2);
      } else {
          _responsecheck = md5str(_H1 + ':' + _nonce + ':' + _H2);
      }
      ESP_LOGD(PH_TAG, "The Proper response=%s", _responsecheck);
      if(_resp == _responsecheck){
        authReq = "";
        return true;
      }
    }
    authReq = "";
  }
  return false;
}

const String PsychicHttpServerRequest::_extractParam(const String& authReq, const String& param, const char delimit)
{
  int _begin = authReq.indexOf(param);
  if (_begin == -1)
    return "";
  return authReq.substring(_begin+param.length(),authReq.indexOf(delimit,_begin+param.length()));
}

const String PsychicHttpServerRequest::_getRandomHexString()
{
  char buffer[33];  // buffer to hold 32 Hex Digit + /0
  int i;
  for(i = 0; i < 4; i++) {
    sprintf (buffer + (i*8), "%08lx", esp_random());
  }
  return String(buffer);
}

esp_err_t PsychicHttpServerRequest::requestAuthentication(HTTPAuthMethod mode, const char* realm, const String& authFailMsg)
{
  //what is thy realm, sire?
  if(realm == NULL)
    this->setSessionKey("realm", "Login Required");
  else
    this->setSessionKey("realm", realm);

  PsychicHttpServerResponse response(this);
  String authStr;

  //what kind of auth?
  if(mode == BASIC_AUTH)
  {
    authStr = "Basic realm=\"" + this->getSessionKey("realm") + "\"";
    response.addHeader("WWW-Authenticate", authStr.c_str());
  }
  else
  {
    this->setSessionKey("nonce", _getRandomHexString());
    this->setSessionKey("opaque", _getRandomHexString());

    authStr = "Digest realm=\"" + this->getSessionKey("realm") + "\", qop=\"auth\", nonce=\"" + this->getSessionKey("nonce") + "\", opaque=\"" + this->getSessionKey("opaque") + "\"";

    response.addHeader("WWW-Authenticate", authStr.c_str());
  }

  response.setCode(401);
  response.setContentType("text/html");
  response.setContent(authFailMsg.c_str());
  return response.send();
}

esp_err_t PsychicHttpServerRequest::reply(int code)
{
  PsychicHttpServerResponse response(this);

  response.setCode(code);
  response.setContentType("text/plain");
  response.setContent(http_status_reason(code));

  return response.send();
}

esp_err_t PsychicHttpServerRequest::reply(const char *content)
{
  PsychicHttpServerResponse response(this);

  response.setCode(200);
  response.setContentType("text/plain");
  response.setContent(content);

  return response.send();
}

// esp_err_t PsychicHttpServerRequest::reply(int code, const char *content)
// {
//   PsychicHttpServerResponse response(this);

//   response.setCode(code);
//   response.setContentType("text/plain");
//   response.setContent(content);

//   return response.send();
// }

esp_err_t PsychicHttpServerRequest::reply(int code, const char *contentType, const char *content)
{
  PsychicHttpServerResponse response(this);

  response.setCode(code);
  response.setContentType(contentType);
  response.setContent(content);

  return response.send();
}

/*************************************/
/*  PsychicHttpServerResponse        */
/*************************************/

PsychicHttpServerResponse::PsychicHttpServerResponse(PsychicHttpServerRequest *request) :
  _request(request)
{
  this->setCode(200);
}

PsychicHttpServerResponse::~PsychicHttpServerResponse()
{
}

void PsychicHttpServerResponse::addHeader(const char *field, const char *value)
{
  //these get freed during send()
  HTTPHeader header;
  header.field =(char *)malloc(strlen(field)+1);
  header.value = (char *)malloc(strlen(value)+1);

  strlcpy(header.field, field, strlen(field)+1);
  strlcpy(header.value, value, strlen(value)+1);

  this->headers.push_back(header);
}

void PsychicHttpServerResponse::setCookie(const char *name, const char *value, unsigned long max_age)
{
  String output;
  String v = urlEncode(value);
  output = String(name) + "=" + v;
  output += "; SameSite=Lax";
  output += "; Max-Age=" + String(max_age);

  this->addHeader("Set-Cookie", output.c_str());
}

void PsychicHttpServerResponse::setCode(int code)
{
  //esp-idf makes you set the whole status.
  sprintf(this->_status, "%u %s", code, http_status_reason(code));

  httpd_resp_set_status(this->_request->_req, this->_status);
}

void PsychicHttpServerResponse::setContentType(const char *contentType)
{
  httpd_resp_set_type(this->_request->_req, contentType);
}

void PsychicHttpServerResponse::setContent(const char *content)
{
  this->body = content;
  setContentLength(strlen(content));
}

void PsychicHttpServerResponse::setContent(const uint8_t *content, size_t len)
{
  this->body = (char *)content;

  setContentLength(len);
}

const char * PsychicHttpServerResponse::getContent()
{
  return this->body;
}

size_t PsychicHttpServerResponse::getContentLength()
{
  return this->_contentLength;
}

esp_err_t PsychicHttpServerResponse::send()
{
  //get our headers out of the way first
  for (HTTPHeader header : this->headers)
    httpd_resp_set_hdr(this->_request->_req, header.field, header.value);

  //now send it off
  esp_err_t err = httpd_resp_send(this->_request->_req, this->getContent(), this->getContentLength());

  //clean up our header variables.  we have to do this since httpd_resp_send doesn't store copies
  for (HTTPHeader header : this->headers)
  {
    free(header.field);
    free(header.value);
  }
  this->headers.clear();

  if (err != ESP_OK)
  {
    ESP_LOGE(PH_TAG, "Send response failed (%s)", esp_err_to_name(err));
  }

  return err;
}

/*************************************/
/*  PsychicHttpWebSocketRequest      */
/*************************************/

PsychicHttpWebSocketRequest::PsychicHttpWebSocketRequest(PsychicHttpServer *server, httpd_req_t *req) :
  PsychicHttpServerRequest(server, req)
{
  this->connection = new PsychicHttpWebSocketConnection(this->_server, httpd_req_to_sockfd(this->_req));
}

PsychicHttpWebSocketRequest::~PsychicHttpWebSocketRequest()
{
  delete this->connection;
}

esp_err_t PsychicHttpWebSocketRequest::reply(httpd_ws_frame_t * ws_pkt)
{
  return httpd_ws_send_frame(this->_req, ws_pkt);
} 

esp_err_t PsychicHttpWebSocketRequest::reply(httpd_ws_type_t op, const void *data, size_t len)
{
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  ws_pkt.payload = (uint8_t*)data;
  ws_pkt.len = len;
  ws_pkt.type = op;

  return this->reply(&ws_pkt);
}

esp_err_t PsychicHttpWebSocketRequest::reply(const char *buf)
{
  return this->reply(HTTPD_WS_TYPE_TEXT, buf, strlen(buf));
}

/*************************************/
/*  PsychicHttpWebSocketConnection   */
/*************************************/

PsychicHttpWebSocketConnection::PsychicHttpWebSocketConnection(httpd_handle_t server, int fd) :
  _server(server),
  _fd(fd)
{
}

esp_err_t PsychicHttpWebSocketConnection::queueMessage(httpd_ws_frame_t * ws_pkt)
{
  //create a copy of this packet as its getting queued to the http server
  //freed in queueMessageCallback
  struct async_resp_arg *resp_arg = (async_resp_arg *)malloc(sizeof(struct async_resp_arg));

  //did we get the memory?
  if (resp_arg == NULL)
  {
    ESP_LOGE(PH_TAG, "queueMessage malloc failed to allocate");
    return ESP_ERR_NO_MEM;
  }

  resp_arg->hd = this->_server;
  resp_arg->fd = this->_fd;
  //freed in queueMessageCallback
  resp_arg->data = (char *)malloc(ws_pkt->len+1);

  //did we get the memory?
  if (resp_arg->data == NULL)
  {
    ESP_LOGE(PH_TAG, "httpd_queue_work malloc failed to allocate %d", ws_pkt->len+1);
    return ESP_ERR_NO_MEM;
  }

  //copy it over and send it off
  memcpy(resp_arg->data, ws_pkt->payload, ws_pkt->len+1);
  esp_err_t err = httpd_queue_work(resp_arg->hd, PsychicHttpWebSocketConnection::queueMessageCallback, resp_arg);
  if (err != ESP_OK)
    ESP_LOGE(PH_TAG, "httpd_queue_work failed with %s", esp_err_to_name(err));

  return err;
} 

esp_err_t PsychicHttpWebSocketConnection::queueMessage(httpd_ws_type_t op, const void *data, size_t len)
{
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  ws_pkt.payload = (uint8_t*)data;
  ws_pkt.len = len;
  ws_pkt.type = op;

  return this->queueMessage(&ws_pkt);
}

esp_err_t PsychicHttpWebSocketConnection::queueMessage(const char *buf)
{
  return this->queueMessage(HTTPD_WS_TYPE_TEXT, buf, strlen(buf));
}

void PsychicHttpWebSocketConnection::queueMessageCallback(void *arg)
{
  //get our handles and ids
  struct async_resp_arg *resp_arg = (async_resp_arg *)arg;
  httpd_handle_t hd = resp_arg->hd;
  int fd = resp_arg->fd;

  //construct our outgoing packet
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  ws_pkt.payload = (uint8_t*)resp_arg->data;
  ws_pkt.len = strlen(resp_arg->data);
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;

  //send the packet
  httpd_ws_send_frame_async(hd, fd, &ws_pkt);

  //clean up our data
  free(resp_arg->data);
  free(resp_arg);
}

/*************************************/
/*  PsychicStaticFileHandler         */
/*************************************/

PsychicStaticFileHandler::PsychicStaticFileHandler(const char* uri, FS& fs, const char* path, const char* cache_control)
  : _fs(fs), _uri(uri), _path(path), _default_file("index.html"), _cache_control(cache_control), _last_modified("")
{
  // Ensure leading '/'
  if (_uri.length() == 0 || _uri[0] != '/') _uri = "/" + _uri;
  if (_path.length() == 0 || _path[0] != '/') _path = "/" + _path;

  // If path ends with '/' we assume a hint that this is a directory to improve performance.
  // However - if it does not end with '/' we, can't assume a file, path can still be a directory.
  _isDir = _path[_path.length()-1] == '/';

  // Remove the trailing '/' so we can handle default file
  // Notice that root will be "" not "/"
  if (_uri[_uri.length()-1] == '/') _uri = _uri.substring(0, _uri.length()-1);
  if (_path[_path.length()-1] == '/') _path = _path.substring(0, _path.length()-1);

  // Reset stats
  _gzipFirst = false;
  _gzipStats = 0xF8;
}

PsychicStaticFileHandler& PsychicStaticFileHandler::setIsDir(bool isDir){
  _isDir = isDir;
  return *this;
}

PsychicStaticFileHandler& PsychicStaticFileHandler::setDefaultFile(const char* filename){
  _default_file = String(filename);
  return *this;
}

PsychicStaticFileHandler& PsychicStaticFileHandler::setCacheControl(const char* cache_control){
  _cache_control = String(cache_control);
  return *this;
}

PsychicStaticFileHandler& PsychicStaticFileHandler::setLastModified(const char* last_modified){
  _last_modified = String(last_modified);
  return *this;
}

PsychicStaticFileHandler& PsychicStaticFileHandler::setLastModified(struct tm* last_modified){
  char result[30];
  strftime (result,30,"%a, %d %b %Y %H:%M:%S %Z", last_modified);
  return setLastModified((const char *)result);
}

#ifdef ESP8266
PsychicStaticFileHandler& PsychicStaticFileHandler::setLastModified(time_t last_modified){
  return setLastModified((struct tm *)gmtime(&last_modified));
}

PsychicStaticFileHandler& PsychicStaticFileHandler::setLastModified(){
  time_t last_modified;
  if(time(&last_modified) == 0) //time is not yet set
    return *this;
  return setLastModified(last_modified);
}
#endif

bool PsychicStaticFileHandler::canHandle(PsychicHttpServerRequest *request)
{
  if(request->method() != HTTP_GET || !request->uri().startsWith(_uri) )
    return false;

  if (_getFile(request))
    return true;

  return false;
}

bool PsychicStaticFileHandler::_getFile(PsychicHttpServerRequest *request)
{
  // Remove the found uri
  String path = request->uri().substring(_uri.length());

  // We can skip the file check and look for default if request is to the root of a directory or that request path ends with '/'
  bool canSkipFileCheck = (_isDir && path.length() == 0) || (path.length() && path[path.length()-1] == '/');

  path = _path + path;

  // Do we have a file or .gz file
  if (!canSkipFileCheck && _fileExists(path))
    return true;

  // Can't handle if not default file
  if (_default_file.length() == 0)
    return false;

  // Try to add default file, ensure there is a trailing '/' ot the path.
  if (path.length() == 0 || path[path.length()-1] != '/')
    path += "/";
  path += _default_file;

  return _fileExists(path);
}

#ifdef ESP32
  #define FILE_IS_REAL(f) (f == true && !f.isDirectory())
#else
  #define FILE_IS_REAL(f) (f == true)
#endif

bool PsychicStaticFileHandler::_fileExists(const String& path)
{
  bool fileFound = false;
  bool gzipFound = false;

  String gzip = path + ".gz";

  if (_gzipFirst) {
    _file = _fs.open(gzip, "r");
    gzipFound = FILE_IS_REAL(_file);
    if (!gzipFound){
      _file = _fs.open(path, "r");
      fileFound = FILE_IS_REAL(_file);
    }
  } else {
    _file = _fs.open(path, "r");
    fileFound = FILE_IS_REAL(_file);
    if (!fileFound){
      _file = _fs.open(gzip, "r");
      gzipFound = FILE_IS_REAL(_file);
    }
  }

  bool found = fileFound || gzipFound;

  if (found)
  {
    _filename = path;

    // Calculate gzip statistic
    _gzipStats = (_gzipStats << 1) + (gzipFound ? 1 : 0);
    if (_gzipStats == 0x00) _gzipFirst = false; // All files are not gzip
    else if (_gzipStats == 0xFF) _gzipFirst = true; // All files are gzip
    else _gzipFirst = _countBits(_gzipStats) > 4; // IF we have more gzip files - try gzip first
  }

  return found;
}

uint8_t PsychicStaticFileHandler::_countBits(const uint8_t value) const
{
  uint8_t w = value;
  uint8_t n;
  for (n=0; w!=0; n++) w&=w-1;
  return n;
}

esp_err_t PsychicStaticFileHandler::handleRequest(PsychicHttpServerRequest *request)
{
  //TODO: re-enable authentication for static files
  // if((_username != "" && _password != "") && !request->authenticate(_username.c_str(), _password.c_str()))
  //     return request->requestAuthentication();

  if (_file == true)
  {
    String etag = String(_file.size());
    if (_last_modified.length() && _last_modified == request->header("If-Modified-Since"))
    {
      _file.close();
      request->reply(304); // Not modified
    }
    else
    if (_cache_control.length() && request->hasHeader("If-None-Match") && request->header("If-None-Match").equals(etag))
    {
      _file.close();

      PsychicHttpServerResponse response(request);
      response.addHeader("Cache-Control", _cache_control.c_str());
      response.addHeader("ETag", etag.c_str());
      response.send();
    }
    else
    {
      PsychicHttpFileResponse response(request, _fs, _filename);

      if (_last_modified.length())
        response.addHeader("Last-Modified", _last_modified.c_str());
      if (_cache_control.length()) {
        response.addHeader("Cache-Control", _cache_control.c_str());
        response.addHeader("ETag", etag.c_str());
      }

      return response.send();
    }
  } else {
    request->reply(404);
  }

  return ESP_OK;
}

/*
* File Response
* */

PsychicHttpFileResponse::PsychicHttpFileResponse(PsychicHttpServerRequest *request, FS &fs, const String& path, const String& contentType, bool download)
 : PsychicHttpServerResponse(request) {
  //_code = 200;
  _path = path;

  if(!download && !fs.exists(_path) && fs.exists(_path+".gz")){
    _path = _path+".gz";
    addHeader("Content-Encoding", "gzip");
    _sendContentLength = true;
    _chunked = false;
  }

  _content = fs.open(_path, "r");
  _contentLength = _content.size();

  if(contentType == "")
    _setContentType(path);
  else
    _contentType = contentType;
  setContentType(_contentType.c_str());

  int filenameStart = path.lastIndexOf('/') + 1;
  char buf[26+path.length()-filenameStart];
  char* filename = (char*)path.c_str() + filenameStart;

  if(download) {
    // set filename and force download
    snprintf(buf, sizeof (buf), "attachment; filename=\"%s\"", filename);
  } else {
    // set filename and force rendering
    snprintf(buf, sizeof (buf), "inline; filename=\"%s\"", filename);
  }
  addHeader("Content-Disposition", buf);
}

PsychicHttpFileResponse::PsychicHttpFileResponse(PsychicHttpServerRequest *request, File content, const String& path, const String& contentType, bool download)
 : PsychicHttpServerResponse(request) {
  _path = path;

  if(!download && String(content.name()).endsWith(".gz") && !path.endsWith(".gz")){
    addHeader("Content-Encoding", "gzip");
    _sendContentLength = true;
    _chunked = false;
  }

  _content = content;
  _contentLength = _content.size();
  setContentLength(_contentLength);

  if(contentType == "")
    _setContentType(path);
  else
    _contentType = contentType;
  setContentType(_contentType.c_str());

  int filenameStart = path.lastIndexOf('/') + 1;
  char buf[26+path.length()-filenameStart];
  char* filename = (char*)path.c_str() + filenameStart;

  if(download) {
    snprintf(buf, sizeof (buf), "attachment; filename=\"%s\"", filename);
  } else {
    snprintf(buf, sizeof (buf), "inline; filename=\"%s\"", filename);
  }
  addHeader("Content-Disposition", buf);
}

PsychicHttpFileResponse::~PsychicHttpFileResponse()
{
  if(_content)
    _content.close();
}

void PsychicHttpFileResponse::_setContentType(const String& path){
  if (path.endsWith(".html")) _contentType = "text/html";
  else if (path.endsWith(".htm")) _contentType = "text/html";
  else if (path.endsWith(".css")) _contentType = "text/css";
  else if (path.endsWith(".json")) _contentType = "application/json";
  else if (path.endsWith(".js")) _contentType = "application/javascript";
  else if (path.endsWith(".png")) _contentType = "image/png";
  else if (path.endsWith(".gif")) _contentType = "image/gif";
  else if (path.endsWith(".jpg")) _contentType = "image/jpeg";
  else if (path.endsWith(".ico")) _contentType = "image/x-icon";
  else if (path.endsWith(".svg")) _contentType = "image/svg+xml";
  else if (path.endsWith(".eot")) _contentType = "font/eot";
  else if (path.endsWith(".woff")) _contentType = "font/woff";
  else if (path.endsWith(".woff2")) _contentType = "font/woff2";
  else if (path.endsWith(".ttf")) _contentType = "font/ttf";
  else if (path.endsWith(".xml")) _contentType = "text/xml";
  else if (path.endsWith(".pdf")) _contentType = "application/pdf";
  else if (path.endsWith(".zip")) _contentType = "application/zip";
  else if(path.endsWith(".gz")) _contentType = "application/x-gzip";
  else _contentType = "text/plain";
}

esp_err_t PsychicHttpFileResponse::send()
{
  esp_err_t err = ESP_OK;

  //just send small files directly
  size_t size = getContentLength();
  if (size < FILE_CHUNK_SIZE)
  {
    uint8_t *buffer = (uint8_t *)malloc(size);
    int readSize = _content.readBytes((char *)buffer, size);

    this->setContent(buffer, size);
    err = PsychicHttpServerResponse::send();
    
    free(buffer);
  }
  else
  {
    //get our headers out of the way first
    for (HTTPHeader header : this->headers)
      httpd_resp_set_hdr(this->_request->_req, header.field, header.value);

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *chunk = (char *)malloc(FILE_CHUNK_SIZE);
    size_t chunksize;
    do {
        /* Read file in chunks into the scratch buffer */
        chunksize = _content.readBytes(chunk, FILE_CHUNK_SIZE);
        if (chunksize > 0)
        {
          /* Send the buffer contents as HTTP response chunk */
          err = httpd_resp_send_chunk(this->_request->_req, chunk, chunksize);
          if (err != ESP_OK)
          {
            ESP_LOGE(PH_TAG, "File sending failed (%s)", esp_err_to_name(err));

            /* Abort sending file */
            httpd_resp_sendstr_chunk(this->_request->_req, NULL);

            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(this->_request->_req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");

            //bail
            break;
          }
        }

        /* Keep looping till the whole file is sent */
    } while (chunksize != 0);

    //keep track of our memory
    free(chunk);

    if (err == ESP_OK)
    {
      ESP_LOGI(PH_TAG, "File sending complete");

      /* Respond with an empty chunk to signal HTTP response completion */
      httpd_resp_send_chunk(this->_request->_req, NULL, 0);
    }

    /* Close file after sending complete */
    _content.close();

    //clean up our header variables.  we have to do this since httpd_resp_send doesn't store copies
    for (HTTPHeader header : this->headers)
    {
      free(header.field);
      free(header.value);
    }
    this->headers.clear();
  }

  return err;
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

#ifdef ENABLE_ASYNC
  bool is_on_async_worker_thread(void)
  {
      // is our handle one of the known async handles?
      TaskHandle_t handle = xTaskGetCurrentTaskHandle();
      for (int i = 0; i < ASYNC_WORKER_COUNT; i++) {
          if (worker_handles[i] == handle) {
              return true;
          }
      }
      return false;
  }

// Submit an HTTP req to the async worker queue
esp_err_t submit_async_req(httpd_req_t *req, httpd_req_handler_t handler)
{
    // must create a copy of the request that we own
    httpd_req_t* copy = NULL;
    esp_err_t err = httpd_req_async_handler_begin(req, &copy);
    if (err != ESP_OK) {
        return err;
    }

    httpd_async_req_t async_req = {
        .req = copy,
        .handler = handler,
    };

    // How should we handle resource exhaustion?
    // In this example, we immediately respond with an
    // http error if no workers are available.
    int ticks = 0;

    // counting semaphore: if success, we know 1 or
    // more asyncReqTaskWorkers are available.
    if (xSemaphoreTake(worker_ready_count, ticks) == false) {
        ESP_LOGE(PH_TAG, "No workers are available");
        httpd_req_async_handler_complete(copy); // cleanup
        return ESP_FAIL;
    }

    // Since worker_ready_count > 0 the queue should already have space.
    // But lets wait up to 100ms just to be safe.
    if (xQueueSend(async_req_queue, &async_req, pdMS_TO_TICKS(100)) == false) {
        ESP_LOGE(PH_TAG, "worker queue is full");
        httpd_req_async_handler_complete(copy); // cleanup
        return ESP_FAIL;
    }

    return ESP_OK;
}

void async_req_worker_task(void *p)
{
    ESP_LOGI(PH_TAG, "starting async req task worker");

    while (true) {

        // counting semaphore - this signals that a worker
        // is ready to accept work
        xSemaphoreGive(worker_ready_count);

        // wait for a request
        httpd_async_req_t async_req;
        if (xQueueReceive(async_req_queue, &async_req, portMAX_DELAY)) {

            ESP_LOGI(PH_TAG, "invoking %s", async_req.req->uri);

            // call the handler
            async_req.handler(async_req.req);

            // Inform the server that it can purge the socket used for
            // this request, if needed.
            if (httpd_req_async_handler_complete(async_req.req) != ESP_OK) {
                ESP_LOGE(PH_TAG, "failed to complete async req");
            }
        }
    }

    ESP_LOGW(PH_TAG, "worker stopped");
    vTaskDelete(NULL);
}

void start_async_req_workers(void)
{
    // counting semaphore keeps track of available workers
    worker_ready_count = xSemaphoreCreateCounting(
        ASYNC_WORKER_COUNT,  // Max Count
        0); // Initial Count
    if (worker_ready_count == NULL) {
        ESP_LOGE(PH_TAG, "Failed to create workers counting Semaphore");
        return;
    }

    // create queue
    async_req_queue = xQueueCreate(1, sizeof(httpd_async_req_t));
    if (async_req_queue == NULL){
        ESP_LOGE(PH_TAG, "Failed to create async_req_queue");
        vSemaphoreDelete(worker_ready_count);
        return;
    }

    // start worker tasks
    for (int i = 0; i < ASYNC_WORKER_COUNT; i++) {

        bool success = xTaskCreate(async_req_worker_task, "async_req_worker",
                                    ASYNC_WORKER_TASK_STACK_SIZE, // stack size
                                    (void *)0, // argument
                                    ASYNC_WORKER_TASK_PRIORITY, // priority
                                    &worker_handles[i]);

        if (!success) {
            ESP_LOGE(PH_TAG, "Failed to start asyncReqWorker");
            continue;
        }
    }
}
#endif