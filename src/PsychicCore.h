#ifndef PsychicCore_h
#define PsychicCore_h

#define PH_TAG "http"

#define MAX_COOKIE_SIZE 256
#define FILE_CHUNK_SIZE 4*1024
#define MAX_UPLOAD_SIZE (200*1024) // 200 KB
#define MAX_UPLOAD_SIZE_STR "200KB"

#ifdef ARDUINO
  #include <Arduino.h>
  //#include <ArduinoTrace.h>
#endif

#include <esp_https_server.h>
#include <map>
#include <list>
#include <libb64/cencode.h>
#include "esp_random.h"
#include "MD5Builder.h"
#include <UrlEncode.h>
#include "FS.h"
#include "StringArray.h"

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
class PsychicHttpWebSocketRequest;

//callback definitions
typedef std::function<esp_err_t(PsychicHttpServer *server, int sockfd)> PsychicHttpConnectionHandler;
typedef std::function<esp_err_t(PsychicHttpServerRequest *request)> PsychicHttpRequestHandler;
typedef std::function<esp_err_t(PsychicHttpServerRequest *request, const String& filename, uint64_t index, uint8_t *data, size_t len)> PsychicHttpBasicUploadHandler;
typedef std::function<esp_err_t(PsychicHttpServerRequest *request, const String& filename, uint64_t index, uint8_t *data, size_t len)> PsychicHttpMultipartUploadHandler;
typedef std::function<esp_err_t(PsychicHttpWebSocketRequest *connection)> PsychicHttpWebSocketRequestHandler;
typedef std::function<esp_err_t(PsychicHttpWebSocketRequest *connection, httpd_ws_frame *frame)> PsychicHttpWebSocketFrameHandler;

#endif //PsychicCore_h