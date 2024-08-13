# Middleware Update Plan

* new PsychicMiddleware class
  * name field for identification / debug
  * old callback pointer
  * old callback format: bool (*request)
    * wrap old callback in a function that returns CONTINUE or STOP on true/false?
  * new callback pointer
  * new callback: enum (*request, *response)
  * flow control enum for return: STOP, CONTINUE???

# PsychicHandler

* create addMiddleware()
* create removeMiddleware(name)
* create runMiddleware()
* convert handler.setFilter() to call addMiddleware with a wrapper
* depreciate handler.filter(), change to runMiddleware()
* what about canHandle()?
  * depreciate and convert to middleware that gets added internally on construction.
  * remove canHandle() calls
* handleRequest() -> handleRequest(*request, *response);
  * depreciate and add new style (*request, *response) call
* _filters -> _middleware

# PsychicEndpoint

* depreciate endpoint.setFilter(), change to addMiddleware()
* convert setAuthentication() to add AuthMiddleware instead.
* create addMiddleware()
  * pass to PsychicHandler
* create runMiddleware()
  * pass to PsychicHandler

## PsychicHttpServer

* convert server.setFilter()
  * change to addMiddleware() with wrapper
* create additional calls to server.on() with new callback format
* create addMiddleware()
* create runMiddleware()
* create removeMiddleware(name)
* _filters -> _middleware

# PsychicRequest

* add _response pointer to PsychicRequest
* request->beginReply() should return existing _response pointer
* depreciate authenticate() -> middleware function
* requestAuthentication() -> should add middleware?

# PsychicResponse

* add new constructor request(*response)
  * refactor existing custom Response objects to Delegation style
    * store root response pointer internally
    * pass on 'standard' calls to root object
      * code, contentType, headers, cookies, content, send, etc.
  * existing constructors that pass in *request pointer can instead call constructor with new *request->_response style