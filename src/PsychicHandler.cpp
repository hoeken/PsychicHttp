#include "PsychicHandler.h"

PsychicHandler::PsychicHandler() :
  _username(""),
  _password(""),
  _method(DIGEST_AUTH),
  _realm(""),
  _authFailMsg("")
  {}

PsychicHandler::~PsychicHandler() {}

PsychicHandler* PsychicHandler::onOpen(PsychicClientCallback fn) {
  _onopen = fn;
  return this;
}

PsychicHandler* PsychicHandler::onClose(PsychicClientCallback fn) {
  _onclose = fn;
  return this;
}

PsychicHandler* PsychicHandler::setFilter(PsychicRequestFilterFunction fn) {
  _filter = fn;
  return this;
}

bool PsychicHandler::filter(PsychicHttpServerRequest *request){
  return _filter == NULL || _filter(request);
}

PsychicHandler* PsychicHandler::setAuthentication(const char *username, const char *password, HTTPAuthMethod method, const char *realm, const char *authFailMsg) {
  _username = String(username);
  _password = String(password);
  _method = method;
  _realm = String(realm);
  _authFailMsg = String(authFailMsg);
  return this;
};

bool PsychicHandler::needsAuthentication(PsychicHttpServerRequest *request) {
  return (_username != "" && _password != "") && !request->authenticate(_username.c_str(), _password.c_str());
}

esp_err_t PsychicHandler::authenticate(PsychicHttpServerRequest *request) {
  return request->requestAuthentication(_method, _realm.c_str(), _authFailMsg.c_str());
}

void PsychicHandler::closeCallback(PsychicClient *client) {}

bool PsychicHandler::isWebsocket() {
  return false;
}