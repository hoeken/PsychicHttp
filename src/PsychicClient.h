#ifndef PsychicClient_h
#define PsychicClient_h

#include "PsychicCore.h"

/*
* PsychicClient :: Generic wrapper around the ESP-IDF socket
*/

#include "PsychicCore.h"

class PsychicClient {
  protected:
    httpd_handle_t _server;
    int _socket;

  public:
    PsychicClient(httpd_handle_t server, int socket);
    ~PsychicClient();

    bool operator==(PsychicClient& rhs) const { return _socket == rhs.socket(); }

    httpd_handle_t server();
    int socket();

    IPAddress localIP();
};

#endif