#ifndef PsychicMiddlewareChain_h
#define PsychicMiddlewareChain_h

#include "PsychicCore.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"
#include "PsychicMiddleware.h"

/*
 * PsychicMiddlewareChain - handle tracking and executing our chain of middleware objects
 * */

class PsychicMiddlewareChain
{
  protected:
    std::list<PsychicMiddleware*> _middleware;

  public:
    PsychicMiddlewareChain();
    virtual ~PsychicMiddlewareChain();

    void add(PsychicMiddleware* middleware);
    void add(PsychicMiddlewareFunction fn);
    void remove(PsychicMiddleware* middleware);

    esp_err_t run(PsychicRequest* request, PsychicResponse* response, PsychicMiddlewareCallback finalizer);
};

#endif