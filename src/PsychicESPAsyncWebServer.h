#pragma once

#include "PsychicEndpoint.h"
#include "PsychicEventSource.h"
#include "PsychicFileResponse.h"
#include "PsychicHandler.h"
#include "PsychicHttpServer.h"
#include "PsychicJson.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"
#include "PsychicStaticFileHandler.h"
#include "PsychicStreamResponse.h"
#include "PsychicUploadHandler.h"
#include "PsychicVersion.h"
#include "PsychicWebSocket.h"
#include <esp_err.h>
#include <http_status.h>

using ArRequestHandlerFunction = PsychicHttpRequestCallback;
using AsyncCallbackWebHandler = PsychicEndpoint;
using AsyncJsonResponse = PsychicJsonResponse;
using AsyncWebHandler = PsychicHandler;
using AsyncWebRewrite = PsychicRewrite;
using AsyncWebServerRequest = PsychicRequest;
using AsyncWebServerResponse = PsychicResponse;
using WebRequestMethodComposite = int;

#define PSYCHIC_OK  ESP_OK
#define PSYCHIC_REQ request,

class AsyncWebServer
{
  private:
    PsychicHttpServer* _server;

  public:
    AsyncWebServer(uint16_t port = 80) : _server(new PsychicHttpServer(port)) {};
    ~AsyncWebServer() { delete _server; };

    void begin() { _server->begin(); };
    void end() { _server->end(); };

    void reset() { _server->reset(); };

    void onNotFound(ArRequestHandlerFunction fn) { _server->onNotFound(fn); };

    AsyncCallbackWebHandler& on(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest) { return *_server->on(uri, method, onRequest); };

    bool removeHandler(AsyncCallbackWebHandler* handler) { return _server->removeEndpoint(handler); };

    AsyncWebHandler& addHandler(AsyncWebHandler* handler) { return *_server->addHandler(handler); };
    bool removeHandler(AsyncWebHandler* handler)
    {
      _server->removeHandler(handler);
      return true;
    };

    AsyncWebRewrite& addRewrite(AsyncWebRewrite* rewrite) { return *_server->addRewrite(rewrite); };
    AsyncWebRewrite& rewrite(const char* from, const char* to) { return *_server->rewrite(from, to); };

    // AsyncCallbackWebHandler& on(const char* uri, ArRequestHandlerFunction onRequest);
    // AsyncCallbackWebHandler& on(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload);
    // AsyncCallbackWebHandler& on(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload, ArBodyHandlerFunction onBody);

    // AsyncStaticWebHandler& serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_control = NULL);

    // void onFileUpload(ArUploadHandlerFunction fn);
    // void onRequestBody(ArBodyHandlerFunction fn);
    // void reset();
};
