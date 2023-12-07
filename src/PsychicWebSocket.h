#ifndef PsychicWebSocket_h
#define PsychicWebSocket_h

#include "PsychicCore.h"
#include "PsychicRequest.h"

class PsychicWebSocketRequest;
class PsychicWebSocketClient;

//callback function definitions
typedef std::function<esp_err_t(PsychicWebSocketClient *client)> PsychicWebSocketClientCallback;
typedef std::function<esp_err_t(PsychicWebSocketRequest *request, httpd_ws_frame *frame)> PsychicWebSocketFrameCallback;

class PsychicWebSocketRequest : public PsychicRequest
{
  public:
    PsychicWebSocketRequest(PsychicRequest *req, PsychicWebSocketClient *client);
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

class PsychicWebSocketHandler : public PsychicHandler {
  protected:
    std::list<PsychicWebSocketClient*> _clients;

    PsychicWebSocketClientCallback _onOpen;
    PsychicWebSocketFrameCallback _onFrame;
    PsychicWebSocketClientCallback _onClose;

  public:
    PsychicWebSocketHandler();
    ~PsychicWebSocketHandler();

    bool isWebSocket() override;
    bool canHandle(PsychicRequest *request) override final;
    esp_err_t handleRequest(PsychicRequest *request) override final;

    void closeCallback(PsychicClient *client) override;
    void addClient(PsychicWebSocketClient *client);
    void removeClient(PsychicWebSocketClient *client);
    PsychicWebSocketClient * getClient(PsychicClient *client);
    bool hasClient(PsychicClient *client);

    PsychicWebSocketHandler *onOpen(PsychicWebSocketClientCallback fn);
    PsychicWebSocketHandler *onFrame(PsychicWebSocketFrameCallback fn);
    PsychicWebSocketHandler *onClose(PsychicWebSocketClientCallback fn);

    void sendAll(httpd_ws_frame_t * ws_pkt);
    void sendAll(httpd_ws_type_t op, const void *data, size_t len);
    void sendAll(const char *buf);
};

#endif // PsychicWebSocket_h