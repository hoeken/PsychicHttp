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

#include "_secret.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <PsychicHttp.h>
#include <WiFi.h>
#include <esp_sntp.h>

// #define this to enable SSL at build (or switch to the 'ssl' build target in vscode)
#ifdef PSY_ENABLE_SSL
  #include <PsychicHttpsServer.h>
#endif

#ifndef WIFI_SSID
  #error "You need to enter your wifi credentials. Rename secret.h to _secret.h and enter your credentials there."
#endif

// Enter your WIFI credentials in secret.h
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;

// Set your SoftAP credentials
const char* softap_ssid = "PsychicHttp";
const char* softap_password = "";
IPAddress softap_ip(10, 0, 0, 1);

// credentials for the /auth-basic and /auth-digest examples
const char* app_user = "admin";
const char* app_pass = "admin";
const char* app_name = "Your App";

// hostname for mdns (psychic.local)
const char* local_hostname = "psychic";

// #define PSY_ENABLE_SSL to enable ssl
#ifdef PSY_ENABLE_SSL
bool app_enable_ssl = true;
String server_cert;
String server_key;
#endif

// our main server object
#ifdef PSY_ENABLE_SSL
PsychicHttpsServer server;
#else
PsychicHttpServer server;
#endif
PsychicWebSocketHandler websocketHandler;
PsychicEventSource eventSource;

// NTP server stuff
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;
struct tm timeinfo;

// Callback function (gets called when time adjusts via NTP)
void timeAvailable(struct timeval* t)
{
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }

  Serial.print("NTP update: ");
  char buffer[40];
  strftime(buffer, 40, "%FT%T%z", &timeinfo);
  Serial.println(buffer);
}

bool connectToWifi()
{
  // dual client and AP mode
  WiFi.mode(WIFI_AP_STA);

  // Configure SoftAP
  WiFi.softAPConfig(softap_ip, softap_ip, IPAddress(255, 255, 255, 0)); // subnet FF FF FF 00
  WiFi.softAP(softap_ssid, softap_password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("SoftAP IP Address: ");
  Serial.println(myIP);

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
  esp_log_level_set(PH_TAG, ESP_LOG_DEBUG);
  esp_log_level_set("httpd_uri", ESP_LOG_DEBUG);

  Serial.begin(115200);
  delay(10);

  Serial.printf("ESP-IDF Version: %s\n", esp_get_idf_version());

#ifdef ESP_ARDUINO_VERSION_STR
  Serial.printf("Arduino Version: %s\n", ESP_ARDUINO_VERSION_STR);
#else
  Serial.printf("Arduino Version: %d.%d.%d\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
#endif
  Serial.printf("PsychicHttp Version: %s\n", PSYCHIC_VERSION_STR);

  // We start by connecting to a WiFi network
  // To debug, please enable Core Debug Level to Verbose
  if (connectToWifi())
  {
    // Setup our NTP to get the current time.
    sntp_set_time_sync_notification_cb(timeAvailable);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 2)
    esp_sntp_servermode_dhcp(1); // (optional)
#else
    sntp_servermode_dhcp(1); // (optional)
#endif
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

    // set up our esp32 to listen on the local_hostname.local domain
    if (!MDNS.begin(local_hostname))
    {
      Serial.println("Error starting mDNS");
      return;
    }
    MDNS.addService("http", "tcp", 80);

    if (!LittleFS.begin())
    {
      Serial.println("LittleFS Mount Failed. Do Platform -> Build Filesystem Image and Platform -> Upload Filesystem Image from VSCode");
      return;
    }

// look up our keys?
#ifdef PSY_ENABLE_SSL
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
#endif

#ifdef PSY_ENABLE_SSL
    // do we want secure or not?
    if (app_enable_ssl)
    {
      server.setCertificate(server_cert.c_str(), server_key.c_str());

      // this creates a 2nd server listening on port 80 and redirects all requests HTTPS
      PsychicHttpServer* redirectServer = new PsychicHttpServer();
      redirectServer->config.ctrl_port = 20424; // just a random port different from the default one
      redirectServer->onNotFound([](PsychicRequest* request)
        {
        String url = "https://" + request->host() + request->url();
        return request->redirect(url.c_str()); });
    }
#endif

    DefaultHeaders::Instance().addHeader("Server", "PsychicHttp");

    // rewrites!
    server.rewrite("/rewrite", "/api?foo=rewrite");

    // serve static files from LittleFS/www on / only to clients on same wifi network
    // this is where our /index.html file lives
    //  curl -i http://psychic.local/
    PsychicStaticFileHandler* handler = server.serveStatic("/", LittleFS, "/www/");
    handler->setFilter(ON_STA_FILTER);
    handler->setCacheControl("max-age=60");

    // serve static files from LittleFS/www-ap on / only to clients on SoftAP
    // this is where our /index.html file lives
    server.serveStatic("/", LittleFS, "/www-ap/")->setFilter(ON_AP_FILTER);

    // serve static files from LittleFS/img on /img
    // it's more efficient to serve everything from a single www directory, but this is also possible.
    //  curl -i http://psychic.local/img/request_flow.png
    server.serveStatic("/img", LittleFS, "/img/");

    // you can also serve single files
    //  curl -i http://psychic.local/myfile.txt
    server.serveStatic("/myfile.txt", LittleFS, "/custom.txt");

    // example callback everytime a connection is opened
    server.onOpen([](PsychicClient* client)
      { Serial.printf("[http] connection #%u connected from %s\n", client->socket(), client->remoteIP().toString().c_str()); });

    // example callback everytime a connection is closed
    server.onClose([](PsychicClient* client)
      { Serial.printf("[http] connection #%u closed from %s\n", client->socket(), client->remoteIP().toString().c_str()); });

    // api - json message passed in as post body
    //  curl -i -X POST -H "Content-Type: application/json" -d '{"foo":"bar"}' http://psychic.local/api
    server.on("/api", HTTP_POST, [](PsychicRequest* request, JsonVariant& json)
      {
      JsonObject input = json.as<JsonObject>();

      //create our response json
      PsychicJsonResponse response = PsychicJsonResponse(request);
      JsonObject output = response.getRoot();

      output["msg"] = "status";
      output["status"] = "success";
      output["millis"] = millis();
      output["method"] = request->methodStr();

      //work with some params
      if (input.containsKey("foo"))
      {
        String foo = input["foo"];
        output["foo"] = foo;
      }
  
      return response.send(); });

    // ip - get info about the client
    //  curl -i http://psychic.local/ip
    server.on("/ip", HTTP_GET, [](PsychicRequest* request)
      {
      String output = "Your IP is: " + request->client()->remoteIP().toString();
      return request->reply(output.c_str()); });

    // client connect/disconnect to a url
    //  curl -i http://psychic.local/handler
    PsychicWebHandler* connectionHandler = new PsychicWebHandler();
    connectionHandler->onRequest([](PsychicRequest* request)
      { return request->reply("OK"); });
    connectionHandler->onOpen([](PsychicClient* client)
      { Serial.printf("[handler] connection #%u connected from %s\n", client->socket(), client->remoteIP().toString().c_str()); });
    connectionHandler->onClose([](PsychicClient* client)
      { Serial.printf("[handler] connection #%u closed from %s\n", client->socket(), client->remoteIP().toString().c_str()); });

    // add it to our server
    server.on("/handler", connectionHandler);

    // api - parameters passed in via query eg. /api?foo=bar
    //  curl -i 'http://psychic.local/api?foo=bar'
    server.on("/api", HTTP_GET, [](PsychicRequest* request)
      {
      //create our response json
      PsychicJsonResponse response = PsychicJsonResponse(request);
      JsonObject output = response.getRoot();

      output["msg"] = "status";
      output["status"] = "success";
      output["millis"] = millis();
      output["method"] = request->methodStr();

      //work with some params
      if (request->hasParam("foo"))
      {
        String foo = request->getParam("foo")->value();
        output["foo"] = foo;
      }

      return response.send(); });

    // curl -i -X GET 'http://psychic.local/any'
    // curl -i -X POST 'http://psychic.local/any'
    server.on("/any", HTTP_ANY, [](PsychicRequest* request)
      {
      //create our response json
      PsychicJsonResponse response = PsychicJsonResponse(request);
      JsonObject output = response.getRoot();

      output["msg"] = "status";
      output["status"] = "success";
      output["millis"] = millis();
      output["method"] = request->methodStr();

      return response.send(); });

    // curl -i 'http://psychic.local/simple'
    server.on("/simple", HTTP_GET, [](PsychicRequest* request)
            { return request->reply("Simple"); })
      ->setURIMatchFunction(MATCH_SIMPLE);

#ifdef PSY_ENABLE_REGEX
    // curl -i 'http://psychic.local/regex/23'
    // curl -i 'http://psychic.local/regex/4223'
    server.on("^/regex/([\\d]+)/?$", HTTP_GET, [](PsychicRequest* request)
            {
        //look up our regex matches
        std::smatch matches;
        if (request->getRegexMatches(matches))
        {
          String output;
          output += "Matches: " + String(matches.size()) + "<br/>\n";
          output += "Matched URI: " + String(matches.str(0).c_str()) + "<br/>\n";
          output += "Match 1: " + String(matches.str(1).c_str()) + "<br/>\n";

          return request->reply(output.c_str());
        }
        else
          return request->reply("No regex match."); })
      ->setURIMatchFunction(MATCH_REGEX);
#endif

    // JsonResponse example
    //  curl -i http://psychic.local/json
    server.on("/json", HTTP_GET, [](PsychicRequest* request)
      {
      PsychicJsonResponse response = PsychicJsonResponse(request);

      char key[16];
      char value[32];
      JsonObject root = response.getRoot();
      for (int i=0; i<100; i++)
      {
        sprintf(key, "key%d", i);
        sprintf(value, "value is %d", i);
        root[key] = value;
      }

      return response.send(); });

    // how to redirect a request
    //  curl -i  http://psychic.local/redirect
    server.on("/redirect", HTTP_GET, [](PsychicRequest* request)
      { return request->redirect("/alien.png"); });

    // how to do basic auth
    //  curl -i --user admin:admin http://psychic.local/auth-basic
    server.on("/auth-basic", HTTP_GET, [](PsychicRequest* request)
      {
      if (!request->authenticate(app_user, app_pass))
        return request->requestAuthentication(BASIC_AUTH, app_name, "You must log in.");
      return request->reply("Auth Basic Success!"); });

    // how to do digest auth
    //  curl -i --user admin:admin http://psychic.local/auth-digest
    server.on("/auth-digest", HTTP_GET, [](PsychicRequest* request)
      {
      if (!request->authenticate(app_user, app_pass))
        return request->requestAuthentication(DIGEST_AUTH, app_name, "You must log in.");
      return request->reply("Auth Digest Success!"); });

    // example of getting / setting cookies
    //  curl -i -b cookie.txt -c cookie.txt http://psychic.local/cookies
    server.on("/cookies", HTTP_GET, [](PsychicRequest* request)
      {
      PsychicResponse response(request);

      int counter = 0;
      if (request->hasCookie("counter"))
      {
        counter = std::stoi(request->getCookie("counter").c_str());
        counter++;
      }

      char cookie[14];
      sprintf(cookie, "%d", counter);

      response.setCookie("counter", cookie);
      response.setContent(cookie);
      return response.send(); });

    // example of getting POST variables
    //  curl -i -d "param1=value1&param2=value2" -X POST http://psychic.local/post
    //  curl -F "param1=value1" -F "param2=value2" -X POST http://psychic.local/post
    server.on("/post", HTTP_POST, [](PsychicRequest* request)
      {
      String output;
      output += "Param 1: " + request->getParam("param1")->value() + "<br/>\n";
      output += "Param 2: " + request->getParam("param2")->value() + "<br/>\n";

      return request->reply(output.c_str()); });

    // you can set up a custom 404 handler.
    //  curl -i http://psychic.local/404
    server.onNotFound([](PsychicRequest* request)
      { return request->reply(404, "text/html", "Custom 404 Handler"); });

    // handle a very basic upload as post body
    PsychicUploadHandler* uploadHandler = new PsychicUploadHandler();
    uploadHandler->onUpload([](PsychicRequest* request, const String& filename, uint64_t index, uint8_t* data, size_t len, bool last)
      {
      File file;
      String path = "/www/" + filename;

      Serial.printf("Writing %d/%d bytes to: %s\n", (int)index+(int)len, request->contentLength(), path.c_str());

      if (last)
        Serial.printf("%s is finished. Total bytes: %llu\n", path.c_str(), (uint64_t)index + (uint64_t)len);

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

      return ESP_OK; });

    // gets called after upload has been handled
    uploadHandler->onRequest([](PsychicRequest* request)
      {
      String url = "/" + request->getFilename();
      String output = "<a href=\"" + url + "\">" + url + "</a>";

      return request->reply(output.c_str()); });

    // wildcard basic file upload - POST to /upload/filename.ext
    //  use http://psychic.local/ to test
    server.on("/upload/*", HTTP_POST, uploadHandler);

    // a little bit more complicated multipart form
    PsychicUploadHandler* multipartHandler = new PsychicUploadHandler();
    multipartHandler->onUpload([](PsychicRequest* request, const String& filename, uint64_t index, uint8_t* data, size_t len, bool last)
      {
        File file;
        String path = "/www/" + filename;

        // some progress over serial.
        Serial.printf("Writing %d bytes to: %s @ index %llu\n", (int)len, path.c_str(), index);
        if (last)
          Serial.printf("%s is finished. Total bytes: %llu\n", path.c_str(), (uint64_t)index + (uint64_t)len);

        // our first call?
        if (!index)
          file = LittleFS.open(path, FILE_WRITE);
        else
          file = LittleFS.open(path, FILE_APPEND);

        if (!file)
        {
          Serial.println("Failed to open file");
          return ESP_FAIL;
        }

        if (!file.write(data, len))
        {
          Serial.println("Write failed");
          return ESP_FAIL;
        }

        return ESP_OK; });

    // gets called after upload has been handled
    multipartHandler->onRequest([](PsychicRequest* request)
      {
        String output;
        if (request->hasParam("file_upload"))
        {
          PsychicWebParameter* file = request->getParam("file_upload");

          String url = "/" + file->value();
          output += "<a href=\"" + url + "\">" + url + "</a><br/>\n";
          output += "Bytes: " + String(file->size()) + "<br/>\n";
        }

        if (request->hasParam("param1"))
          output += "Param 1: " + request->getParam("param1")->value() + "<br/>\n";
        if (request->hasParam("param2"))
          output += "Param 2: " + request->getParam("param2")->value() + "<br/>\n";

        return request->reply(output.c_str()); });

    // wildcard basic file upload - POST to /upload/filename.ext
    //  use http://psychic.local/ to test
    // just multipart data: curl -F "param1=multi" -F "param2=part" http://psychic.local/multipart
    server.on("/multipart", HTTP_POST, multipartHandler);

    // form only multipart handler
    // curl -F "param1=multi" -F "param2=part" http://psychic.local/multipart-data
    PsychicUploadHandler* multipartFormHandler = new PsychicUploadHandler();
    multipartFormHandler->onRequest([](PsychicRequest* request)
      {
        String output;
        if (request->hasParam("param1"))
          output += "Param 1: " + request->getParam("param1")->value() + "<br/>\n";
        if (request->hasParam("param2"))
          output += "Param 2: " + request->getParam("param2")->value() + "<br/>\n";

        return request->reply(output.c_str()); });
    server.on("/multipart-data", HTTP_POST, multipartFormHandler);

    // a websocket echo server
    //  npm install -g wscat
    // Plaintext: wscat -c ws://psychic.local/ws
    // SSL: wscat -n -c wss://psychic.local/ws
    websocketHandler.onOpen([](PsychicWebSocketClient* client)
      {
      Serial.printf("[socket] connection #%u connected from %s\n", client->socket(), client->remoteIP().toString().c_str());
      client->sendMessage("Hello!"); });
    websocketHandler.onFrame([](PsychicWebSocketRequest* request, httpd_ws_frame* frame)
      {
      Serial.printf("[socket] #%d sent: %s\n", request->client()->socket(), (char *)frame->payload);
      return request->reply(frame); });
    websocketHandler.onClose([](PsychicWebSocketClient* client)
      { Serial.printf("[socket] connection #%u closed from %s\n", client->socket(), client->remoteIP().toString().c_str()); });
    server.on("/ws", &websocketHandler);

    // EventSource server
    //  curl -i -N http://psychic.local/events
    eventSource.onOpen([](PsychicEventSourceClient* client)
      {
      Serial.printf("[eventsource] connection #%u connected from %s\n", client->socket(), client->remoteIP().toString().c_str());
      client->send("Hello user!", NULL, millis(), 1000); });
    eventSource.onClose([](PsychicEventSourceClient* client)
      { Serial.printf("[eventsource] connection #%u closed from %s\n", client->socket(), client->remoteIP().toString().c_str()); });
    server.on("/events", &eventSource);

    // example of using POST data inside the
    // works: curl -F "secret=password" http://psychic.local/post-filter
    // 404:   curl -F "foo=bar" http://psychic.local/post-filter
    server.on("/post-filter", HTTP_POST, [](PsychicRequest* request)
            {
      String output;
      output += "Secret: " + request->getParam("secret")->value() + "<br/>\n";

      return request->reply(output.c_str()); })
      ->setFilter([](PsychicRequest* request)
        {
      request->loadParams();
      return request->hasParam("secret"); });

    server.begin();
  }
}

unsigned long lastUpdate = 0;
char output[60];

void loop()
{
  if (millis() - lastUpdate > 2000)
  {
    sprintf(output, "Millis: %lu\n", millis());
    websocketHandler.sendAll(output);

    sprintf(output, "%lu", millis());
    eventSource.send(output, "millis", millis(), 0);

    lastUpdate = millis();
  }
}