#ifndef MongooseHttp_h
#define MongooseHttp_h

#include "MongooseCore.h"

#ifndef ARDUINO_MONGOOSE_PARAM_BUFFER_LENGTH
#define ARDUINO_MONGOOSE_PARAM_BUFFER_LENGTH 128
#endif

#ifndef ARDUINO_MONGOOSE_DEFAULT_STREAM_BUFFER
#define ARDUINO_MONGOOSE_DEFAULT_STREAM_BUFFER 128
#endif

#ifndef ARDUINO_MONGOOSE_SEND_BUFFER_SIZE
#define ARDUINO_MONGOOSE_SEND_BUFFER_SIZE 256
#endif

#ifndef _ESPAsyncWebServer_H_
  typedef enum {
    HTTP_GET     = 0b00000001,
    HTTP_POST    = 0b00000010,
    HTTP_DELETE  = 0b00000100,
    HTTP_PUT     = 0b00001000,
    HTTP_PATCH   = 0b00010000,
    HTTP_HEAD    = 0b00100000,
    HTTP_OPTIONS = 0b01000000,
    HTTP_ANY     = 0b01111111,
  } HttpRequestMethod;
#else
  typedef WebRequestMethod HttpRequestMethod;
#endif

typedef uint8_t HttpRequestMethodComposite;

#endif /* _MongooseHttp_H_ */