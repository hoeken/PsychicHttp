#include "PsychicMiddleware.h"
#include "PsychicMiddlewareChain.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"

PsychicMiddlewareClosure::PsychicMiddlewareClosure(PsychicMiddlewareFunction fn) : _fn(fn)
{
  assert(_fn);
}
esp_err_t PsychicMiddlewareClosure::run(PsychicMiddlewareCallback next, PsychicRequest* request, PsychicResponse* response)
{
  return _fn(next, request, response);
}