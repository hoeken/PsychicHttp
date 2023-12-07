#ifndef PsychicCore_h
#define PsychicCore_h

#define PH_TAG "psychic"

#ifndef MAX_COOKIE_SIZE
  #define MAX_COOKIE_SIZE 256
#endif

#ifndef FILE_CHUNK_SIZE
  #define FILE_CHUNK_SIZE 4*1024
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

#include <esp_https_server.h>
#include <map>
#include <list>
#include <libb64/cencode.h>
#include "esp_random.h"
#include "MD5Builder.h"
#include <UrlEncode.h>
#include "FS.h"

#if !CONFIG_HTTPD_WS_SUPPORT
  #error This library cannot be used unless HTTPD_WS_SUPPORT is enabled in esp-http-server component configuration
#endif

typedef std::map<String, String> SessionData;

struct HTTPHeader {
  char * field;
  char * value;
};

enum Disposition { NONE, INLINE, ATTACHMENT, FORM_DATA};

struct ContentDisposition {
  Disposition disposition;
  String filename;
  String name;
};

//TODO: not quite used yet. for content-disposition
// struct MultipartContent {
//   char * content_type;
//   char * name;
//   char * filename;
// };

enum HTTPAuthMethod { BASIC_AUTH, DIGEST_AUTH };

String urlDecode(const char* encoded);

class PsychicHttpServer;
class PsychicHttpServerRequest;
class PsychicWebSocketRequest;
class PsychicClient;

//callback definitions
typedef std::function<esp_err_t(PsychicClient *client)> PsychicClientCallback;
typedef std::function<esp_err_t(PsychicHttpServerRequest *request)> PsychicHttpRequestHandler;

//filter function definition
typedef std::function<bool(PsychicHttpServerRequest *request)> PsychicRequestFilterFunction;

#endif //PsychicCore_h