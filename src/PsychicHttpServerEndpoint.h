#ifndef PsychicHttpServerEndpoint_h
#define PsychicHttpServerEndpoint_h

#include "PsychicCore.h"
#include "PsychicHttpServer.h"

class PsychicHttpServerEndpoint
{
  friend PsychicHttpServer;

  private:
    PsychicHttpServer *server;
    http_method method;

    PsychicHttpRequestHandler _requestCallback;
    PsychicHttpBasicUploadHandler _uploadCallback;
    PsychicHttpMultipartUploadHandler _multipartCallback;
    PsychicHttpWebSocketRequestHandler _wsConnectCallback;
    PsychicHttpWebSocketFrameHandler _wsFrameCallback;
    PsychicHttpConnectionHandler _wsCloseCallback;

  public:
    PsychicHttpServerEndpoint();
    PsychicHttpServerEndpoint(PsychicHttpServer *server, http_method method);

    std::list<int> websocketConnections;

    bool isUpload;
    bool isWebsocket;

    PsychicHttpServerEndpoint *onRequest(PsychicHttpRequestHandler handler);
    PsychicHttpServerEndpoint *onUpload(PsychicHttpBasicUploadHandler handler);
    PsychicHttpServerEndpoint *onMultipart(PsychicHttpMultipartUploadHandler handler);
    PsychicHttpServerEndpoint *onConnect(PsychicHttpWebSocketRequestHandler handler);
    PsychicHttpServerEndpoint *onFrame(PsychicHttpWebSocketFrameHandler handler);
    PsychicHttpServerEndpoint *onClose(PsychicHttpConnectionHandler handler);

    static esp_err_t requestHandler(httpd_req_t *req);

    esp_err_t _requestHandler(PsychicHttpServerRequest &request);
    esp_err_t _uploadHandler(PsychicHttpServerRequest &request);
    esp_err_t _multipartUploadHandler(PsychicHttpServerRequest &request);
    esp_err_t _basicUploadHandler(PsychicHttpServerRequest &request);
    esp_err_t _websocketHandler(PsychicHttpWebSocketRequest &request);
};

#endif // PsychicHttpServerEndpoint_h