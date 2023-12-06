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

    PsychicHttpWebSocketRequestHandler _wsConnectCallback;
    PsychicHttpWebSocketFrameHandler _wsFrameCallback;
    PsychicHttpConnectionHandler _wsCloseCallback;

  public:
    PsychicHttpServerEndpoint();
    PsychicHttpServerEndpoint(PsychicHttpServer *server, http_method method, const char * uri);

    std::list<int> websocketConnections;

    bool isUpload;
    bool isWebsocket;

    PsychicHttpServerEndpoint *setHandler(PsychicHandler *handler);
    PsychicHandler *handler();

    String uri();

    PsychicHttpServerEndpoint *onConnect(PsychicHttpWebSocketRequestHandler handler);
    PsychicHttpServerEndpoint *onFrame(PsychicHttpWebSocketFrameHandler handler);
    PsychicHttpServerEndpoint *onClose(PsychicHttpConnectionHandler handler);

    static esp_err_t requestCallback(httpd_req_t *req);

    esp_err_t _websocketHandler(PsychicHttpWebSocketRequest &request);
};

#endif // PsychicHttpServerEndpoint_h