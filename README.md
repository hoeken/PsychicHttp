# PsychicHttp - HTTP on your ESP 🧙🔮

PsychicHttp is a webserver library for ESP32 + Arduino framework which uses the ESP-IDF HTTP Server library under the hood.  It is written in a similar style to the [Arduino WebServer](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer), [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer), and [ArduinoMongoose](https://github.com/jeremypoulter/ArduinoMongoose) to make writing code simple and porting from those other libraries straightforward.

# Features

* Asynchronous approach (server running in its own FreeRTOS thread)
* Handles all HTTP methods with lots of helper functions:
    * GET/POST parameters
    * get/set headers
    * get/set cookies
    * basic key/value session data storage
    * authentication (basic and digest mode)
* Websocket support with onConnect, onFrame, and onClose callbacks
* HTTPS / SSL support
* Static fileserving (SPIFFS, LittleFS, etc.)
* Chunked response
* File uploads (Basic, no multipart... yet)
* No templating system
* No url rewriting
* No Async Event Source
* No AP/STA filter

# v1.0 Release TODO:

* fix the authdigest issues
* get library in arduino library manager
* get library on platformio
* make a release script
* update docs / readme
* post on forums

# Roadmap:

* investigate websocket performance gap
* multipart file uploads
* support for esp-idf framework
* support for arduino 3.0 framework
* Enable worker based multithreading with esp-idf v5.x
  * #if ESP_ARDUINO_VERSION_MAJOR >= 3
* 100-continue support
* 'borrow' other nice features from ESPAsyncServer and Arduino WebServer
    * https://github.com/me-no-dev/ESPAsyncWebServer
      * AP/STA filter

# Performance

Here are the results of running the ./loadtest.sh script over 24 hours:

```
Target URL:          http://192.168.2.131/
Max time (s):        21600
Concurrent clients:  20
Running on cores:    2
Agent:               none

Completed requests:  833471
Total errors:        192
Total time:          21600.076 s
Mean latency:        517.5 ms
Effective rps:       39

Percentage of requests served within a certain time
  50%      129 ms
  90%      1126 ms
  95%      1225 ms
  99%      7278 ms
 100%      131208 ms (longest request)

   -1:   192 errors

Target URL:          http://192.168.2.131/api
Max time (s):        21600
Concurrent clients:  20
Running on cores:    2
Agent:               none

Completed requests:  275165
Total errors:        1183
Total time:          21600.086 s
Mean latency:        1567.3 ms
Effective rps:       13

Percentage of requests served within a certain time
  50%      197 ms
  90%      1392 ms
  95%      2188 ms
  99%      31629 ms
 100%      131206 ms (longest request)

   -1:   1183 errors

Target URL:          ws://192.168.2.131/ws
Max time (s):        21600
Concurrent clients:  20
Running on cores:    2
Agent:               none

Completed requests:  293375
Total errors:        0
Total time:          21600.217 s
Mean latency:        422.4 ms
Effective rps:       14

Percentage of requests served within a certain time
  50%      120 ms
  90%      1044 ms
  95%      1971 ms
  99%      2503 ms
 100%      6499 ms (longest request)

Target URL:          http://192.168.2.131/alien.png
Max time (s):        21600
Concurrent clients:  20
Running on cores:    2
Agent:               none

Completed requests:  203181
Total errors:        1253
Total time:          21600.016 s
Mean latency:        2122.5 ms
Effective rps:       9

Percentage of requests served within a certain time
  50%      645 ms
  90%      786 ms
  95%      3674 ms
  99%      32203 ms
 100%      131199 ms (longest request)

   -1:   1253 errors
```