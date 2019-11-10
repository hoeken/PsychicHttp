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
static bool fetching_time = false;

double diff_time(timeval tv1, timeval tv2)
{
    double t1 = (double) tv1.tv_sec + (((double) tv1.tv_usec) / 1000000.0);
    double t2 = (double) tv2.tv_sec + (((double) tv2.tv_usec) / 1000000.0);

    return t1-t2;
}


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
    fetching_time = false;
    next_time = millis() + 10 * 1000;
  });
  
  next_time = millis();
}

void loop()
{
  Mongoose.poll(fetching_time ? 1000 : next_time - millis());

  if(false == fetching_time && millis() >= next_time)
  {
    fetching_time = true;

    DBUGF("Trying to get time from " SNTP_HOST);
    sntp.getTime(SNTP_HOST, [](struct timeval server_time)
    {
      struct timeval local_time;
      gettimeofday(&local_time, NULL);
      Serial.printf("Local time: %s", ctime(&local_time.tv_sec));
      Serial.printf("Time from %s: %s", SNTP_HOST, ctime(&server_time.tv_sec));
      Serial.printf("Diff %.2f\n", diff_time(server_time, local_time));
      settimeofday(&server_time, NULL);
      
      fetching_time = false;
      next_time = millis() + 10 * 1000;
    });
  }
}
