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
}

PsychicEventSourceClient * PsychicEventSource::getClient(int socket)
{
  PsychicClient *client = PsychicHandler::getClient(socket);

  if (client == NULL)
    return NULL;

  return (PsychicEventSourceClient *)client->_friend;
}

PsychicEventSourceClient * PsychicEventSource::getClient(PsychicClient *client) {
  return getClient(client->socket());
}

esp_err_t PsychicEventSource::handleRequest(PsychicRequest *request)
{
  PsychicEventSourceResponse response(request);
  esp_err_t err = response.send();

  //lookup our client
  PsychicClient *client = checkForNewClient(request->client());
  if (client->isNew)
  {
    //save our friend
    PsychicEventSourceClient *buddy = new PsychicEventSourceClient(client);
    client->_friend = buddy;

    //did we get our last id?
    if(request->hasHeader("Last-Event-ID"))
      buddy->_lastId = atoi(request->header("Last-Event-ID").c_str());

    openCallback(client);
  }

  return err;
}

PsychicEventSource * PsychicEventSource::onOpen(PsychicEventSourceClientCallback fn) {
  _onOpen = fn;
  return this;
}

PsychicEventSource * PsychicEventSource::onClose(PsychicEventSourceClientCallback fn) {
  _onClose = fn;
  return this;
}

void PsychicEventSource::openCallback(PsychicClient *client) {
  if (_onOpen != NULL)
    _onOpen((PsychicEventSourceClient*)client->_friend);
}

void PsychicEventSource::closeCallback(PsychicClient *client) {
  if (_onClose != NULL)
    _onClose((PsychicEventSourceClient*)client->_friend);
  delete (PsychicEventSourceClient*)client->_friend;
}

void PsychicEventSource::send(const char *message, const char *event, uint32_t id, uint32_t reconnect)
{
  String ev = generateEventMessage(message, event, id, reconnect);
  for(PsychicClient *c : _clients) {
    ((PsychicEventSourceClient*)c->_friend)->sendEvent(ev.c_str());
  }
}

/*****************************************/
// PsychicEventSourceClient
/*****************************************/

PsychicEventSourceClient::PsychicEventSourceClient(PsychicClient *client) :
  PsychicClient(client->server(), client->socket()),
  _lastId(0)
{
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

  if (result < 0)
    ESP_LOGE(PH_TAG, "EventSource send failed with %s", esp_err_to_name(result));

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