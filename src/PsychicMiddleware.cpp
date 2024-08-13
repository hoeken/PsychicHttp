#include "PsychicMiddleware.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"

PsychicMiddleware::PsychicMiddleware(PsychicRequestFilterFunction filter) :
  _name(""),
  _filter(filter),
  _middleware(nullptr)
  {
  }

PsychicMiddleware::PsychicMiddleware(PsychicMiddlewareFunction middleware) :
  _name(""),
  _filter(nullptr),
  _middleware(middleware)
  {
  }

PsychicMiddleware::~PsychicMiddleware() {}

bool PsychicMiddleware::run(PsychicRequest *request, PsychicResponse *response)
{
  if (_middleware)
    return _middleware(request, response);

  if (_filter)
    return _filter(request);

  return false;
}

void PsychicMiddleware::setName(const char *name)
{
  _name = name;
}

const String& PsychicMiddleware::getName()
{
  return _name;
}