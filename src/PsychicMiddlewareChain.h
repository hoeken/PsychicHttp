#ifndef PsychicMiddlewareChain_h
#define PsychicMiddlewareChain_h

#include "PsychicCore.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"
#include "PsychicMiddleware.h"

class PsychicMiddleware;

/*
 * PsychicMiddlewareChain - handle tracking and executing our chain of middleware objects
 * */

class PsychicMiddlewareChain {
  protected:
    std::list<PsychicMiddleware*> _middleware;
    std::list<PsychicMiddleware*>::iterator _iterator;
    PsychicRequest* _request;
    PsychicResponse* _response;

  public:
    PsychicMiddlewareChain();
    virtual ~PsychicMiddlewareChain();

    void add(PsychicMiddleware* middleware);
    void run(PsychicRequest *request, PsychicResponse *response);
    void next();
};

#endif