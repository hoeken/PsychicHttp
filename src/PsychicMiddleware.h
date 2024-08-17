#ifndef PsychicMiddleware_h
#define PsychicMiddleware_h

#include "PsychicCore.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"

/*
 * PsychicMiddleware :: fancy callback wrapper for handling requests and responses.
 * */

class PsychicMiddleware
{
  public:
    virtual ~PsychicMiddleware() {}
    virtual esp_err_t run(PsychicMiddlewareCallback next, PsychicRequest* request, PsychicResponse* response) = 0;
};

class PsychicMiddlewareClosure : public PsychicMiddleware
{
  protected:
    PsychicMiddlewareFunction _fn;

  public:
    PsychicMiddlewareClosure(PsychicMiddlewareFunction fn);
    esp_err_t run(PsychicMiddlewareCallback next, PsychicRequest* request, PsychicResponse* response) override;
};

#endif