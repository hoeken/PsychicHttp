#include "PsychicHandler.h"

PsychicHandler::PsychicHandler() : _username(""), _password("") {}
PsychicHandler::~PsychicHandler() {}

PsychicHandler& PsychicHandler::onOpen(PsychicClientCallback fn) {
  _onopen = fn;
  return *this;
}

PsychicHandler& PsychicHandler::onClose(PsychicClientCallback fn) {
  _onclose = fn;
  return *this;
}

void PsychicHandler::closeCallback(PsychicClient *client) {}

PsychicHandler& PsychicHandler::setFilter(PsychicRequestFilterFunction fn) {
  _filter = fn;
  return *this;
}

bool PsychicHandler::filter(PsychicHttpServerRequest *request){
  return _filter == NULL || _filter(request);
}

bool PsychicHandler::isWebsocket() {
  return false;
}

// PsychicHandler& PsychicHandler::setAuthentication(const char *username, const char *password) {
//   _username = String(username);
//   _password = String(password);
//   return *this;
// };