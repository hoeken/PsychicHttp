#include "PsychicMiddlewareChain.h"

PsychicMiddlewareChain::PsychicMiddlewareChain() :
  _request(nullptr),
  _response(nullptr)
  {
  }

//TODO: memory management
PsychicMiddlewareChain::~PsychicMiddlewareChain() {}

void PsychicMiddlewareChain::add(PsychicMiddleware *middleware)
{
  _middleware.push_back(middleware);
}

bool PsychicMiddlewareChain::run(PsychicRequest *request, PsychicResponse *response)
{
  //save our in/out objects
  _request = request;
  _response = response;
  _finished = false;

  //start at the beginning.
  auto _iterator = _middleware.begin();

  //is it valid?
  if (_iterator != _middleware.end())
  {
    PsychicMiddleware* mw = *_iterator;
    mw->run(this, _request, _response);
  }

  //let them know if we finished or not.
  return _finished;
}

void PsychicMiddlewareChain::next()
{
  //get the next one!
  _iterator++;

  //is there a next one?
  if (_iterator != _middleware.end())
  {
    PsychicMiddleware* mw = *_iterator;
    mw->run(this, _request, _response);
  }
  //nope, we're done.
  else
    _finished = true;
}