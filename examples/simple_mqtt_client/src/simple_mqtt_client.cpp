//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#include <MicroDebug.h>
#include <MongooseCore.h>
#include <MongooseMqttClient.h>

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

//#define MQTT_HOST "172.16.0.109:1883"
//#define MQTT_HOST "moonshade.lan:1883"
#define MQTT_HOST "test.mosquitto.org:1883"

MongooseMqttClient client;

const char *ssid = "wibble";
const char *password = "TheB1gJungle2";

#if MG_ENABLE_SSL
// Root CA bundle
const char *root_ca =
// test.mosquitto.org Root CAs
"-----BEGIN CERTIFICATE-----\r\n"
"MIIC8DCCAlmgAwIBAgIJAOD63PlXjJi8MA0GCSqGSIb3DQEBBQUAMIGQMQswCQYD\r\n"
"VQQGEwJHQjEXMBUGA1UECAwOVW5pdGVkIEtpbmdkb20xDjAMBgNVBAcMBURlcmJ5\r\n"
"MRIwEAYDVQQKDAlNb3NxdWl0dG8xCzAJBgNVBAsMAkNBMRYwFAYDVQQDDA1tb3Nx\r\n"
"dWl0dG8ub3JnMR8wHQYJKoZIhvcNAQkBFhByb2dlckBhdGNob28ub3JnMB4XDTEy\r\n"
"MDYyOTIyMTE1OVoXDTIyMDYyNzIyMTE1OVowgZAxCzAJBgNVBAYTAkdCMRcwFQYD\r\n"
"VQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwGA1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1v\r\n"
"c3F1aXR0bzELMAkGA1UECwwCQ0ExFjAUBgNVBAMMDW1vc3F1aXR0by5vcmcxHzAd\r\n"
"BgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hvby5vcmcwgZ8wDQYJKoZIhvcNAQEBBQAD\r\n"
"gY0AMIGJAoGBAMYkLmX7SqOT/jJCZoQ1NWdCrr/pq47m3xxyXcI+FLEmwbE3R9vM\r\n"
"rE6sRbP2S89pfrCt7iuITXPKycpUcIU0mtcT1OqxGBV2lb6RaOT2gC5pxyGaFJ+h\r\n"
"A+GIbdYKO3JprPxSBoRponZJvDGEZuM3N7p3S/lRoi7G5wG5mvUmaE5RAgMBAAGj\r\n"
"UDBOMB0GA1UdDgQWBBTad2QneVztIPQzRRGj6ZHKqJTv5jAfBgNVHSMEGDAWgBTa\r\n"
"d2QneVztIPQzRRGj6ZHKqJTv5jAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBQUA\r\n"
"A4GBAAqw1rK4NlRUCUBLhEFUQasjP7xfFqlVbE2cRy0Rs4o3KS0JwzQVBwG85xge\r\n"
"REyPOFdGdhBY2P1FNRy0MDr6xr+D2ZOwxs63dG1nnAnWZg7qwoLgpZ4fESPD3PkA\r\n"
"1ZgKJc2zbSQ9fCPxt2W3mdVav66c6fsb7els2W2Iz7gERJSX\r\n"
"-----END CERTIFICATE-----\r\n"
#endif


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

#if MG_ENABLE_SSL
  client.setRootCa(root_ca);
#endif

  client.onMessage([](MongooseString topic, MongooseString payload) {
    DBUGF("%.*s: %.*s", topic.length(), (const char *)topic, payload.length(), (const char *)payload);
    client.publish("/test", payload);
  });
  client.onError([](uint8_t err) {
    DBUGF("Got error %u", err);
  });

  DBUGF("Trying to connect to " MQTT_HOST);
  client.connect(MQTT_HOST, []()
  {
    DBUGF("Connected, subscribing to #");
    client.subscribe("/stuff");
  });
}

static unsigned long next_time = 0;
void loop()
{
  Mongoose.poll(next_time - millis());

  unsigned long now = millis();
  if(client.connected() && now >= next_time) {
    String time = String(now);
    client.publish("/clock", time);
    next_time += 1000;
    DBUGVAR(next_time);
  }
}
