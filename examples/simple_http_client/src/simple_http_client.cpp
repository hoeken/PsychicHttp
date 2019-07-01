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

#if MG_ENABLE_SSL
const char *root_ca = 
"-----BEGIN CERTIFICATE-----\r\n"
"MIIENjCCAx6gAwIBAgIBATANBgkqhkiG9w0BAQUFADBvMQswCQYDVQQGEwJTRTEU\r\n"
"MBIGA1UEChMLQWRkVHJ1c3QgQUIxJjAkBgNVBAsTHUFkZFRydXN0IEV4dGVybmFs\r\n"
"IFRUUCBOZXR3b3JrMSIwIAYDVQQDExlBZGRUcnVzdCBFeHRlcm5hbCBDQSBSb290\r\n"
"MB4XDTAwMDUzMDEwNDgzOFoXDTIwMDUzMDEwNDgzOFowbzELMAkGA1UEBhMCU0Ux\r\n"
"FDASBgNVBAoTC0FkZFRydXN0IEFCMSYwJAYDVQQLEx1BZGRUcnVzdCBFeHRlcm5h\r\n"
"bCBUVFAgTmV0d29yazEiMCAGA1UEAxMZQWRkVHJ1c3QgRXh0ZXJuYWwgQ0EgUm9v\r\n"
"dDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALf3GjPm8gAELTngTlvt\r\n"
"H7xsD821+iO2zt6bETOXpClMfZOfvUq8k+0DGuOPz+VtUFrWlymUWoCwSXrbLpX9\r\n"
"uMq/NzgtHj6RQa1wVsfwTz/oMp50ysiQVOnGXw94nZpAPA6sYapeFI+eh6FqUNzX\r\n"
"mk6vBbOmcZSccbNQYArHE504B4YCqOmoaSYYkKtMsE8jqzpPhNjfzp/haW+710LX\r\n"
"a0Tkx63ubUFfclpxCDezeWWkWaCUN/cALw3CknLa0Dhy2xSoRcRdKn23tNbE7qzN\r\n"
"E0S3ySvdQwAl+mG5aWpYIxG3pzOPVnVZ9c0p10a3CitlttNCbxWyuHv77+ldU9U0\r\n"
"WicCAwEAAaOB3DCB2TAdBgNVHQ4EFgQUrb2YejS0Jvf6xCZU7wO94CTLVBowCwYD\r\n"
"VR0PBAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wgZkGA1UdIwSBkTCBjoAUrb2YejS0\r\n"
"Jvf6xCZU7wO94CTLVBqhc6RxMG8xCzAJBgNVBAYTAlNFMRQwEgYDVQQKEwtBZGRU\r\n"
"cnVzdCBBQjEmMCQGA1UECxMdQWRkVHJ1c3QgRXh0ZXJuYWwgVFRQIE5ldHdvcmsx\r\n"
"IjAgBgNVBAMTGUFkZFRydXN0IEV4dGVybmFsIENBIFJvb3SCAQEwDQYJKoZIhvcN\r\n"
"AQEFBQADggEBALCb4IUlwtYj4g+WBpKdQZic2YR5gdkeWxQHIzZlj7DYd7usQWxH\r\n"
"YINRsPkyPef89iYTx4AWpb9a/IfPeHmJIZriTAcKhjW88t5RxNKWt9x+Tu5w/Rw5\r\n"
"6wwCURQtjr0W4MHfRnXnJK3s9EK0hZNwEGe6nQY1ShjTK3rMUUKhemPR5ruhxSvC\r\n"
"Nr4TDea9Y355e6cJDUCrat2PisP29owaQgVR1EX1n6diIWgVIEM8med8vSTYqZEX\r\n"
"c4g/VhsxOBi0cQ+azcgOno4uG+GMmIPLHzHxREzGBHNJdmAPx/i9F4BrLunMTA5a\r\n"
"mnkPIAou1Z5jJh5VkpTYghdae9C8x49OhgQ=\r\n"
"-----END CERTIFICATE-----\r\n";
#endif

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

#if MG_ENABLE_SSL
  client.setRootCa(root_ca);
#endif

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
