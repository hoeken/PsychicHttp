#include "PsychicMiddleware.h"
#include "PsychicMiddlewareChain.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"

PsychicMiddlewareClosure::PsychicMiddlewareClosure(PsychicMiddlewareFunction fn) : _fn(fn)
{
  assert(_fn);
}
esp_err_t PsychicMiddlewareClosure::run(PsychicRequest* request, PsychicResponse* response, PsychicMiddlewareCallback next)
{
  return _fn(request, response, next);
}