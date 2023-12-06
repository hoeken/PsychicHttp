#ifndef PsychicHttpWebsocket_h
#define PsychicHttpWebsocket_h

#include "PsychicCore.h"
#include "PsychicHttpServerEndpoint.h"
#include "PsychicHttpServerRequest.h"

class PsychicWebSocketRequest;
class PsychicWebSocketClient;

//callback function definitions
typedef std::function<esp_err_t(PsychicWebSocketClient *client)> PsychicWebsocketClientCallback;
typedef std::function<esp_err_t(PsychicWebSocketRequest *request, httpd_ws_frame *frame)> PsychicWebSocketFrameCallback;

class PsychicWebSocketRequest : public PsychicHttpServerRequest
{
  public:
    PsychicWebSocketRequest(PsychicHttpServerRequest *req, PsychicWebSocketClient *client);
    virtual ~PsychicWebSocketRequest();

    PsychicWebSocketClient *wsclient;

    esp_err_t reply(httpd_ws_frame_t * ws_pkt);
    esp_err_t reply(httpd_ws_type_t op, const void *data, size_t len);
    esp_err_t reply(const char *buf);
};

class PsychicWebSocketClient : public PsychicClient
{
  public:
    PsychicWebSocketClient(PsychicClient *client);

    esp_err_t sendMessage(httpd_ws_frame_t * ws_pkt);
    esp_err_t sendMessage(httpd_ws_type_t op, const void *data, size_t len);
    esp_err_t sendMessage(const char *buf);
};

class PsychicWebsocketHandler : public PsychicHandler {
  protected:
    std::list<PsychicWebSocketClient*> _clients;

    PsychicWebsocketClientCallback _onOpen;
    PsychicWebSocketFrameCallback _onFrame;
    PsychicWebsocketClientCallback _onClose;

  public:
    PsychicWebsocketHandler();
    ~PsychicWebsocketHandler();

    bool isWebsocket() override;
    bool canHandle(PsychicHttpServerRequest *request);
    esp_err_t handleRequest(PsychicHttpServerRequest *request);

    void closeCallback(PsychicClient *client) override;
    void addClient(PsychicWebSocketClient *client);
    void removeClient(PsychicWebSocketClient *client);
    PsychicWebSocketClient * getClient(PsychicClient *client);
    bool hasClient(PsychicClient *client);

    PsychicWebsocketHandler *onOpen(PsychicWebsocketClientCallback fn);
    PsychicWebsocketHandler *onFrame(PsychicWebSocketFrameCallback fn);
    PsychicWebsocketHandler *onClose(PsychicWebsocketClientCallback fn);

    void sendAll(httpd_ws_frame_t * ws_pkt);
    void sendAll(httpd_ws_type_t op, const void *data, size_t len);
    void sendAll(const char *buf);
};

#endif // PsychicHttpWebsocket_h