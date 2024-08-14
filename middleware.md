# Middleware Update Plan

- [x] new PsychicMiddleware class
  - [x] name field for identification / debug
  - [x] old callback pointer
  - [x] old callback format: bool (*request)
  - [x] new callback pointer
  - [x] new callback: enum (*request, *response)

# PsychicHandler

- [ ] create addMiddleware()
- [ ] create removeMiddleware(name)
- [ ] create runMiddleware()
- [ ] convert handler.setFilter() to call addMiddleware with a wrapper
- [ ] depreciate handler.filter(), change to runMiddleware()
- [ ] what about canHandle()?
  - [ ] depreciate and convert to middleware that gets added internally on construction.
  - [ ] remove canHandle() calls
- [ ] handleRequest() -> handleRequest(*request, *response);
  - [ ] depreciate and add new style (*request, *response) call
- [ ] _filters -> _middleware

# PsychicEndpoint

- [ ] depreciate endpoint.setFilter(), change to addMiddleware()
- [ ] convert setAuthentication() to add AuthMiddleware instead.
- [ ] create addMiddleware()
  - [ ] pass to PsychicHandler
- [ ] create runMiddleware()
  - [ ] pass to PsychicHandler

## PsychicHttpServer

- [ ] convert server.setFilter()
  - [ ] change to addMiddleware() with wrapper
- [ ] create additional calls to server.on() with new callback format
- [ ] create addMiddleware()
- [ ] create runMiddleware()
- [ ] create removeMiddleware(name)
- [ ] _filters -> _middleware

# PsychicRequest

- [ ] add _response pointer to PsychicRequest
- [ ] request->beginReply() should return existing _response pointer
- [ ] depreciate authenticate() -> middleware function
- [ ] requestAuthentication() -> should add middleware?

# PsychicResponse

- [x] make class final
- [x] refactor existing custom Response objects to Delegation style
  - [x] store root response pointer internally
  - [x] pass on all calls to root object
