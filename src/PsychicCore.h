#ifndef PsychicCore_h
#define PsychicCore_h

#define PH_TAG "psychic"

#ifndef MAX_COOKIE_SIZE
  #define MAX_COOKIE_SIZE 512
#endif

#ifndef FILE_CHUNK_SIZE
  #define FILE_CHUNK_SIZE 8*1024
#endif

#ifndef MAX_UPLOAD_SIZE
  #define MAX_UPLOAD_SIZE (2048*1024) // 2MB
#endif

#ifndef MAX_REQUEST_BODY_SIZE
  #define MAX_REQUEST_BODY_SIZE (16*1024) //16K
#endif

#ifdef ARDUINO
  #include <Arduino.h>
  #include <ArduinoTrace.h>
#endif

#include <esp_http_server.h>
#include <map>
#include <list>
#include <libb64/cencode.h>
#include "esp_random.h"
#include "MD5Builder.h"
#include <UrlEncode.h>
#include "FS.h"

enum HTTPAuthMethod { BASIC_AUTH, DIGEST_AUTH };

String urlDecode(const char* encoded);

class PsychicHttpServer;
class PsychicRequest;
class PsychicWebSocketRequest;
class PsychicClient;

//filter function definition
typedef std::function<bool(PsychicRequest *request)> PsychicRequestFilterFunction;

//client connect callback
typedef std::function<void(PsychicClient *client)> PsychicClientCallback;

#endif //PsychicCore_h