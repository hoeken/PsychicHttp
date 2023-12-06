#ifndef PsychicWebHandler_h
#define PsychicWebHandler_h

#include "PsychicCore.h"
#include "PsychicHttpServer.h"
#include "PsychicHttpServerRequest.h"
#include "PsychicHandler.h"

/*
* HANDLER :: Can be attached to any endpoint or as a generic request handler.
*/

class PsychicWebHandler : public PsychicHandler {
  protected:
    PsychicHttpRequestHandler _requestCallback;

  public:
    PsychicWebHandler();
    ~PsychicWebHandler();

    bool canHandle(PsychicHttpServerRequest *request);
    esp_err_t handleRequest(PsychicHttpServerRequest *request);
    PsychicWebHandler * onRequest(PsychicHttpRequestHandler fn);
};

#endif