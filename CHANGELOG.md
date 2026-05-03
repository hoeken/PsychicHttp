## 2.2.0

- fix: memory leaks — add PsychicEndpoint destructor to delete handler; removeEndpoint/removeHandler/removeRewrite now delete removed objects; reset() deletes middleware chain
- fix: stale endpoint state — removeEndpoint now cleans up _esp_idf_endpoints so WebSocket entries are properly unregistered across server restarts
- fix: HTTPS server now syncs max_uri_handlers and stack_size from config before starting (both were silently ignored before)
- fix: path traversal attack blocked in static file handler (#security)
- fix: redirect response code was initialized wrong, breaking redirects (#239)
- fix: correct integer comparisons in indexOf() checks, char overflow in auth buffer, and format specifiers for size_t
- fix: urlDecode bounds-checks before reading past a trailing %
- fix: regex URI matching now catches invalid patterns instead of crashing
- fix: closeCallback guards against null handler before calling checkForClosedClient
- fix: improper delegation call in PsychicJSONResponse
- feat: removed urlencode external dependency — pulled into repository as src/UrlEncode.cpp
- feat: replaced WiFi.h/ETH.h dependencies with generic esp_netif API; isConnected(), ON_STA_FILTER, and ON_AP_FILTER now work on all interface types including ESP32-P4
- feat: esp_netif compatibility — use esp_netif_next loop for older ESP-IDF versions (#235)
- feat: added warning when registering WebSocket handler after start() call (#233)
- feat: ESP-IDF v5.5 support added; v6.0 removed pending Arduino support
- fix: NTP/DHCP handling for ESP-IDF
- examples: increased stack size to fix multipart file upload issues; added start() call to HTTPS redirect server example

## 2.1.3

- fix: Added getParams() to access all parameters from issue #236

## 2.1.2

- fix: close _file before PsychicFileResponse to prevent LittleFS remove() failure

## 2.1.1

- Re-added deleted MAX function per #230

## 2.1.0 (since 2.0.0)

- send to all clients, not bail on the first one.
- Fix issue whereby H2 encoding ignores method and defaults to HTTP_GET. (#202)
- now using the stable version of pioarduino.
- V2 dev rollup: update PsychicFileResponse (set status and content type before chunked responses), fix getCookie, and add pong reply to ping. (#228, #207, #209, #222)
- Update async_worker.cpp to fix compatibility with Arduino ESP32 3.3.0. (#225)
- fixed a mistake from the pull merge.
- Moved setting content type and response code into sendHeaders(). (PR #220)
- Check if content size is 0 before sending a response. (#218)
- Fix crash with Event Source and update CI / IDF examples. (#221)
- fixed EventSource error with missing headers (content type, cache-control, keep-alive).
- fixed the CI to use the latest stable versions.
- ugh. CI so annoying.
- bump to v2.1.0.


# v2.0

I apologize for sitting on this release for so long.  Its been almost a year and life just sort of got away from me.  I'd like to get this release out and then start working through the backlog of issues.  v2.0 has been very stable for me, so it's more than time to release it.

* Huge amount of work was done to add MiddleWare and some more under the hood updates
* Modified the request handling to bring initial url matching and filtering into PsychicHttpServer itself.
    * Fixed a bug with filter() where endpoint is matched, but filter fails and it doesn't continue matching further endpoints on same uri (checks were in different codebases)
    * HTTP_ANY support
    * unlimited endpoints (no more need to manually set config.max_uri_handlers)
    * much more flexibility for future
* Endpoint Matching Updates
    * Endpoint matching functions can be set on server level (```server.setURIMatchFunction()```) or endpoint level (```endpoint.setURIMatchFunction()```)
    * Added convenience macros MATCH_SIMPLE, MATCH_WILDCARD, and MATCH_REGEX
    * Added regex matching of URIs, enable it with define PSY_ENABLE_REGEX
    * On regex matched requests, you can get match data with request->getRegexMatches()
* Ported URL rewrite functionality from ESPAsyncWS

## Changes required from v1.x to v2.0:

* add a ```server.begin()``` or ```server.start()``` after all your ```server.on()``` calls
* remove any calls to ```config.max_uri_handlers```
* if you are using a custom ```server.config.uri_match_fn``` to match uris, change it to ```server.setURIMatchFunction()```

# v1.2.1

* Fix bug with missing include preventing the HTTPS server from compiling.

# v1.2

* Added TemplatePrinter from https://github.com/Chris--A/PsychicHttp/tree/templatePrint
* Support using as ESP IDF component
* Optional using https server in ESP IDF
* Fixed bug with headers
* Add ESP IDF example + CI script
* Added Arduino Captive Portal example and OTAUpdate from @06GitHub
* HTTPS fix for ESP-IDF v5.0.2+ from @06GitHub
* lots of bugfixes from @mathieucarbou

Thanks to @Chris--A, @06GitHub, and @dzungpv for your contributions.

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