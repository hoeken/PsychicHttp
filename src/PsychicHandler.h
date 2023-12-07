#ifndef PsychicHandler_h
#define PsychicHandler_h

#include "PsychicCore.h"
#include "PsychicRequest.h"

/*
* HANDLER :: Can be attached to any endpoint or as a generic request handler.
*/

class PsychicHandler {
  protected:    
    PsychicRequestFilterFunction _filter;
    PsychicClientCallback _onopen;
    PsychicClientCallback _onclose;

    String _username;
    String _password;
    HTTPAuthMethod _method;
    String _realm;
    String _authFailMsg;

  public:
    PsychicHandler();
    ~PsychicHandler();

    PsychicHandler* onOpen(PsychicClientCallback fn);
    PsychicHandler* onClose(PsychicClientCallback fn);

    PsychicHandler* setFilter(PsychicRequestFilterFunction fn);
    bool filter(PsychicRequest *request);

    PsychicHandler* setAuthentication(const char *username, const char *password, HTTPAuthMethod method = BASIC_AUTH, const char *realm = "", const char *authFailMsg = "");
    bool needsAuthentication(PsychicRequest *request);
    esp_err_t authenticate(PsychicRequest *request);

    virtual void closeCallback(PsychicClient *client);
    virtual bool isWebSocket();

    //derived classes must implement these functions
    virtual bool canHandle(PsychicRequest *request) { return true; };
    virtual esp_err_t handleRequest(PsychicRequest *request) = 0;
};

#endif