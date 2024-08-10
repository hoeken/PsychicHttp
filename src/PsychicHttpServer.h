#ifndef PsychicHttpServer_h
#define PsychicHttpServer_h

#include "PsychicClient.h"
#include "PsychicCore.h"
#include "PsychicHandler.h"
#include "PsychicRewrite.h"

#ifdef PSYCHIC_REGEX
#include <regex>
#endif

enum PsychicHttpMethod
{
  HTTP_ANY = 99
};

class PsychicEndpoint;
class PsychicHandler;
class PsychicStaticFileHandler;

class PsychicHttpServer
{
  protected:
    std::list<httpd_uri_t> _esp_idf_endpoints;
    std::list<PsychicEndpoint*> _endpoints;
    std::list<PsychicHandler*> _handlers;
    std::list<PsychicClient*> _clients;
    std::list<PsychicRewrite*> _rewrites;

    PsychicClientCallback _onOpen;
    PsychicClientCallback _onClose;

    esp_err_t _start();
    virtual esp_err_t _startServer();
    virtual esp_err_t _stopServer();
    bool _running = false;
    httpd_uri_match_func_t _uri_match_fn = nullptr;

    bool _rewriteRequest(PsychicRequest *request);

  public:
    PsychicHttpServer(uint16_t port = 80);
    virtual ~PsychicHttpServer();

    // what methods to support
    std::list<http_method> supported_methods = {HTTP_GET, HTTP_POST, HTTP_DELETE, HTTP_HEAD, HTTP_PUT};

    // esp-idf specific stuff
    httpd_handle_t server;
    httpd_config_t config;

    // some limits on what we will accept
    unsigned long maxUploadSize;
    unsigned long maxRequestBodySize;

    PsychicEndpoint* defaultEndpoint;

    static void destroy(void* ctx);

    virtual void setPort(uint16_t port);

    bool isRunning() { return _running; }
    esp_err_t begin() { return start(); }
    esp_err_t end() { return stop(); }
    esp_err_t start();
    esp_err_t stop();

    httpd_uri_match_func_t getURIMatchFunction();
    void setURIMatchFunction(httpd_uri_match_func_t match_fn);

    PsychicRewrite* addRewrite(PsychicRewrite* rewrite);
    void removeRewrite(PsychicRewrite* rewrite);
    PsychicRewrite* rewrite(const char* from, const char* to);

    PsychicHandler* addHandler(PsychicHandler* handler);
    void removeHandler(PsychicHandler* handler);

    void addClient(PsychicClient* client);
    void removeClient(PsychicClient* client);
    PsychicClient* getClient(int socket);
    PsychicClient* getClient(httpd_req_t* req);
    bool hasClient(int socket);
    int count() { return _clients.size(); };
    const std::list<PsychicClient*>& getClientList();

    PsychicEndpoint* on(const char* uri);
    PsychicEndpoint* on(const char* uri, int method);
    PsychicEndpoint* on(const char* uri, PsychicHandler* handler);
    PsychicEndpoint* on(const char* uri, int method, PsychicHandler* handler);
    PsychicEndpoint* on(const char* uri, PsychicHttpRequestCallback onRequest);
    PsychicEndpoint* on(const char* uri, int method, PsychicHttpRequestCallback onRequest);
    PsychicEndpoint* on(const char* uri, PsychicJsonRequestCallback onRequest);
    PsychicEndpoint* on(const char* uri, int method, PsychicJsonRequestCallback onRequest);

    static esp_err_t requestHandler(httpd_req_t* req);
    static esp_err_t notFoundHandler(httpd_req_t* req, httpd_err_code_t err);
    static esp_err_t defaultNotFoundHandler(PsychicRequest* request);
    void onNotFound(PsychicHttpRequestCallback fn);

    void onOpen(PsychicClientCallback handler);
    void onClose(PsychicClientCallback handler);
    static esp_err_t openCallback(httpd_handle_t hd, int sockfd);
    static void closeCallback(httpd_handle_t hd, int sockfd);

    PsychicStaticFileHandler* serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_control = NULL);
};

bool ON_STA_FILTER(PsychicRequest* request);
bool ON_AP_FILTER(PsychicRequest* request);

//URI matching functions
bool psychic_uri_match_simple(const char* uri1, const char* uri2, size_t len2);
#define MATCH_SIMPLE   psychic_uri_match_simple
#define MATCH_WILDCARD httpd_uri_match_wildcard

#ifdef PSYCHIC_REGEX
bool psychic_uri_match_regex(const char* uri1, const char* uri2, size_t len2);
#define MATCH_REGEX psychic_uri_match_regex
#endif

#endif // PsychicHttpServer_h