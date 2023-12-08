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

#ifdef PSY_ENABLE_SSL
  #include <esp_https_server.h>
#else
  #include <esp_http_server.h>
#endif

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

enum HTTPAuthMethod { BASIC_AUTH, DIGEST_AUTH };

String urlDecode(const char* encoded);

class PsychicHttpServer;
class PsychicRequest;
class PsychicWebSocketRequest;
class PsychicClient;

//callback definitions
typedef std::function<void(PsychicClient *client)> PsychicClientCallback;
typedef std::function<esp_err_t(PsychicRequest *request)> PsychicHttpRequestCallback;

//filter function definition
typedef std::function<bool(PsychicRequest *request)> PsychicRequestFilterFunction;

/*
 * PARAMETER :: Chainable object to hold GET/POST and FILE parameters
 * */

class PsychicWebParameter {
  private:
    String _name;
    String _value;
    size_t _size;
    bool _isForm;
    bool _isFile;

  public:
    PsychicWebParameter(const String& name, const String& value, bool form=false, bool file=false, size_t size=0): _name(name), _value(value), _size(size), _isForm(form), _isFile(file){}
    const String& name() const { return _name; }
    const String& value() const { return _value; }
    size_t size() const { return _size; }
    bool isPost() const { return _isForm; }
    bool isFile() const { return _isFile; }
};


#endif //PsychicCore_h