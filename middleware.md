# Middleware Update Plan

- [x] new PsychicMiddleware class
  - [x] name field for identification / debug
  - [x] old callback pointer
  - [x] old callback format: bool (*request)
  - [x] new callback pointer
  - [x] new callback: enum (*request, *response)

# PsychicHandler

- [x] _filters -> _middleware
- [x] create addMiddleware()
- [ ] create removeMiddleware(name)
- [ ] create removeMiddleware(pointer)
- [x] create runMiddleware()
- [x] convert handler.setFilter() to call addMiddleware with a wrapper
- [ ] depreciate handler.filter() - preprocessor? how?
- [ ] what about canHandle()?
  * we should probably keep this - middleware is not context aware, and there are probably situations where we want to check handler variables
- [x] handleRequest() -> handleRequest(*request, *response);
  - [ ] depreciate old style
  - [x] add new style (*request, *response) call
- [x] destructor memory management
- [ ] convert auth to middleware, remove handler->needsAuthentication calls

# PsychicEndpoint

- [x] convert endpoint.setFilter(), change to addMiddleware()
- [ ] convert setAuthentication() to add AuthMiddleware instead.
- [x] create addMiddleware()
  - [x] pass to PsychicHandler
- [x] create runMiddleware()
  - [x] pass to PsychicHandler

## PsychicHttpServer

- [x] _filters -> _middleware
- [x] convert server.setFilter()
  - [x] change to addMiddleware() with wrapper
- [ ] create additional calls to server.on() with new callback format
- [x] create addMiddleware()
- [x] create runMiddleware()
- [ ] create removeMiddleware(name)

# PsychicRequest

- [x] add _response pointer to PsychicRequest
- [x] request->beginReply() should return existing _response pointer
- [ ] requestAuthentication() -> should add middleware?

# PsychicResponse

- [x] make class final
- [x] refactor existing custom Response objects to Delegation style
  - [x] store root response pointer internally
  - [x] pass on all calls to root object