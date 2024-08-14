#include "PsychicMiddleware.h"
#include "PsychicMiddlewareChain.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"

PsychicMiddleware::PsychicMiddleware(PsychicMiddlewareFunction callback) :
  _callback(callback)
  {
  }

PsychicMiddleware::~PsychicMiddleware() {}

void PsychicMiddleware::run(PsychicMiddlewareChain *chain, PsychicRequest *request, PsychicResponse *response)
{
  if (_callback)
    _callback(chain, request, response);
}