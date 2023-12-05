/*
  PsychicHTTP Server Example

  This example code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
*/

/**********************************************************************************************
* Note: this demo relies on various files to be uploaded on the LittleFS partition
* PlatformIO -> Build Filesystem Image and then PlatformIO -> Upload Filesystem Image
**********************************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <PsychicHttp.h>

#ifdef ENABLE_EVENT_SOURCE
  #include <AsyncEventSource.h>
#endif

//Enter your WIFI credentials here.
const char *ssid = "";
const char *password = "";

//credentials for the /auth-basic and /auth-digest examples
const char *app_user = "admin";
const char *app_pass = "admin";
const char *app_name = "Your App";

//hostname for mdns (psychic.local)
const char *local_hostname = "psychic";

//change this to true to enable SSL
bool app_enable_ssl = false;
String server_cert;
String server_key;

//our main server object
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

esp_err_t uploadHandler(PsychicHttpServerRequest *request, const String& filename, uint64_t index, uint8_t *data, size_t len)
{
  File file;
  String path = "/www/" + filename;
  Serial.println("Writing to: " + path);

  //our first call?
  if (!index)
    file = LittleFS.open(path, FILE_WRITE);
  else
    file = LittleFS.open(path, FILE_APPEND);
  
  if(!file) {
    Serial.println("Failed to open file");
    return ESP_FAIL;
  }

  if(!file.write(data, len)) {
    Serial.println("Write failed");
    return ESP_FAIL;
  }

  file.close();
  return ESP_OK;
}

void setup()
{
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network
  // To debug, please enable Core Debug Level to Verbose
  if (connectToWifi())
  {
    //set up our esp32 to listen on the local_hostname.local domain
    if (!MDNS.begin(local_hostname)) {
      Serial.println("Error starting mDNS");
      return;
    }
    MDNS.addService("http", "tcp", 80);

    if(!LittleFS.begin())
    {
      Serial.println("LittleFS Mount Failed. Do Platform -> Build Filesystem Image and Platform -> Upload Filesystem Image from VSCode");
      return;
    }

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

    //setup server config stuff here
    server.config.max_uri_handlers = 20; //maximum number of uri handlers (.on() calls)
    server.ssl_config.httpd.max_uri_handlers = 20; //maximum number of uri handlers (.on() calls)

    //do we want secure or not?
    if (app_enable_ssl)
      server.listen(443, server_cert.c_str(), server_key.c_str());
    else
      server.listen(80);

    //serve static files from LittleFS/www on /
    //this is where our /index.html file lives
    server.serveStatic("/", LittleFS, "/www/");

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
      })->
      onClose([](PsychicHttpServer *server, int sockfd) {
        Serial.printf("[socket] connection closed (#%u)\n", sockfd);
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

    //single point basic file upload - POST to /upload?_filename=filename.ext
    server.on("/upload", HTTP_POST)->onUpload(uploadHandler);

    //wildcard basic file upload - POST to /upload/filename.ext
    server.on("/upload/*", HTTP_POST)->onUpload(uploadHandler);

    //example of getting POST variables
    server.on("/post", HTTP_POST, [](PsychicHttpServerRequest *request)
    {
      Serial.println(request->getParam("param1"));

      String response = "Data: " + request->getParam("param1");

      return request->reply(200, "text/html", response.c_str());
    });
  }
}

void loop()
{
}