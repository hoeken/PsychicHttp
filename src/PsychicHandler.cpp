#include "PsychicHandler.h"

PsychicHandler::PsychicHandler() :
  _username(""),
  _password(""),
  _method(DIGEST_AUTH),
  _realm(""),
  _authFailMsg(""),
  _onOpen(NULL),
  _onClose(NULL)
  {}

PsychicHandler::~PsychicHandler() {
  // actual PsychicClient deletion handled by PsychicServer
  // for (PsychicClient *client : _clients)
  //   delete(client);
  _clients.clear();
}

PsychicHandler* PsychicHandler::setFilter(PsychicRequestFilterFunction fn) {
  _filter = fn;
  return this;
}

bool PsychicHandler::filter(PsychicRequest *request){
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

bool PsychicHandler::needsAuthentication(PsychicRequest *request) {
  return (_username != "" && _password != "") && !request->authenticate(_username.c_str(), _password.c_str());
}

esp_err_t PsychicHandler::authenticate(PsychicRequest *request) {
  return request->requestAuthentication(_method, _realm.c_str(), _authFailMsg.c_str());
}

PsychicClient * PsychicHandler::checkForNewClient(PsychicClient *client)
{
  PsychicClient *c = getClient(client);
  if (c == NULL)
  {
    c = client;
    addClient(c);
    c->isNew = true;
  }
  else
    c->isNew = false;

  return c;
}

void PsychicHandler::checkForClosedClient(PsychicClient *client)
{
  if (hasClient(client))
  {
    closeCallback(client);
    removeClient(client);
  }
}

void PsychicHandler::addClient(PsychicClient *client) {
  _clients.push_back(client);
}

void PsychicHandler::removeClient(PsychicClient *client) {
  _clients.remove(client);
}

PsychicClient * PsychicHandler::getClient(PsychicClient *socket) {
  for (PsychicClient *client : _clients)
    if (client->socket() == socket->socket())
      return client;

  return NULL;
}

bool PsychicHandler::hasClient(PsychicClient *socket) {
  return getClient(socket) != NULL;
}

const std::list<PsychicClient*>& PsychicHandler::getClientList() {
  return _clients;
}

void PsychicHandler::openCallback(PsychicClient *client) {
  if (_onOpen != NULL)
    this->_onOpen(client);
}

void PsychicHandler::closeCallback(PsychicClient *client) {
  if (_onClose != NULL)
    this->_onClose(client);
}

void PsychicHandler::onOpen(PsychicClientCallback handler) {
  this->_onOpen = handler;
}

void PsychicHandler::onClose(PsychicClientCallback handler) {
  this->_onClose = handler;
}
