#include "PsychicHttpsServer.h"

#ifdef CONFIG_ESP_HTTPS_SERVER_ENABLE

PsychicHttpsServer::PsychicHttpsServer(uint16_t port, const char* cert, const char* private_key) : PsychicHttpServer(port)
{
  // for a SSL server
  ssl_config = HTTPD_SSL_CONFIG_DEFAULT();
  ssl_config.httpd.open_fn = PsychicHttpServer::openCallback;
  ssl_config.httpd.close_fn = PsychicHttpServer::closeCallback;
  ssl_config.httpd.uri_match_fn = httpd_uri_match_wildcard;
  ssl_config.httpd.global_user_ctx = this;
  ssl_config.httpd.global_user_ctx_free_fn = destroy;
  ssl_config.httpd.max_uri_handlers = 20;

  // each SSL connection takes about 45kb of heap
  // a barebones sketch with PsychicHttp has ~150kb of heap available
  // if we set it higher than 2 and use all the connections, we get lots of memory errors.
  // not to mention there is no heap left over for the program itself.
  ssl_config.httpd.max_open_sockets = 2;

  setPort(port);
  setCertificate(cert, private_key);
}

PsychicHttpsServer::~PsychicHttpsServer() {}

void PsychicHttpsServer::setPort(uint16_t port)
{
  this->ssl_config.port_secure = port;
}

void PsychicHttpsServer::setCertificate(const char* cert, const char* private_key)
{
  if (cert)
  {
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 2)
    this->ssl_config.servercert = (uint8_t*)cert;
    this->ssl_config.servercert_len = strlen(cert) + 1;
  #else
    this->ssl_config.cacert_pem = (uint8_t*)cert;
    this->ssl_config.cacert_len = strlen(cert) + 1;
  #endif
  }

  if (private_key)
  {
    this->ssl_config.prvtkey_pem = (uint8_t*)private_key;
    this->ssl_config.prvtkey_len = strlen(private_key) + 1;
  }
}

esp_err_t PsychicHttpsServer::_startServer()
{
  return httpd_ssl_start(&this->server, &this->ssl_config);
}

esp_err_t PsychicHttpsServer::_stopServer()
{
  ret = httpd_ssl_stop(this->server);
}

#endif // CONFIG_ESP_HTTPS_SERVER_ENABLE