//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#include <MicroDebug.h>
#include <MongooseCore.h>
#include <MongooseSntpClient.h>

#ifdef ESP32
#include <WiFi.h>
#define START_ESP_WIFI
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#define START_ESP_WIFI
#else
#error Platform not supported
#endif

#ifndef SNTP_HOST
#define SNTP_HOST "pool.ntp.org"
//#define SNTP_HOST "time.google.com"
#endif

MongooseSntpClient sntp;

const char *ssid = "wifi";
const char *password = "password";

static unsigned long next_time = 0;

void setup()
{
  Serial.begin(115200);

#ifdef START_ESP_WIFI
  //WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.printf("\n\nConnecting to %s - %s\n", ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed! %d\n", WiFi.status());
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

  sntp.onError([](uint8_t err) {
    DBUGF("Got error %u", err);
  });
  
  next_time = millis();
}

void loop()
{
  Mongoose.poll(next_time - millis());
  
  unsigned long now = millis();
  if(now >= next_time)
  {
    next_time = now + 10 * 1000;

    DBUGF("Trying to get time from " SNTP_HOST);
    sntp.getTime(SNTP_HOST, [](double server_time)
    {
      time_t t = time(NULL);
      Serial.printf("Local time: %s\n", ctime(&t));
      t = (time_t)server_time;
      Serial.printf("Time from %s: %s\n", SNTP_HOST, ctime(&t));
    });
  }
}
