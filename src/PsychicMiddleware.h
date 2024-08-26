#ifndef PsychicMiddleware_h
#define PsychicMiddleware_h

#include "PsychicCore.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"

class PsychicMiddlewareChain;
/*
 * PsychicMiddleware :: fancy callback wrapper for handling requests and responses.
 * */

class PsychicMiddleware
{
  private:
    bool _managed = false;
    friend PsychicMiddlewareChain;

  public:
    virtual ~PsychicMiddleware() {}
    virtual esp_err_t run(PsychicRequest* request, PsychicResponse* response, PsychicMiddlewareCallback next) = 0;
};

class PsychicMiddlewareClosure : public PsychicMiddleware
{
  protected:
    PsychicMiddlewareFunction _fn;

  public:
    PsychicMiddlewareClosure(PsychicMiddlewareFunction fn);
    esp_err_t run(PsychicRequest* request, PsychicResponse* response, PsychicMiddlewareCallback next) override;
};

#endif