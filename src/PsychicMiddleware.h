#ifndef PsychicMiddleware_h
#define PsychicMiddleware_h

#include "PsychicCore.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"

/*
 * REWRITE :: One instance can be handle any Request (done by the Server)
 * */

class PsychicMiddleware {
  protected:
    String _name;
    PsychicRequestFilterFunction _filter;
    PsychicMiddlewareFunction _middleware;

  public:
    PsychicMiddleware(PsychicRequestFilterFunction filter);
    PsychicMiddleware(PsychicMiddlewareFunction middleware);
    virtual ~PsychicMiddleware();

    bool run(PsychicRequest *request, PsychicResponse *response);

    void setName(const char *name);
    const String& getName();
};

#endif