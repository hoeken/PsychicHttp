#ifndef PsychicHttpServer_h
#define PsychicHttpServer_h

#include "PsychicCore.h"

class PsychicHttpServerEndpoint;
class PsychicWebHandler;
class PsychicStaticFileHandler;

class PsychicHttpServer
{
  protected:
    bool use_ssl = false;
    std::list<PsychicHttpServerEndpoint *> endpoints;
    LinkedList<PsychicWebHandler*> _handlers;

    esp_err_t _start();

  public:
    PsychicHttpServer();
    ~PsychicHttpServer();

    //esp-idf specific stuff
    httpd_handle_t server;
    httpd_config_t config;

    httpd_ssl_config_t ssl_config;

    //some limits on what we will accept
    unsigned long maxUploadSize = 200 * 1024;
    unsigned long maxRequestBodySize = 16 * 1024;

    PsychicHttpServerEndpoint *defaultEndpoint;
    PsychicHttpConnectionHandler openHandler;
    PsychicHttpConnectionHandler closeHandler;

    static void destroy(void *ctx);

    esp_err_t listen(uint16_t port);
    esp_err_t listen(uint16_t port, const char *cert, const char *private_key);
    void stop();

    PsychicWebHandler& addHandler(PsychicWebHandler* handler);
    bool removeHandler(PsychicWebHandler* handler);

    PsychicHttpServerEndpoint *on(const char* uri);
    PsychicHttpServerEndpoint *on(const char* uri, http_method method);
    PsychicHttpServerEndpoint *on(const char* uri, PsychicHttpRequestHandler onRequest);
    PsychicHttpServerEndpoint *on(const char* uri, http_method method, PsychicHttpRequestHandler onRequest);

    PsychicHttpServerEndpoint *websocket(const char* uri);

    void onNotFound(PsychicHttpRequestHandler fn);
    static esp_err_t notFoundHandler(httpd_req_t *req, httpd_err_code_t err);
    static esp_err_t defaultNotFoundHandler(PsychicHttpServerRequest *request);

    void onOpen(PsychicHttpConnectionHandler handler);
    void onClose(PsychicHttpConnectionHandler handler);
    static esp_err_t openCallback(httpd_handle_t hd, int sockfd);
    static void closeCallback(httpd_handle_t hd, int sockfd);

    PsychicStaticFileHandler *staticHandler;
    PsychicStaticFileHandler& serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_control = NULL);

    void sendAll(httpd_ws_frame_t * ws_pkt);
    void sendAll(httpd_ws_type_t op, const void *data, size_t len);
    void sendAll(const char *buf);
};

#endif // PsychicHttpServer_h