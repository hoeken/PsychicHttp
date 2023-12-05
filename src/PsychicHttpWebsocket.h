#ifndef PsychicHttpWebsocket_h
#define PsychicHttpWebsocket_h

#include "PsychicCore.h"
#include "PsychicHttpServerRequest.h"

class PsychicHttpWebSocketRequest;
class PsychicHttpWebSocketConnection;

class PsychicHttpWebSocketRequest : public PsychicHttpServerRequest
{
  friend PsychicHttpServer;

  public:
    PsychicHttpWebSocketRequest(PsychicHttpServer *server, httpd_req_t *req);
    virtual ~PsychicHttpWebSocketRequest();

    PsychicHttpWebSocketConnection *connection;

    esp_err_t reply(httpd_ws_frame_t * ws_pkt);
    esp_err_t reply(httpd_ws_type_t op, const void *data, size_t len);
    esp_err_t reply(const char *buf);
};

class PsychicHttpWebSocketConnection
{
  friend PsychicHttpServer;

  protected:
    httpd_handle_t _server;
    int _fd;

  public:
    PsychicHttpWebSocketConnection(httpd_handle_t server, int fd);

    esp_err_t queueMessage(httpd_ws_frame_t * ws_pkt);
    esp_err_t queueMessage(httpd_ws_type_t op, const void *data, size_t len);
    esp_err_t queueMessage(const char *buf);
    //static void queueMessageCallback(void *arg);

    int id() { return this->_fd; }
};

//callback definitions
typedef std::function<esp_err_t(PsychicHttpWebSocketRequest *connection)> PsychicHttpWebSocketRequestHandler;
typedef std::function<esp_err_t(PsychicHttpWebSocketRequest *connection, httpd_ws_frame *frame)> PsychicHttpWebSocketFrameHandler;

#endif // PsychicHttpWebsocket_h