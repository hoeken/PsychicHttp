#ifndef PsychicHttp_h
#define PsychicHttp_h

//#define ENABLE_ASYNC // This is something added in ESP-IDF 5.1.x where each request can be handled in its own thread
//#define ENABLE_EVENT_SOURCE // Support for EventSource requests/responses

#include "PsychicHttpServer.h"
#include "PsychicHttpServerRequest.h"
#include "PsychicHttpServerResponse.h"
#include "PsychicHttpFileResponse.h"
#include "PsychicHttpServerEndpoint.h"
#include "PsychicWebHandler.h"
#include "PsychicStaticFileHandler.h"
#include "PsychicHttpWebsocket.h"

#endif /* PsychicHttp_h */