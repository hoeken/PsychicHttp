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
#ifndef PsychicEventSource_H_
#define PsychicEventSource_H_

#include <Arduino.h>
#include "PsychicCore.h"
#include "PsychicHandler.h"
#include "PsychicHttpServerResponse.h"
#include "StringArray.h"

#ifndef SSE_MAX_QUEUED_MESSAGES
  #define SSE_MAX_QUEUED_MESSAGES 32
#endif

#ifndef DEFAULT_MAX_SSE_CLIENTS
  #ifdef ESP32
    #define DEFAULT_MAX_SSE_CLIENTS 8
  #else
    #define DEFAULT_MAX_SSE_CLIENTS 4
  #endif
#endif

class PsychicEventSource;
class PsychicEventSourceResponse;
class PsychicEventSourceClient;
class PsychicConnection;
class PsychicHttpServerResponse;

typedef std::function<void(PsychicEventSourceClient *client)> ArEventHandlerFunction;

class PsychicEventSourceMessage {
  private:
    uint8_t * _data; 
    size_t _len;
    size_t _sent;
    size_t _acked; 
  public:
    PsychicEventSourceMessage(const char * data, size_t len);
    ~PsychicEventSourceMessage();
    size_t ack(size_t len, uint32_t time __attribute__((unused)));
    size_t send(PsychicConnection *client);
    bool finished(){ return _acked == _len; }
    bool sent() { return _sent == _len; }
};

class PsychicEventSourceClient {
  private:
    PsychicConnection *_client;
    PsychicEventSource *_server;
    uint32_t _lastId;
    LinkedList<PsychicEventSourceMessage *> _messageQueue;
    void _queueMessage(PsychicEventSourceMessage *dataMessage);
    void _runQueue();

  public:

    PsychicEventSourceClient(PsychicHttpServerRequest *request, PsychicEventSource *server);
    ~PsychicEventSourceClient();

    PsychicConnection* client(){ return _client; }
    void close();
    void write(const char * message, size_t len);
    void send(const char *message, const char *event=NULL, uint32_t id=0, uint32_t reconnect=0);
    //bool connected() const { return (_client != NULL) && _client->connected(); }
    uint32_t lastId() const { return _lastId; }
    size_t  packetsWaiting() const { return _messageQueue.length(); }

    //system callbacks (do not call)
    void _onAck(size_t len, uint32_t time);
    void _onPoll(); 
    void _onTimeout(uint32_t time);
    void _onDisconnect();
};

class PsychicEventSource : public PsychicHandler {
  private:
    LinkedList<PsychicEventSourceClient *> _clients;
    ArEventHandlerFunction _connectcb;
  public:
    PsychicEventSource();
    ~PsychicEventSource();

    void close();
    void onConnect(ArEventHandlerFunction cb);
    void send(const char *message, const char *event=NULL, uint32_t id=0, uint32_t reconnect=0);
    size_t count() const; //number clinets connected
    size_t  avgPacketsWaiting() const;

    //system callbacks (do not call)
    void _addClient(PsychicEventSourceClient * client);
    void _handleDisconnect(PsychicEventSourceClient * client);
    virtual bool canHandle(PsychicHttpServerRequest *request) override final;
    virtual void handleRequest(PsychicHttpServerRequest *request) override final;
};

class PsychicEventSourceResponse: public PsychicHttpServerResponse {
  private:
    String _content;
    PsychicEventSource *_server;
  public:
    PsychicEventSourceResponse(PsychicEventSource *server, PsychicHttpServerRequest *request);
    void _respond(PsychicHttpServerRequest *request);
    size_t _ack(PsychicHttpServerRequest *request, size_t len, uint32_t time);
    bool _sourceValid() const { return true; }
};

class PsychicConnection {
    PsychicConnection(int sockfd);
    ~PsychicConnection();
};

#endif /* PsychicEventSource_H_ */