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

    String _username;
    String _password;
    HTTPAuthMethod _method;
    String _realm;
    String _authFailMsg;

    std::list<PsychicClient*> _clients;
    PsychicClientCallback _onOpen;
    PsychicClientCallback _onClose;

  public:
    PsychicHandler();
    ~PsychicHandler();

    PsychicHandler* setFilter(PsychicRequestFilterFunction fn);
    bool filter(PsychicRequest *request);

    PsychicHandler* setAuthentication(const char *username, const char *password, HTTPAuthMethod method = BASIC_AUTH, const char *realm = "", const char *authFailMsg = "");
    bool needsAuthentication(PsychicRequest *request);
    esp_err_t authenticate(PsychicRequest *request);

    void onOpen(PsychicClientCallback handler);
    void onClose(PsychicClientCallback handler);

    virtual bool isWebSocket() { return false; };

    virtual void openCallback(PsychicClient *client);
    virtual void closeCallback(PsychicClient *client);

    PsychicClient * checkForNewClient(PsychicClient *client);
    void checkForClosedClient(PsychicClient *client);

    void addClient(PsychicClient *client);
    void removeClient(PsychicClient *client);
    PsychicClient * getClient(PsychicClient *client);
    bool hasClient(PsychicClient *client);
    int count() { return _clients.size(); };
    const std::list<PsychicClient*>& getClientList();

    //derived classes must implement these functions
    virtual bool canHandle(PsychicRequest *request) { return true; };
    virtual esp_err_t handleRequest(PsychicRequest *request) = 0;
};

#endif