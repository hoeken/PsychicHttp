# PsychicHttp - HTTP on your ESP 🧙🔮

PsychicHttp is a webserver library for ESP32 + Arduino framework which uses the [ESP-IDF HTTP Server](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html) library under the hood.  It is written in a similar style to the [Arduino WebServer](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer), [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer), and [ArduinoMongoose](https://github.com/jeremypoulter/ArduinoMongoose) libraries to make writing code simple and porting from those other libraries straightforward.

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

# Differences from ESPAsyncWebserver

* No templating system
* No url rewriting
* No Async Event Source
* No AP/STA filter

# v1.1: Event Source Todo
    * map out the request flow
    * re-implement the Handler functionality
    * Convert all calls to AsyncClient to new PsychicConnection class
    * determine which callbacks we can support

# Roadmap / Longterm TODO

* investigate websocket performance gap
* multipart file uploads
* support for esp-idf framework
* support for arduino 3.0 framework
* Enable worker based multithreading with esp-idf v5.x
* 100-continue support
* 'borrow' other nice features from ESPAsyncServer
     
If anyone wants to take a crack at implementing any of the above features I am more than happy to accept pull requests.

# Performance

In order to really see the differences between libraries, I created some basic benchmark firmwares for PsychicHttp, ESPAsyncWebserver, and ArduinoMongoose.  I then ran the loadtest-http.sh and loadtest-websocket.sh scripts against each firmware to get some real numbers on the performance of each server library.  All of the code and results are available in the /benchmark folder.  If you want to see the collated data and graphs, there is a [LibreOffice spreadsheet](/benchmark/comparison.ods).

![Performance graph](/benchmark/performance.png)
![Latency graph](/benchmark/latency.png)

## HTTPS / SSL

Yes, PsychicHttp supports SSL out of the box, but there are a few caveats:

* Due to memory limitations, it can only handle 2 connections at a time. Each SSL connection takes about 45k ram, and a blank PsychicHttp sketch has about 150k ram free.
* Speed and latency are still pretty good (see graph above) but the SSH handshake seems to take 1500ms.  With websockets or browser its not an issue since the connection is kept alive, but if you are loading requests in another way it will be a bit slow
* Unless you want to expose your ESP to the internet, you are limited to self signed keys and the annoying browser security warnings that come with them.

To generate your own self signed certificate, you can use the command below:

```
openssl req -x509 -newkey rsa:4096 -nodes -keyout server.key -out server.crt -sha256 -days 365
```

## Analysis

The results clearly show some of the reasons for writing PsychicHttp: ESPAsyncWebserver crashes under heavy load on each test, across the board in a 60s test.  That means in normal usage, you're just rolling the dice with how long it will go until it crashes.  Every other number is moot, IMHO.

ArduinoMongoose doesn't crash under heavy load, but it does bog down with extremely high latency (15s) for web requests and appears to not even respond at the highest loadings as the loadtest script crashes instead.  The code itself doesnt crash, so bonus points there.  After the high load, it does go back to serving normally.  One area ArduinoMongoose does shine, is in websockets where its performance is almost 2x the performance of PsychicHttp.  Both in requests per second and latency.  Clearly an area of improvement for PsychicHttp.

PsychicHttp has good performance across the board.  No crashes and continously responds during each test.  It is a clear winner in requests per second when serving files from memory, dynamic JSON, and has consistent performance when serving files from LittleFS. The only real downside is the lower performance of the websockets with a single connection handling 38rps, and maxing out at 120rps across multiple connections.

## Takeaways

With all due respect to @me-no-dev who has done some amazing work in the open source community, I cannot recommend anyone use the ESPAsyncWebserver for anything other than simple projects that don't need to be reliable.  Even then, PsychicHttp has taken the arcane api of the ESP-IDF web server library and made it nice and friendly to use with a very similar API to ESPAsyncWebserver.  Also, ESPAsyncWebserver is more or less abandoned, with 150 open issues, 77 pending pull requests, and the last commit in over 2 years.

ArduinoMongoose is a good alternative, although the latency issues when it gets fully loaded can be very annoying. I believe it is also cross platform to other microcontrollers as well, but I haven't tested that. The other issue here is that it is based on an old version of a modified Mongoose library that will be difficult to update as it is a major revision behind and several security updates behind as well.  Big thanks to @jeremypoulter though as PsychicHttp is a fork of ArduinoMongoose so it's built on strong bones.
