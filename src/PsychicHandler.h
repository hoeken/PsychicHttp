#ifndef PsychicHandler_h
#define PsychicHandler_h

#include "PsychicCore.h"
#include "PsychicHttpServerRequest.h"

/*
* HANDLER :: Can be attached to any endpoint or as a generic request handler.
*/

class PsychicHandler {
  protected:
    PsychicRequestFilterFunction _filter;
    PsychicClientCallback _onopen;
    PsychicClientCallback _onclose;
    String _username;
    String _password;

  public:
    PsychicHandler();
    ~PsychicHandler();

    PsychicHandler& onOpen(PsychicClientCallback fn);
    PsychicHandler& onClose(PsychicClientCallback fn);

    PsychicHandler& setFilter(PsychicRequestFilterFunction fn);
    bool filter(PsychicHttpServerRequest *request);
    //PsychicHandler& setAuthentication(const char *username, const char *password);

    virtual void closeCallback(PsychicClient *client);
    virtual bool isWebsocket();

    //derived classes must implement these functions
    virtual bool canHandle(PsychicHttpServerRequest *request) { return true; };
    virtual esp_err_t handleRequest(PsychicHttpServerRequest *request) = 0;
};

#endif