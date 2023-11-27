/* Wi-Fi STA Connect and Disconnect Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

*/
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJSON.h>
#include <ESPmDNS.h>

const char *ssid = "Phoenix";
const char *password = "FulleSende";

const char *app_user = "admin";
const char *app_pass = "admin";
const char *app_name = "ESPAsyncWebserver";
const char *local_hostname = "espasync";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

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

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    //client connected
    // Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    // client->printf("Hello Client %u :)", client->id());
    // client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    //client disconnected
    // Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    //error was received from the other end
    // Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    //pong message was received (in response to a ping request maybe)
    // Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      // Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
      if(info->opcode == WS_TEXT){
        data[len] = 0;
        // Serial.printf("%s\n", (char*)data);
      } else {
        // for(size_t i=0; i < info->len; i++){
        //   Serial.printf("%02x ", data[i]);
        // }
        // Serial.printf("\n");
      }
      if(info->opcode == WS_TEXT)
      {
        Serial.println((char *)data);
        client->text((char *)data, len);
      }
      // else
      //   client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        // if(info->num == 0)
        //   Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        // Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
      if(info->message_opcode == WS_TEXT){
        data[len] = 0;
        // Serial.printf("%s\n", (char*)data);
      } else {
        // for(size_t i=0; i < len; i++){
        //   Serial.printf("%02x ", data[i]);
        // }
        // Serial.printf("\n");
      }

      if((info->index + len) == info->len){
        // Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          // Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
          {
            Serial.println((char *)data);
            client->text((char *)data, info->len);
          }
          // else
          //   client->binary("I got your binary message");
        }
      }
    }
  }
}

esp_err_t uploadHandler(AsyncWebServerRequest *request, const String& filename, uint64_t index, uint8_t *data, size_t len)
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
  Serial.println("ESPAsyncWebserver Benchmark");

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

    //serve static files from LittleFS/www on /
    //this is where our /index.html file lives
    server.serveStatic("/", LittleFS, "/www/");

    //api - parameters passed in via query eg. /api/endpoint?foo=bar
    server.on("/api", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      //create a response object
      StaticJsonDocument<128> output;
      output["msg"] = "status";
      output["status"] = "success";
      output["millis"] = millis();

      //work with some params
      if (request->hasParam("foo"))
      {
        AsyncWebParameter* foo = request->getParam("foo");
        output["foo"] = foo->value();
      }

      //serialize and return
      String jsonBuffer;
      serializeJson(output, jsonBuffer);
      request->send(200, "application/json", jsonBuffer.c_str());
    });

    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.begin();
  }
}

void loop()
{
  ws.cleanupClients();
}