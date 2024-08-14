#include "PsychicHandler.h"

PsychicHandler::PsychicHandler() : _server(NULL),
                                   _subprotocol("")
{
}

PsychicHandler::~PsychicHandler()
{
  // actual PsychicClient deletion handled by PsychicServer
  // for (PsychicClient *client : _clients)
  //   delete(client);
  _clients.clear();
}

void PsychicHandler::setSubprotocol(const String& subprotocol)
{
  this->_subprotocol = subprotocol;
}
const char* PsychicHandler::getSubprotocol() const
{
  return _subprotocol.c_str();
}

PsychicClient* PsychicHandler::checkForNewClient(PsychicClient* client)
{
  PsychicClient* c = PsychicHandler::getClient(client);
  if (c == NULL) {
    c = client;
    addClient(c);
    c->isNew = true;
  } else
    c->isNew = false;

  return c;
}

void PsychicHandler::checkForClosedClient(PsychicClient* client)
{
  if (hasClient(client)) {
    closeCallback(client);
    removeClient(client);
  }
}

void PsychicHandler::addClient(PsychicClient* client)
{
  _clients.push_back(client);
}

void PsychicHandler::removeClient(PsychicClient* client)
{
  _clients.remove(client);
}

PsychicClient* PsychicHandler::getClient(int socket)
{
  // make sure the server has it too.
  if (!_server->hasClient(socket))
    return NULL;

  // what about us?
  for (PsychicClient* client : _clients)
    if (client->socket() == socket)
      return client;

  // nothing found.
  return NULL;
}

PsychicClient* PsychicHandler::getClient(PsychicClient* client)
{
  return PsychicHandler::getClient(client->socket());
}

bool PsychicHandler::hasClient(PsychicClient* socket)
{
  return PsychicHandler::getClient(socket) != NULL;
}

const std::list<PsychicClient*>& PsychicHandler::getClientList()
{
  return _clients;
}

PsychicHandler* PsychicHandler::setFilter(PsychicRequestFilterFunction fn)
{
  _filter = fn;
  return this;
}

PsychicHandler* PsychicHandler::addMiddleware(PsychicMiddleware* middleware)
{
  _middlewareChain.add(middleware);
  return this;
}

PsychicHandler* PsychicHandler::addMiddleware(PsychicMiddlewareFunction fn)
{
  _middlewareChain.add(fn);
  return this;
}

bool PsychicHandler::removeMiddleware(PsychicMiddleware* middleware)
{
  return _middlewareChain.remove(middleware);
}

esp_err_t PsychicHandler::process(PsychicRequest* request, PsychicResponse* response)
{
  return _middlewareChain.run(request, response, std::bind(&PsychicHandler::handleRequest, this, std::placeholders::_1, std::placeholders::_2));
}
