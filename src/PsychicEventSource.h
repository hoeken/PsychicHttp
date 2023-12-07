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

#include "PsychicCore.h"
#include "PsychicClient.h"
#include "PsychicHandler.h"
#include "PsychicHttpServerResponse.h"

class PsychicEventSource;
class PsychicEventSourceResponse;
class PsychicEventSourceClient;
class PsychicHttpServerResponse;

typedef std::function<esp_err_t(PsychicEventSourceClient *client)> PsychicEventSourceClientCallback;

class PsychicEventSourceClient : public PsychicClient {
  friend PsychicEventSource;

  protected:
    uint32_t _lastId;

  public:
    PsychicEventSourceClient(PsychicClient *client);
    ~PsychicEventSourceClient();

    uint32_t lastId() const { return _lastId; }
    void send(const char *message, const char *event=NULL, uint32_t id=0, uint32_t reconnect=0);
    void sendEvent(const char *event);
};

class PsychicEventSource : public PsychicHandler {
  private:
    std::list<PsychicEventSourceClient*> _clients;

    PsychicEventSourceClientCallback _onOpen;
    PsychicEventSourceClientCallback _onClose;

  public:
    PsychicEventSource();
    ~PsychicEventSource();

    esp_err_t handleRequest(PsychicHttpServerRequest *request) override final;

    void closeCallback(PsychicClient *client) override;
    void addClient(PsychicEventSourceClient *client);
    void removeClient(PsychicEventSourceClient *client);
    PsychicEventSourceClient * getClient(PsychicClient *client);
    bool hasClient(PsychicClient *client);
    int count() { return _clients.size(); }; 

    PsychicEventSource *onOpen(PsychicEventSourceClientCallback fn);
    PsychicEventSource *onClose(PsychicEventSourceClientCallback fn);

    void send(const char *message, const char *event=NULL, uint32_t id=0, uint32_t reconnect=0);
};

class PsychicEventSourceResponse: public PsychicHttpServerResponse {
  public:
    PsychicEventSourceResponse(PsychicHttpServerRequest *request);
    virtual esp_err_t send() override;
};

#endif /* PsychicEventSource_H_ */