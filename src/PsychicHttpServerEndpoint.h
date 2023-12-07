#ifndef PsychicHttpServerEndpoint_h
#define PsychicHttpServerEndpoint_h

#include "PsychicCore.h"
#include "PsychicHttpServer.h"

class PsychicHttpServerEndpoint
{
  friend PsychicHttpServer;

  private:
    PsychicHttpServer *_server;
    String _uri;
    http_method _method;
    PsychicHandler *_handler;

  public:
    PsychicHttpServerEndpoint();
    PsychicHttpServerEndpoint(PsychicHttpServer *server, http_method method, const char * uri);

    PsychicHttpServerEndpoint *setHandler(PsychicHandler *handler);
    PsychicHandler *handler();

    String uri();

    static esp_err_t requestCallback(httpd_req_t *req);
};

#endif // PsychicHttpServerEndpoint_h