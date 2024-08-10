#ifndef PsychicEndpoint_h
#define PsychicEndpoint_h

#include "PsychicCore.h"

class PsychicHandler;

#ifdef ENABLE_ASYNC
  #include "async_worker.h"
#endif

class PsychicEndpoint
{
    friend PsychicHttpServer;

  private:
    PsychicHttpServer* _server;
    String _uri;
    int _method;
    PsychicHandler* _handler;
    httpd_uri_match_func_t _uri_match_fn = nullptr; // use this change the endpoint matching function.

  public:
    PsychicEndpoint();
    PsychicEndpoint(PsychicHttpServer* server, int method, const char* uri);

    PsychicEndpoint* setHandler(PsychicHandler* handler);
    PsychicHandler* handler();

    httpd_uri_match_func_t getURIMatchFunction();
    void setURIMatchFunction(httpd_uri_match_func_t match_fn);

    bool matches(const char* uri);

    PsychicEndpoint* setFilter(PsychicFilterFunction fn);
    PsychicEndpoint* setFilter(PsychicRequestFilterFunction fn);
    PsychicEndpoint* setAuthentication(const char* username, const char* password, HTTPAuthMethod method = BASIC_AUTH, const char* realm = "", const char* authFailMsg = "");

    String uri();

    static esp_err_t requestCallback(httpd_req_t* req);
};

#endif // PsychicEndpoint_h