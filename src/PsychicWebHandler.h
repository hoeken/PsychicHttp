#ifndef PsychicWebHandler_h
#define PsychicWebHandler_h

#include <Arduino.h>

/*
 * HANDLER :: One instance can be attached to any Request (done by the Server)
 * */

class PsychicWebHandler {
  protected:
    //ArRequestFilterFunction _filter;
    String _username;
    String _password;
  public:
    PsychicWebHandler():_username(""), _password(""){}
    //TODO: implement filters
    //PsychicWebHandler& setFilter(ArRequestFilterFunction fn) { _filter = fn; return *this; }
    //bool filter(AsyncWebServerRequest *request){ return _filter == NULL || _filter(request); }
    PsychicWebHandler& setAuthentication(const char *username, const char *password){  _username = String(username);_password = String(password); return *this; };
    virtual ~PsychicWebHandler(){}
    virtual bool canHandle(PsychicHttpServerRequest *request __attribute__((unused))){
      return false;
    }
    virtual void handleRequest(PsychicHttpServerRequest *request __attribute__((unused))){}
    virtual void handleUpload(PsychicHttpServerRequest *request  __attribute__((unused)), const String& filename __attribute__((unused)), size_t index __attribute__((unused)), uint8_t *data __attribute__((unused)), size_t len __attribute__((unused)), bool final  __attribute__((unused))){}
    virtual void handleBody(PsychicHttpServerRequest *request __attribute__((unused)), uint8_t *data __attribute__((unused)), size_t len __attribute__((unused)), size_t index __attribute__((unused)), size_t total __attribute__((unused))){}
    virtual bool isRequestHandlerTrivial(){return true;}
};

#endif