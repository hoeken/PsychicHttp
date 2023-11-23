/* Wi-Fi STA Connect and Disconnect Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

*/
#include <Arduino.h>
#include <WiFi.h>
#include <PsychicHttp.h>
#include <LittleFS.h>
#include <ArduinoJSON.h>

const char *ssid = "Phoenix";
const char *password = "FulleSende";

const char *app_user = "admin";
const char *app_pass = "admin";
const char *app_name = "Your App";

bool app_enable_ssl = false;
String server_cert;
String server_key;

PsychicHttpServer server;

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

  // We start by connecting to a WiFi network
  // To debug, please enable Core Debug Level to Verbose
  if (connectToWifi())
  {
    if(!LittleFS.begin())
    {
      Serial.println("LittleFS Mount Failed. Do Platform -> Build Filesystem Image and Platform -> Upload Filesystem Image from VSCode");
      return;
    }

    if (app_enable_ssl)
      Serial.println("SSL enabled");
    else
      Serial.println("SSL disabled");

    //look up our keys?
    if (app_enable_ssl)
    {
      File fp = LittleFS.open("/server.crt");
      if (fp)
      {
        server_cert = fp.readString();

        // Serial.println("Server Cert:");
        // Serial.println(server_cert);
      }
      else
      {
        Serial.println("server.pem not found, SSL not available");
        app_enable_ssl = false;
      }
      fp.close();

      File fp2 = LittleFS.open("/server.key");
      if (fp2)
      {
        server_key = fp2.readString();

        // Serial.println("Server Key:");
        // Serial.println(server_key);
      }
      else
      {
        Serial.println("server.key not found, SSL not available");
        app_enable_ssl = false;
      }
      fp2.close();
    }

    //do we want secure or not?
    if (app_enable_ssl)
      server.listen(443, server_cert.c_str(), server_key.c_str());
    else
      server.listen(80);

    //lets gooo!
    server.start();

    //basic home page
    server.on("/", HTTP_GET, [](PsychicHttpServerRequest *request) {
      PsychicHttpServerResponse response(request);
      response.setCode(200);
      response.setContentType("text/html");
      response.setContent("Homepage");

      return response.send();
    });

    //a websocket echo server
    server.websocket("/ws")->
      onFrame([](PsychicHttpWebSocketRequest *request, httpd_ws_frame *frame) {
        Serial.println((char *)frame->payload);
        request->reply(frame);
        return ESP_OK;
      })->
      onConnect([](PsychicHttpWebSocketRequest *request) {
        Serial.printf("[socket] new connection (#%u)\n", request->connection->id());
        return ESP_OK;
      });

    //example callback everytime a connection is opened
    server.onOpen([](httpd_handle_t hd, int sockfd) {
      Serial.printf("[http] new connection (#%u)\n", sockfd);
      return ESP_OK;
    });

    //example callback everytime a connection is closed
    server.onClose([](httpd_handle_t hd, int sockfd) {
      Serial.printf("[http] connection closed (#%u)\n", sockfd);
      return ESP_OK;
    });

    //api - json message passed in as post body
    server.on("/api", HTTP_POST, [](PsychicHttpServerRequest *request)
    {
      //load our JSON request
      StaticJsonDocument<1024> json;
      String body = request->body();
      DeserializationError err = deserializeJson(json, body);

      //create our response json
      StaticJsonDocument<128> output;
      output["msg"] = "status";
      output["status"] = "success";
      output["millis"] = millis();

      //work with some params
      if (json.containsKey("foo"))
      {
        String foo = json["foo"];
        output["foo"] = foo;
      }

      //serialize and return
      String jsonBuffer;
      serializeJson(output, jsonBuffer);
      return request->reply(200, "application/json", jsonBuffer.c_str());
    });

    //api - parameters passed in via query eg. /api/endpoint?foo=bar
    server.on("/api", HTTP_GET, [](PsychicHttpServerRequest *request)
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
      return request->reply(200, "application/json", jsonBuffer.c_str());
    });

    //how to redirect a request
    server.on("/redirect", HTTP_GET, [](PsychicHttpServerRequest *request)
    {
      return request->redirect("/");
    });

    //how to do basic auth
    server.on("/auth-basic", HTTP_GET, [](PsychicHttpServerRequest *request)
    {
      if (!request->authenticate(app_user, app_pass))
        return request->requestAuthentication(BASIC_AUTH, app_name, "You must log in.");
      return request->reply("Auth Basic Success!");
    });

    //how to do digest auth
    server.on("/auth-digest", HTTP_GET, [](PsychicHttpServerRequest *request)
    {
      if (!request->authenticate(app_user, app_pass))
        return request->requestAuthentication(DIGEST_AUTH, app_name, "You must log in.");
      return request->reply("Auth Digest Success!");
    });

    //example of getting / setting cookies
    server.on("/cookies", HTTP_GET, [](PsychicHttpServerRequest *request)
    {
      PsychicHttpServerResponse response(request);

      int counter = 0;
      if (request->hasCookie("counter"))
      {
        counter = std::stoi(request->getCookie("counter").c_str());
        counter++;
      }

      char cookie[10];
      sprintf(cookie, "%i", counter);

      response.setCookie("counter", cookie);
      response.setContent(cookie);
      return response.send();
    });

    server.serveStatic("/", LittleFS, "/www/");
  }
}

void loop()
{
}