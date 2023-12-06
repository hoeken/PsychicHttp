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
    PsychicHttpConnectionHandler _onopen;
    PsychicHttpConnectionHandler _onclose;
    String _username;
    String _password;
  public:
    PsychicHandler();
    virtual ~PsychicHandler();

    PsychicHandler& onOpen(PsychicHttpConnectionHandler fn);
    PsychicHandler& onClose(PsychicHttpConnectionHandler fn);

    PsychicHandler& setFilter(PsychicRequestFilterFunction fn);
    bool filter(PsychicHttpServerRequest *request);
    PsychicHandler& setAuthentication(const char *username, const char *password);

    virtual bool canHandle(PsychicHttpServerRequest *request __attribute__((unused)));
    virtual void handleRequest(PsychicHttpServerRequest *request __attribute__((unused)));
    virtual void handle(PsychicHttpServerRequest *request __attribute__((unused)));
};

#endif