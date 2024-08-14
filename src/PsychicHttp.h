#ifndef PsychicHttp_h
#define PsychicHttp_h

// #define ENABLE_ASYNC // This is something added in ESP-IDF 5.1.x where each request can be handled in its own thread

#include <http_status.h>

#include "PsychicVersion.h"
#include "PsychicEndpoint.h"
#include "PsychicEventSource.h"
#include "PsychicFileResponse.h"
#include "PsychicHandler.h"
#include "PsychicHttpServer.h"
#include "PsychicJson.h"
#include "PsychicMiddleware.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"
#include "PsychicResponseDelegate.h"
#include "PsychicStaticFileHandler.h"
#include "PsychicStreamResponse.h"
#include "PsychicUploadHandler.h"
#include "PsychicWebSocket.h"

#include "Middlewares.h"

#ifdef ENABLE_ASYNC
  #include "async_worker.h"
#endif

#endif /* PsychicHttp_h */