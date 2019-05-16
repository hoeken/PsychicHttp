#ifdef _MSC_VER
#error _MSC_VER
#endif

#ifdef TEST_SIMPLEST_WEB_SERVER_NATIVE
#include "../examples/simplest_web_server_native/src/simplest_web_server.c"
#endif
#ifdef TEST_SIMPLEST_WEB_SERVER_ESP
#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#error Platform not supported
#endif
#include "../examples/simplest_web_server_esp/src/simplest_web_server.c"
#endif
