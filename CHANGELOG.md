# v1.2

* Add TemplatePrinter from https://github.com/Chris--A/PsychicHttp/tree/templatePrint
* Support using as ESP IDF component
* Optional using https server in ESP IDF
* Fix memory leak when send headers in PsychicResponse
* Add ESP IDF example + CI script

Thanks to @Chris--A and @dzungpv for your contributions.

# v1.1

* Changed the internal structure to support request handlers on endpoints and generic requests that do not match an endpoint
    * websockets, uploads, etc should now create an appropriate handler and attach to an endpoint with the server.on() syntax
* Added PsychicClient to abstract away some of the internals of ESP-IDF sockets + add convenience
    * onOpen and onClose callbacks have changed as a result
* Added support for EventSource / SSE
* Added support for multipart file uploads
* changed getParam() to return a PsychicWebParameter in line with ESPAsyncWebserver
* Renamed various classes / files:
    * PsychicHttpFileResponse -> PsychicFileResponse
    * PsychicHttpServerEndpoint -> PsychicEndpoint
    * PsychicHttpServerRequest -> PsychicRequest
    * PsychicHttpServerResponse -> PsychicResponse
    * PsychicHttpWebsocket.h -> PsychicWebSocket.h
    * Websocket => WebSocket
* Quite a few bugfixes from the community. Thank you @glennsky, @gb88, @KastanEr, @kstam, and @zekageri