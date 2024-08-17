#include "PsychicMiddlewareChain.h"

PsychicMiddlewareChain::PsychicMiddlewareChain() {}

PsychicMiddlewareChain::~PsychicMiddlewareChain()
{
  for (auto middleware : _middleware) {
    delete middleware;
  }
  _middleware.clear();
}

void PsychicMiddlewareChain::add(PsychicMiddleware* middleware)
{
  _middleware.push_back(middleware);
}

void PsychicMiddlewareChain::add(PsychicMiddlewareFunction fn)
{
  _middleware.push_back(new PsychicMiddlewareClosure(fn));
}

bool PsychicMiddlewareChain::remove(PsychicMiddleware* middleware)
{
  _middleware.remove(middleware);
  return true;
}

esp_err_t PsychicMiddlewareChain::run(PsychicRequest* request, PsychicResponse* response, PsychicMiddlewareCallback finalizer)
{
  if (_middleware.size() == 0) {
    return finalizer(request, response);
  }

  PsychicMiddlewareCallback next;
  std::list<PsychicMiddleware*>::iterator it = _middleware.begin();

  next = [this, &next, &it, finalizer](PsychicRequest* request, PsychicResponse* response) {
    if (it != _middleware.end()) {
      return (*it++)->run(next, request, response);
    } else {
      return finalizer(request, response);
    }
  };

  return (*it)->run(next, request, response);
}