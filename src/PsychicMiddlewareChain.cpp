#include "PsychicMiddlewareChain.h"

PsychicMiddlewareChain::PsychicMiddlewareChain() {}

PsychicMiddlewareChain::~PsychicMiddlewareChain()
{
  for (auto middleware : _middleware) {
    if (middleware->_managed)
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
  PsychicMiddlewareClosure* closure = new PsychicMiddlewareClosure(fn);
  closure->_managed = true;
  _middleware.push_back(closure);
}

void PsychicMiddlewareChain::remove(PsychicMiddleware* middleware)
{
  _middleware.remove(middleware);
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
      PsychicMiddleware* m = *it;
      it++;
      return m->run(request, response, next);
    } else {
      return finalizer(request, response);
    }
  };

  return next(request, response);
}