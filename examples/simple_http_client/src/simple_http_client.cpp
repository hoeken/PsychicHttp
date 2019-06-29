//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#include <MongooseCore.h>
#include <MongooseHttpClient.h>

#ifdef ESP32
#include <WiFi.h>
#define START_ESP_WIFI
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#define START_ESP_WIFI
#else
#error Platform not supported
#endif

#if MG_ENABLE_SSL
#define PROTO "https"
#else
#define PROTO "http"
#endif

MongooseHttpClient client;

const char *ssid = "wibble";
const char *password = "TheB1gJungle2";

void printResponse(MongooseHttpClientResponse *response)
{
  Serial.printf("%d %.*s\n", response->respCode(), response->respStatusMsg().length(), (const char *)response->respStatusMsg());
  int headers = response->headers();
  int i;
  for(i=0; i<headers; i++) {
    Serial.printf("_HEADER[%.*s]: %.*s\n", 
      response->headerNames(i).length(), (const char *)response->headerNames(i), 
      response->headerValues(i).length(), (const char *)response->headerValues(i));
  }

  Serial.printf("\n%.*s\n", response->body().length(), (const char *)response->body());
}

void setup()
{
  Serial.begin(115200);

#ifdef START_ESP_WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Hostname: ");
#ifdef ESP32
  Serial.println(WiFi.getHostname());
#elif defined(ESP8266)
  Serial.println(WiFi.hostname());
#endif
#endif

  Mongoose.begin();

  // Based on https://github.com/typicode/jsonplaceholder#how-to
  client.get(PROTO"://jsonplaceholder.typicode.com/posts/1", [](MongooseHttpClientResponse *response) {
    printResponse(response);
  });

  client.post(PROTO"://jsonplaceholder.typicode.com/posts", "application/json; charset=UTF-8",
    "{\"title\":\"foo\",\"body\":\"bar\",\"userId\":1}",
    [](MongooseHttpClientResponse *response)
  {
    printResponse(response);
  });

//  client.put(PROTO"://jsonplaceholder.typicode.com/posts/1", "application/json; charset=UTF-8",
//    "{\"id\":1,\"title\":\"foo\",\"body\":\"bar\",\"userId\":1}",
//    [](MongooseHttpClientResponse *response)
//  {
//    printResponse(response);
//  });

//  client.patch(PROTO"://jsonplaceholder.typicode.com/posts/1", "application/json; charset=UTF-8",
//    "{\"title\":\"foo\"}",
//    [](MongooseHttpClientResponse *response)
//  {
//    printResponse(response);
//  });

//  client.delete(PROTO"://jsonplaceholder.typicode.com/posts/1", [](MongooseHttpClientResponse *response) {
//    printResponse(response);
//  });

//  MongooseHttpClientRequest *request = client.beginRequest(PROTO"://jsonplaceholder.typicode.com/posts");
//  request->setMethod(HTTP_GET);
//  request->addHeader("X-hello", "world");
//  request->onBody([](const uint8_t *data, size_t len) {
//    Serial.printf("%.*s", len, (const char *)data));
//  };
//  request->onResponse([](MongooseHttpClientResponse *response) {
//    printResponse(response);
//  });
//  client.send(request);
}

void loop()
{
  Mongoose.poll(1000);
}
