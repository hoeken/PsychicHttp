# Middleware Update Plan

* add PsychicMiddleware class
  * flow control enum for return
  * old callback pointer
  * old callback format: bool (*request)
    * wrap old callback in a function that returns CONTINUE or STOP on true/false?
  * new callback pointer
  * new callback: enum (*request, *response)


## PsychicHttpServer

* depreciate server.setFilter(), change to addMiddleware()
* add additional calls to server.on() with new callback format
* add runMiddleware()

# PsychicHandler

* depreciate handler.setFilter(), change to addMiddleware()
* depreciate handler.filter(), change to runMiddleware()
* what about canHandle()?  keep that as-is?
* what about handleRequest()? keep that as-is?
  * possibly depreciate and add new style (*request, *response) call

# PsychicEndpoint

* depreciate endpoint.setFilter(), change to addMiddleware()
* convert setAuthentication() to add AuthMiddleware instead.

# PsychicRequest

* add _response pointer to PsychicRequest
* request->beginReply() should return existing _response pointer

# PsychicResposne

* add new contructor request(*response)
  * refactor existing custom Response objects to Delegation style
    * store root response pointer internally
    * pass on 'standard' calls to root object
      * code, contentType, headers, cookies, content, send, etc.
  * existing calls that pass in *request pointer can instead call constructor with new *request->_response style