#pragma once

#include "PsychicCore.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"

class PsychicMiddleware
{
  public:
    virtual ~PsychicMiddleware() {}
    virtual esp_err_t run(PsychicMiddlewareCallback next, PsychicRequest* request, PsychicResponse* response) = 0;
};

class PsychicMiddlewareBase : public PsychicMiddleware
{
  public:
    esp_err_t run(PsychicMiddlewareCallback next, PsychicRequest* request, PsychicResponse* response) override { return next(request, response); }
};

class PsychicMiddlewareClosure : public PsychicMiddlewareBase
{
  protected:
    PsychicMiddlewareFunction _fn;

  public:
    PsychicMiddlewareClosure(PsychicMiddlewareFunction fn) : _fn(fn) {}
    esp_err_t run(PsychicMiddlewareCallback next, PsychicRequest* request, PsychicResponse* response) override { return _fn(next, request, response); }
};

class PsychicMiddlewareChain
{
  protected:
    std::list<PsychicMiddleware*> _middleware;

  public:
    ~PsychicMiddlewareChain()
    {
      for (auto middleware : _middleware) {
        delete middleware;
      }
    }

    PsychicMiddlewareChain* add(PsychicMiddleware* middleware)
    {
      _middleware.push_back(middleware);
      return this;
    }

    PsychicMiddlewareChain* add(PsychicMiddlewareFunction fn)
    {
      _middleware.push_back(new PsychicMiddlewareClosure(fn));
      return this;
    }

    bool remove(PsychicMiddleware* middleware)
    {
      _middleware.remove(middleware);
      return true;
    }

    esp_err_t run(PsychicRequest* request, PsychicResponse* response, PsychicMiddlewareCallback finalizer)
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
};
