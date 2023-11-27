/* Wi-Fi STA Connect and Disconnect Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

*/
#include <Arduino.h>
#include <WiFi.h>
#include <MongooseCore.h>
#include <MongooseHttpServer.h>
#include <LittleFS.h>
#include <ArduinoJSON.h>
#include <ESPmDNS.h>

const char *ssid = "Phoenix";
const char *password = "FulleSende";

const char *app_user = "admin";
const char *app_pass = "admin";
const char *app_name = "ArduinoMongoose";

MongooseHttpServer server;

bool connectToWifi()
{
  Serial.println();
  Serial.print("[WiFi] Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  // Auto reconnect is set true as default
  // To set auto connect off, use the following function
  // WiFi.setAutoReconnect(false);

  // Will try for about 10 seconds (20x 500ms)
  int tryDelay = 500;
  int numberOfTries = 20;

  // Wait for the WiFi event
  while (true)
  {
    switch (WiFi.status())
    {
      case WL_NO_SSID_AVAIL:
        Serial.println("[WiFi] SSID not found");
        break;
      case WL_CONNECT_FAILED:
        Serial.print("[WiFi] Failed - WiFi not connected! Reason: ");
        return false;
        break;
      case WL_CONNECTION_LOST:
        Serial.println("[WiFi] Connection was lost");
        break;
      case WL_SCAN_COMPLETED:
        Serial.println("[WiFi] Scan is completed");
        break;
      case WL_DISCONNECTED:
        Serial.println("[WiFi] WiFi is disconnected");
        break;
      case WL_CONNECTED:
        Serial.println("[WiFi] WiFi is connected!");
        Serial.print("[WiFi] IP address: ");
        Serial.println(WiFi.localIP());
        return true;
        break;
      default:
        Serial.print("[WiFi] WiFi Status: ");
        Serial.println(WiFi.status());
        break;
    }
    delay(tryDelay);

    if (numberOfTries <= 0)
    {
      Serial.print("[WiFi] Failed to connect to WiFi!");
      // Use disconnect function to force stop trying to connect
      WiFi.disconnect();
      return false;
    }
    else
    {
      numberOfTries--;
    }
  }

  return false;
}

void setup()
{
  Serial.begin(115200);
  delay(10);
  Serial.println("ArduinoMongoose Benchmark");

  // We start by connecting to a WiFi network
  // To debug, please enable Core Debug Level to Verbose
  if (connectToWifi())
  {
    if(!LittleFS.begin())
    {
      Serial.println("LittleFS Mount Failed. Do Platform -> Build Filesystem Image and Platform -> Upload Filesystem Image from VSCode");
      return;
    }

    //start our server
    Mongoose.begin();
    server.begin(80);

    //index file
    server.on("/", HTTP_GET, [](MongooseHttpServerRequest *request)
    {
      File fp = LittleFS.open("/www/index.htm");
      String file = fp.readString();
      request->send(200, "text/html", file);
    });

    //api - parameters passed in via query eg. /api/endpoint?foo=bar
    server.on("/api", HTTP_GET, [](MongooseHttpServerRequest *request)
    {
      //create a response object
      StaticJsonDocument<128> output;
      output["msg"] = "status";
      output["status"] = "success";
      output["millis"] = millis();

      //work with some params
      if (request->hasParam("foo"))
      {
        String foo = request->getParam("foo");
        output["foo"] = foo;
      }

      //serialize and return
      String jsonBuffer;
      serializeJson(output, jsonBuffer);
      request->send(200, "application/json", jsonBuffer.c_str());
    });

    //websocket
    server.on("/ws$")->
    onFrame([](MongooseHttpWebSocketConnection *connection, int flags, uint8_t *data, size_t len) {
      Serial.println((char *)data);
      server.sendAll(connection, (char *)data);
    });
  }
}

void loop()
{
  Mongoose.poll(1000);
}