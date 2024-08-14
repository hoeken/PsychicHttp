#ifndef PsychicMiddleware_h
#define PsychicMiddleware_h

#include "PsychicCore.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"
#include "PsychicMiddlewareChain.h"

/*
 * PsychicMiddleware :: fancy callback wrapper for handling requests and responses.
 * */

class PsychicMiddleware {
  public:
    //void *_context;
    String _name;
    PsychicMiddlewareFunction _callback;

    PsychicMiddleware(PsychicMiddlewareFunction middleware);
    virtual ~PsychicMiddleware();

    void run(PsychicMiddlewareChain *chain, PsychicRequest *request, PsychicResponse *response);
};

#endif