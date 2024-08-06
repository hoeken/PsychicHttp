#pragma once

#include "PsychicHttpServer.h"
#include "PsychicRequest.h"
#include "PsychicWebHandler.h"

#include <esp_err.h>

using AsyncWebServer = PsychicHttpServer;
using AsyncCallbackWebHandler = PsychicEndpoint;
using AsyncWebServerRequest = PsychicRequest;
using AsyncWebServerResponse = PsychicResponse;
using AsyncJsonResponse = PsychicJsonResponse;

#define PSYCHIC_OK  ESP_OK
#define PSYCHIC_REQ request,
