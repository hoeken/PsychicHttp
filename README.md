# PsychicHttp - HTTP on your ESP 🧙🔮

PsychicHttp is a webserver library for ESP32 + Arduino framework which uses the [ESP-IDF HTTP Server](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html) library under the hood.  It is written in a similar style to the [Arduino WebServer](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer), [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer), and [ArduinoMongoose](https://github.com/jeremypoulter/ArduinoMongoose) libraries to make writing code simple and porting from those other libraries straightforward.

# Features

* Asynchronous approach (server runs in its own FreeRTOS thread)
* Handles all HTTP methods with lots of convenience functions:
    * GET/POST parameters
    * get/set headers
    * get/set cookies
    * basic key/value session data storage
    * authentication (basic and digest mode)
* HTTPS / SSL support
* Static fileserving (SPIFFS, LittleFS, etc.)
* Chunked response serving for large files
* File uploads (Basic + Multipart)
* Websocket support with onOpen, onFrame, and onClose callbacks
* EventSource / SSE support with onOpen, and onClose callbacks
* Request filters, including Client vs AP mode (ON_STA_FILTER / ON_AP_FILTER)

## Differences from ESPAsyncWebserver

* No templating system (anyone actually use this?)
* No url rewriting (but you can use request->redirect)

# Usage

## Installation

### Platformio

[PlatformIO](http://platformio.org) is an open source ecosystem for IoT development with cross platform build system, library manager and full support for Espressif ESP8266/ESP32 development. It works on the popular host OS: Mac OS X, Windows, Linux 32/64, Linux ARM (like Raspberry Pi, BeagleBone, CubieBoard).

 Add "PsychicHttp" to project using [Project Configuration File `platformio.ini`](http://docs.platformio.org/page/projectconf.html) and [lib_deps](http://docs.platformio.org/page/projectconf/section_env_library.html#lib-deps) option:

```ini
[env:myboard]
platform = espressif...
board = ...
framework = arduino

# using the latest stable version
lib_deps = hoeken/PsychicHttp

# or using GIT Url (the latest development version)
lib_deps = https://github.com/hoeken/PsychicHttp
```

### Installation - Arduino

Open *Tools -> Manage Libraries...* and search for PsychicHttp.

# Principles of Operation

## Things to Note

* This is fully asynchronous server and as such does not run on the loop thread.
* You should not use yield or delay or any function that uses them inside the callbacks
* The server is smart enough to know when to close the connection and free resources
* You can not send more than one response to a single request

## PsychicHttp

* Listens for connections
* Wraps the incoming request into PsychicRequest
* Keeps track of clients + calls optional callbacks on client open and close
* Find the appropriate handler (if any) for a request and pass it on

## Request Life Cycle

* TCP connection is received by the server
* HTTP request is wrapped inside ```PsychicRequest``` object + TCP Connection wrapped inside PsychicConnection object.
* When the request head is received, the server goes through all ```PsychicEndpoints``` and finds one that matches the url + method.
    * ```handler->filter()``` and ```handler->canHandle()``` are called on the handler to verify the handler should process the request
    * ```handler->needsAuthentication()``` is called and sends an authorization response if required
    * ```handler->handleRequest()``` is called to actually process the HTTP request
* If the handler cannot process the request, the server will loop through any global handlers and call that handler if it passes filter(), canHandle(), and needsAuthentication()
* If no global handlers are called, the server.defaultEndpoint handler will be called
* Each handler is responsible for processing the request and sending a response.
* When the response is sent, the client is closed and freed from the memory
    * Unless its a special handler like websockets or eventsource

### Handlers

* ```PsychicHandler``` is used for executing specific actions to particular requests
* ```PsychicHandler``` instances can be attached to any endpoint or as global handlers.
* Setting a ```Filter``` to the ```PsychicHandler``` controls when to apply the handler, decision can be based on
  request url, request host/port/target host, the request client's localIP or remoteIP.
* Two filter callbacks are provided: ```ON_AP_FILTER``` to execute the rewrite when request is made to the AP interface,
  ```ON_STA_FILTER``` to execute the rewrite when request is made to the STA interface.
* The ```canHandle``` method is used for handler specific control on whether the requests can be handled. Decision can be based on request method, request url, request host/port/target host.
* Depending on how the handler is implemented, it may provide callbacks for adding your own custom processing code to the handler.
* Global ```Handlers``` are evaluated in the order they are attached to the server. The ```canHandle``` is called only
  if the ```Filter``` that was set to the ```Handler``` return true.
* The first global ```Handler``` that can handle the request is selected, no further ```Filter``` and ```canHandle``` are called.

### Responses and how do they work

* The ```PsychicResponse``` objects are used to send the response data back to the client.
* Typically the response should be fully generated and sent from the callback.
* It may be possible to generate the response outside the callback, but it will be difficult.

# Usage

## Create the Server

Here is an example of the typical parts of a server setup:

```
#include <PsychicHttp.h>
PsychicHttpServer server;

void setup()
{
    //optionsl low level setup server config stuff here.  server.config is an ESP-IDF httpd_config struct
    //see: https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-reference/protocols/esp_http_server.html#_CPPv412httpd_config
    server.config.max_uri_handlers = 20; //maximum number of uri handlers (.on() calls)

    //connect to wifi

    //start the server listening on port 80 (standard HTTP port)
    server.listen(80);

    //call server methods to attach endpoints and handlers
    server.on(...);
    server.serveStatic(...);
    server.attachHandler(...);
}
```

## Add Handlers

### Basic Requests

### Uploads

### Static File Serving

### Websockets

### EventSource / SSE

### HTTPS / SSL

To generate your own self signed certificate, you can use the command below:

```
openssl req -x509 -newkey rsa:4096 -nodes -keyout server.key -out server.crt -sha256 -days 365
```

# Performance

In order to really see the differences between libraries, I created some basic benchmark firmwares for PsychicHttp, ESPAsyncWebserver, and ArduinoMongoose.  I then ran the loadtest-http.sh and loadtest-websocket.sh scripts against each firmware to get some real numbers on the performance of each server library.  All of the code and results are available in the /benchmark folder.  If you want to see the collated data and graphs, there is a [LibreOffice spreadsheet](/benchmark/comparison.ods).

![Performance graph](/benchmark/performance.png)
![Latency graph](/benchmark/latency.png)

## HTTPS / SSL

Yes, PsychicHttp supports SSL out of the box, but there are a few caveats:

* Due to memory limitations, it can only handle 2 connections at a time. Each SSL connection takes about 45k ram, and a blank PsychicHttp sketch has about 150k ram free.
* Speed and latency are still pretty good (see graph above) but the SSH handshake seems to take 1500ms.  With websockets or browser its not an issue since the connection is kept alive, but if you are loading requests in another way it will be a bit slow
* Unless you want to expose your ESP to the internet, you are limited to self signed keys and the annoying browser security warnings that come with them.

## Analysis

The results clearly show some of the reasons for writing PsychicHttp: ESPAsyncWebserver crashes under heavy load on each test, across the board in a 60s test.  That means in normal usage, you're just rolling the dice with how long it will go until it crashes.  Every other number is moot, IMHO.

ArduinoMongoose doesn't crash under heavy load, but it does bog down with extremely high latency (15s) for web requests and appears to not even respond at the highest loadings as the loadtest script crashes instead.  The code itself doesnt crash, so bonus points there.  After the high load, it does go back to serving normally.  One area ArduinoMongoose does shine, is in websockets where its performance is almost 2x the performance of PsychicHttp.  Both in requests per second and latency.  Clearly an area of improvement for PsychicHttp.

PsychicHttp has good performance across the board.  No crashes and continously responds during each test.  It is a clear winner in requests per second when serving files from memory, dynamic JSON, and has consistent performance when serving files from LittleFS. The only real downside is the lower performance of the websockets with a single connection handling 38rps, and maxing out at 120rps across multiple connections.

## Takeaways

With all due respect to @me-no-dev who has done some amazing work in the open source community, I cannot recommend anyone use the ESPAsyncWebserver for anything other than simple projects that don't need to be reliable.  Even then, PsychicHttp has taken the arcane api of the ESP-IDF web server library and made it nice and friendly to use with a very similar API to ESPAsyncWebserver.  Also, ESPAsyncWebserver is more or less abandoned, with 150 open issues, 77 pending pull requests, and the last commit in over 2 years.

ArduinoMongoose is a good alternative, although the latency issues when it gets fully loaded can be very annoying. I believe it is also cross platform to other microcontrollers as well, but I haven't tested that. The other issue here is that it is based on an old version of a modified Mongoose library that will be difficult to update as it is a major revision behind and several security updates behind as well.  Big thanks to @jeremypoulter though as PsychicHttp is a fork of ArduinoMongoose so it's built on strong bones.

# Roadmap

## v1.1: Event Source + Handlers
    * Testing
        * inspect the code for memory leaks and other issues
        * run the client test scripts to completion
        * run loadtest benchmarks
    * Docs
        * add the flowcharts
        * write about server + options
            * on
            * serveStatic
            * onOpen/onClose
        * write about handlers
            * write about handler specific callbacks
        * write about enabling SSL
        * write about websocket/eventsource clients

## v1.2: ESPAsyncWebserver Parity
    * templating system
    * rewrite
    * regex url matching
    * What else are we missing?

## Longterm Wants

* investigate websocket performance gap
* support for esp-idf framework
* support for arduino 3.0 framework
* Enable worker based multithreading with esp-idf v5.x
* 100-continue support?
     
If anyone wants to take a crack at implementing any of the above features I am more than happy to accept pull requests.