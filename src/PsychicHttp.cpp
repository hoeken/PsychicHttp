#include "PsychicHttp.h"

PsychicHandler::PsychicHandler() : _username(""), _password("") {}

PsychicHandler& PsychicHandler::onOpen(PsychicHttpConnectionHandler fn) {
  _onopen = fn;
  return *this;
}

PsychicHandler& PsychicHandler::onClose(PsychicHttpConnectionHandler fn) {
  _onclose = fn;
  return *this;
}

PsychicHandler& PsychicHandler::setFilter(PsychicRequestFilterFunction fn) {
  _filter = fn;
  return *this;
}

bool PsychicHandler::filter(PsychicHttpServerRequest *request){
  return _filter == NULL || _filter(request);
}

PsychicHandler& PsychicHandler::setAuthentication(const char *username, const char *password) {
  _username = String(username);
  _password = String(password);
  return *this;
};