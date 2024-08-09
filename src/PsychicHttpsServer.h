#ifndef PsychicHttpsServer_h
#define PsychicHttpsServer_h

#ifdef CONFIG_ESP_HTTPS_SERVER_ENABLE

#include "PsychicCore.h"
#include "PsychicHttpServer.h"
#include <esp_https_server.h>
#if !CONFIG_HTTPD_WS_SUPPORT
  #error PsychicHttpsServer cannot be used unless HTTPD_WS_SUPPORT is enabled in esp-http-server component configuration
#endif

#define PSY_ENABLE_SSL //you can use this define in your code to enable/disable these features

class PsychicHttpsServer : public PsychicHttpServer
{
  protected:
    virtual esp_err_t _startServer() override final;
    virtual esp_err_t _stopServer() override final;

  public:
    PsychicHttpsServer(uint16_t port = 443, const char *cert = nullptr, const char *private_key = nullptr);
    ~PsychicHttpsServer();

    httpd_ssl_config_t ssl_config;

    using PsychicHttpServer::listen; //keep the regular version
    virtual void setPort(uint16_t port) override final;
    void setCertificate(const char *cert, const char *private_key);
};

#endif // PsychicHttpsServer_h

#endif // CONFIG_ESP_HTTPS_SERVER_ENABLE