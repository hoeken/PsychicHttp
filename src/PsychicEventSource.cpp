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
#include "Arduino.h"
#include "PsychicEventSource.h"

static String generateEventMessage(const char *message, const char *event, uint32_t id, uint32_t reconnect){
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
    size_t messageLen = strlen(message);
    char * lineStart = (char *)message;
    char * lineEnd;
    do {
      char * nextN = strchr(lineStart, '\n');
      char * nextR = strchr(lineStart, '\r');
      if(nextN == NULL && nextR == NULL){
        size_t llen = ((char *)message + messageLen) - lineStart;
        char * ldata = (char *)malloc(llen+1);
        if(ldata != NULL){
          memcpy(ldata, lineStart, llen);
          ldata[llen] = 0;
          ev += "data: ";
          ev += ldata;
          ev += "\r\n\r\n";
          free(ldata);
        }
        lineStart = (char *)message + messageLen;
      } else {
        char * nextLine = NULL;
        if(nextN != NULL && nextR != NULL){
          if(nextR < nextN){
            lineEnd = nextR;
            if(nextN == (nextR + 1))
              nextLine = nextN + 1;
            else
              nextLine = nextR + 1;
          } else {
            lineEnd = nextN;
            if(nextR == (nextN + 1))
              nextLine = nextR + 1;
            else
              nextLine = nextN + 1;
          }
        } else if(nextN != NULL){
          lineEnd = nextN;
          nextLine = nextN + 1;
        } else {
          lineEnd = nextR;
          nextLine = nextR + 1;
        }

        size_t llen = lineEnd - lineStart;
        char * ldata = (char *)malloc(llen+1);
        if(ldata != NULL){
          memcpy(ldata, lineStart, llen);
          ldata[llen] = 0;
          ev += "data: ";
          ev += ldata;
          ev += "\r\n";
          free(ldata);
        }
        lineStart = nextLine;
        if(lineStart == ((char *)message + messageLen))
          ev += "\r\n";
      }
    } while(lineStart < ((char *)message + messageLen));
  }

  return ev;
}

// Message

PsychicEventSourceMessage::PsychicEventSourceMessage(const char * data, size_t len)
: _data(nullptr), _len(len), _sent(0), _acked(0)
{
  _data = (uint8_t*)malloc(_len+1);
  if(_data == nullptr){
    _len = 0;
  } else {
    memcpy(_data, data, len);
    _data[_len] = 0;
  }
}

PsychicEventSourceMessage::~PsychicEventSourceMessage() {
  if(_data != NULL)
    free(_data);
}

size_t PsychicEventSourceMessage::ack(size_t len, uint32_t time) {
  (void)time;
  // If the whole message is now acked...
  if(_acked + len > _len){
     // Return the number of extra bytes acked (they will be carried on to the next message)
     const size_t extra = _acked + len - _len;
     _acked = _len;
     return extra;
  }
  // Return that no extra bytes left.
  _acked += len;
  return 0;
}

size_t PsychicEventSourceMessage::send(PsychicConnection *client) {
  //TODO: implement this.
  // const size_t len = _len - _sent;
  // if(client->space() < len){
  //   return 0;
  // }
  // size_t sent = client->add((const char *)_data, len);
  // if(client->canSend())
  //   client->send();
  // _sent += sent;
  // return sent; 
  return 0;
}

// Client

PsychicEventSourceClient::PsychicEventSourceClient(PsychicHttpServerRequest *request, PsychicEventSource *server)
{
//  _client = request->client();
  _server = server;
  _lastId = 0;
  if(request->hasHeader("Last-Event-ID"))
    _lastId = atoi(request->header("Last-Event-ID").c_str());

  //TODO: figure out what we need here    
  // _client->setRxTimeout(0);
  // _client->onError(NULL, NULL);
  // _client->onAck([](void *r, PsychicConnection* c, size_t len, uint32_t time){ (void)c; ((PsychicEventSourceClient*)(r))->_onAck(len, time); }, this);
  // _client->onPoll([](void *r, PsychicConnection* c){ (void)c; ((PsychicEventSourceClient*)(r))->_onPoll(); }, this);
  // _client->onData(NULL, NULL);
  // _client->onTimeout([this](void *r, PsychicConnection* c __attribute__((unused)), uint32_t time){ ((PsychicEventSourceClient*)(r))->_onTimeout(time); }, this);
  // _client->onDisconnect([this](void *r, PsychicConnection* c){ ((PsychicEventSourceClient*)(r))->_onDisconnect(); delete c; }, this);

  _server->_addClient(this);
  delete request;
}

PsychicEventSourceClient::~PsychicEventSourceClient(){
  for (auto *message : _messageQueue)
    delete(message);
  _messageQueue.clear();
}

void PsychicEventSourceClient::_queueMessage(PsychicEventSourceMessage *dataMessage){
  //TODO: implement
  // if(dataMessage == NULL)
  //   return;
  // if(!connected()){
  //   delete dataMessage;
  //   return;
  // }
  // if(_messageQueue.length() >= SSE_MAX_QUEUED_MESSAGES){
  //     ets_printf("ERROR: Too many messages queued\n");
  //     delete dataMessage;
  // } else {
  //     _messageQueue.add(dataMessage);
  // }
  // if(_client->canSend())
  //   _runQueue();
}

void PsychicEventSourceClient::_onAck(size_t len, uint32_t time){
  // while(len && !_messageQueue.isEmpty()){
  //   len = _messageQueue.front()->ack(len, time);
  //   if(_messageQueue.front()->finished())
  //     _messageQueue.remove(_messageQueue.front());
  // }

  // _runQueue();
}

void PsychicEventSourceClient::_onPoll(){
  // if(!_messageQueue.isEmpty()){
  //   _runQueue();
  // }
}

void PsychicEventSourceClient::_onTimeout(uint32_t time __attribute__((unused))){
  //TODO
  //_client->close(true);
}

void PsychicEventSourceClient::_onDisconnect(){
  // _client = NULL;
  // _server->_handleDisconnect(this);
}

void PsychicEventSourceClient::close(){
  // TODO
  // if(_client != NULL)
  //   _client->close();
}

void PsychicEventSourceClient::write(const char * message, size_t len){
  _queueMessage(new PsychicEventSourceMessage(message, len));
}

void PsychicEventSourceClient::send(const char *message, const char *event, uint32_t id, uint32_t reconnect){
  String ev = generateEventMessage(message, event, id, reconnect);
  _queueMessage(new PsychicEventSourceMessage(ev.c_str(), ev.length()));
}

void PsychicEventSourceClient::_runQueue(){
  // while(!_messageQueue.isEmpty() && _messageQueue.front()->finished()){
  //   _messageQueue.remove(_messageQueue.front());
  // }

  // for(auto i = _messageQueue.begin(); i != _messageQueue.end(); ++i)
  // {
  //   if(!(*i)->sent())
  //     (*i)->send(_client);
  // }
}

// Handler

PsychicEventSource::PsychicEventSource() :
  _connectcb(NULL)
{}

PsychicEventSource::~PsychicEventSource() {
}

void PsychicEventSource::onConnect(ArEventHandlerFunction cb){
  _connectcb = cb;
}

void PsychicEventSource::_addClient(PsychicEventSourceClient * client){
  /*char * temp = (char *)malloc(2054);
  if(temp != NULL){
    memset(temp+1,' ',2048);
    temp[0] = ':';
    temp[2049] = '\r';
    temp[2050] = '\n';
    temp[2051] = '\r';
    temp[2052] = '\n';
    temp[2053] = 0;
    client->write((const char *)temp, 2053);
    free(temp);
  }*/
  
  _clients.push_back(client);
  if(_connectcb)
    _connectcb(client);
}

void PsychicEventSource::_handleDisconnect(PsychicEventSourceClient * client){
  _clients.remove(client);
}

void PsychicEventSource::close(){
  //TODO
  // for(const auto &c: _clients){
  //   if(c->connected())
  //     c->close();
  // }
}

// pmb fix
size_t PsychicEventSource::avgPacketsWaiting() const {
  if(_clients.size() == 0)
    return 0;
  
  size_t    aql=0;
  uint32_t  nConnectedClients=0;

  //TODO  
  // for(const auto &c: _clients){
  //   if(c->connected()) {
  //     aql+=c->packetsWaiting();
  //     ++nConnectedClients;
  //   }
  // }

  return ((aql) + (nConnectedClients/2))/(nConnectedClients); // round up
}

void PsychicEventSource::send(const char *message, const char *event, uint32_t id, uint32_t reconnect){


  String ev = generateEventMessage(message, event, id, reconnect);
  for(const auto &c: _clients){
    //TODO
    // if(c->connected()) {
    //   c->write(ev.c_str(), ev.length());
    // }
  }
}

size_t PsychicEventSource::count() const {
  //TODO
  // return _clients.count_if([](PsychicEventSourceClient *c){
  //   return c->connected();
  // });
  return 0;
}

//TODO
// bool PsychicEventSource::canHandle(PsychicHttpServerRequest *request){
//   if(request->method() != HTTP_GET || !request->url().equals(_url)) {
//     return false;
//   }
//   request->addInterestingHeader("Last-Event-ID");
//   return true;
// }

//TODO
// void PsychicEventSource::handleRequest(PsychicHttpServerRequest *request){
//   if((_username != "" && _password != "") && !request->authenticate(_username.c_str(), _password.c_str()))
//     return request->requestAuthentication();
//   request->send(new PsychicEventSourceResponse(this));
// }

// Response

PsychicEventSourceResponse::PsychicEventSourceResponse(PsychicEventSource *server, PsychicHttpServerRequest *request) 
  : PsychicHttpServerResponse(request)
{
  _server = server;
  setCode(200);
  setContentType("text/event-stream");
  //TODO:
  //_sendContentLength = false;
  addHeader("Cache-Control", "no-cache");
  addHeader("Connection", "keep-alive");
}

void PsychicEventSourceResponse::_respond(PsychicHttpServerRequest *request){
  // TODO
  // String out = _assembleHead(request->version());
  // request->client()->write(out.c_str(), _headLength);
  // _state = RESPONSE_WAIT_ACK;
}

size_t PsychicEventSourceResponse::_ack(PsychicHttpServerRequest *request, size_t len, uint32_t time __attribute__((unused))){
  if(len){
    new PsychicEventSourceClient(request, _server);
  }
  return 0;
}