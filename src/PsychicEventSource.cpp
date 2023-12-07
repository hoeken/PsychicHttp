/*
  Asynchronous WebServer library for Espressif MCUs

  Copyright (c) 2016 Hristo Gochkov. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "PsychicEventSource.h"

/*****************************************/
// PsychicEventSource - Handler
/*****************************************/

PsychicEventSource::PsychicEventSource() :
  PsychicHandler(),
  _onOpen(NULL),
  _onClose(NULL)
{}

PsychicEventSource::~PsychicEventSource() {
  for (auto *client : _clients)
    delete(client);
  _clients.clear();
}

esp_err_t PsychicEventSource::handleRequest(PsychicRequest *request)
{
  PsychicEventSourceResponse response(request);
  esp_err_t err = response.send();

  //lookup our client
  PsychicEventSourceClient *client = getClient(request->client());
  if (client == NULL)
  {
    client = new PsychicEventSourceClient(request->client());
    addClient(client);

    //call our handler
    if (this->_onOpen != NULL)  
      this->_onOpen(client);
  }

  //did we get a last id from teh client?
  if(request->hasHeader("Last-Event-ID"))
    client->_lastId = atoi(request->header("Last-Event-ID").c_str());

  return err;
}

void PsychicEventSource::closeCallback(PsychicClient *client) {
  PsychicEventSourceClient *wsclient = getClient(client);
  if (wsclient != NULL)
  {
    if (_onClose != NULL)
      _onClose(wsclient);

    removeClient(wsclient);
  }
}

void PsychicEventSource::addClient(PsychicEventSourceClient *client) {
  _clients.push_back(client);
}

void PsychicEventSource::removeClient(PsychicEventSourceClient *client) {
  _clients.remove(client);
  delete client;
}

PsychicEventSourceClient * PsychicEventSource::getClient(PsychicClient *socket) {
  for (PsychicEventSourceClient *client : _clients)
    if (client->socket() == socket->socket())
      return client;

  return NULL;
}

bool PsychicEventSource::hasClient(PsychicClient *socket) {
  return getClient(socket) != NULL;
}

PsychicEventSource * PsychicEventSource::onOpen(PsychicEventSourceClientCallback fn) {
  _onOpen = fn;
  return this;
}

PsychicEventSource * PsychicEventSource::onClose(PsychicEventSourceClientCallback fn) {
  _onClose = fn;
  return this;
}

void PsychicEventSource::send(const char *message, const char *event, uint32_t id, uint32_t reconnect)
{
  String ev = generateEventMessage(message, event, id, reconnect);
  for(auto *c : _clients)
    c->sendEvent(ev.c_str());
}

/*****************************************/
// PsychicEventSourceClient
/*****************************************/

PsychicEventSourceClient::PsychicEventSourceClient(PsychicClient *client) : PsychicClient(client->server(), client->socket())
{
  _lastId = 0;
}

PsychicEventSourceClient::~PsychicEventSourceClient(){
}

void PsychicEventSourceClient::send(const char *message, const char *event, uint32_t id, uint32_t reconnect){
  String ev = generateEventMessage(message, event, id, reconnect);
  sendEvent(ev.c_str());
}

void PsychicEventSourceClient::sendEvent(const char *event) {
  int result;
  do {
    result = httpd_socket_send(this->server(), this->socket(), event, strlen(event), 0);
  } while (result == HTTPD_SOCK_ERR_TIMEOUT);

  //if (result < 0)
  //error log here
}

/*****************************************/
// PsychicEventSourceResponse
/*****************************************/

PsychicEventSourceResponse::PsychicEventSourceResponse(PsychicRequest *request) 
  : PsychicResponse(request)
{
}

esp_err_t PsychicEventSourceResponse::send() {

  String out = String();
  out.concat("HTTP/1.1 200 OK\r\n");
  out.concat("Content-Type: text/event-stream\r\n");
  out.concat("Cache-Control: no-cache\r\n");
  out.concat("Connection: keep-alive\r\n");
  out.concat("\r\n");

  int result;
  do {
    result = httpd_send(_request->request(), out.c_str(), out.length());
  } while (result == HTTPD_SOCK_ERR_TIMEOUT);

  //if (result < 0)
  //error log here

  if (result > 0)
    return ESP_OK;
  else
    return ESP_ERR_HTTPD_RESP_SEND;
}

/*****************************************/
// Event Message Generator
/*****************************************/

String generateEventMessage(const char *message, const char *event, uint32_t id, uint32_t reconnect) {
  String ev = "";

  if(reconnect){
    ev += "retry: ";
    ev += String(reconnect);
    ev += "\r\n";
  }

  if(id){
    ev += "id: ";
    ev += String(id);
    ev += "\r\n";
  }

  if(event != NULL){
    ev += "event: ";
    ev += String(event);
    ev += "\r\n";
  }

  if(message != NULL){
    ev += "data: ";
    ev += String(message);
    ev += "\r\n";
  }
  ev += "\r\n";

  return ev;
}