#include "PsychicClient.h"

PsychicClient::PsychicClient(httpd_handle_t server, int socket) :
  _server(server),
  _socket(socket),
  _friend(NULL),
  isNew(false)
{}

PsychicClient::~PsychicClient() {
}

httpd_handle_t PsychicClient::server() {
  return _server;
}

int PsychicClient::socket() {
  return _socket;
}

IPAddress PsychicClient::localIP()
{
  IPAddress address(0,0,0,0);

  char ipstr[INET6_ADDRSTRLEN];
  struct sockaddr_in6 addr;   // esp_http_server uses IPv6 addressing
  socklen_t addr_size = sizeof(addr);

  if (getsockname(_socket, (struct sockaddr *)&addr, &addr_size) < 0) {
    ESP_LOGE(PH_TAG, "Error getting client IP");
    return address;
  }

  // Convert to IPv4 string
  inet_ntop(AF_INET, &addr.sin6_addr.un.u32_addr[3], ipstr, sizeof(ipstr));
  ESP_LOGI(PH_TAG, "Client Local IP => %s", ipstr);
  address.fromString(ipstr);

  return address;
}

IPAddress PsychicClient::remoteIP()
{
  IPAddress address(0,0,0,0);

  char ipstr[INET6_ADDRSTRLEN];
  struct sockaddr_in6 addr;   // esp_http_server uses IPv6 addressing
  socklen_t addr_size = sizeof(addr);

  if (getpeername(_socket, (struct sockaddr *)&addr, &addr_size) < 0) {
    ESP_LOGE(PH_TAG, "Error getting client IP");
    return address;
  }

  // Convert to IPv4 string
  inet_ntop(AF_INET, &addr.sin6_addr.un.u32_addr[3], ipstr, sizeof(ipstr));
  ESP_LOGI(PH_TAG, "Client Remote IP => %s", ipstr);
  address.fromString(ipstr);

  return address;
}