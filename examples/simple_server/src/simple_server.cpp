//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#include <MongooseCore.h>
#include <MongooseWebServer.h>

MongoosWebServer server;

const char *PARAM_MESSAGE = "message";

void notFound(MongooseWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void server_setup()
{
  server.on("/", HTTP_GET, [](MongooseWebServerRequest *request) {
    request->send(200, "text/plain", "Hello, world");
  });

  // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [](MongooseWebServerRequest *request) {
    String message;
    if (request->hasParam(PARAM_MESSAGE))
    {
      message = request->getParam(PARAM_MESSAGE)->value();
    }
    else
    {
      message = "No message sent";
    }
    request->send(200, "text/plain", "Hello, GET: " + message);
  });

  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on("/post", HTTP_POST, [](MongooseWebServerRequest *request) {
    String message;
    if (request->hasParam(PARAM_MESSAGE, true))
    {
      message = request->getParam(PARAM_MESSAGE, true)->value();
    }
    else
    {
      message = "No message sent";
    }
    request->send(200, "text/plain", "Hello, POST: " + message);
  });

  server.onNotFound(notFound);

  Mongoose.begin();
  server.begin("80");
}

void server_loop()
{
  Mongoose.poll(1000);
}
