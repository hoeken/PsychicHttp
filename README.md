# PsychicHttp

PsychicHttp is a webserver library for ESP32 + Arduino framework which uses the ESP-IDF HTTP Server library under the hood.  It is written in a similar style to the [Arduino WebServer](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer), [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer), and [ArduinoMongoose](https://github.com/jeremypoulter/ArduinoMongoose) to make writing code simple and porting from those other libraries straightforward.

# Features

* Asynchronous approach (server running in its own FreeRTOS thread)
* Handles all HTTP methods with lots of helper functions:
    * GET/POST parameters
    * get/set headers
    * get/set cookies
    * key/value SessionData
    * authentication (basic and digest mode)
* Websocket support with onConnect, onMessage, and onClose callbacks
* HTTPS / SSL support
* Static fileserving (SPIFFS, LittleFS, etc.)
* File uploads 

# TODO:

* stability fixes - go over code w/ fine toothed comb
    * why are websockets crashing?
* headers should store strings, then send them all during send()
* move send() to response
* add serveStatic
* get onClose working for websockets - need to update to Arduino core 3 / ESP-IDF v5.1
* get SSL working
* test running 2 servers at once
* copy other nice features from ESPAsyncServer and Arduino WebServer
    https://github.com/me-no-dev/ESPAsyncWebServer
    https://github.com/khoih-prog/WiFiWebServer
    https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer
* convert examples
* test in arduino IDE
