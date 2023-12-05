#include "PsychicHttpWebsocket.h"

/*************************************/
/*  PsychicHttpWebSocketRequest      */
/*************************************/

PsychicHttpWebSocketRequest::PsychicHttpWebSocketRequest(PsychicHttpServer *server, httpd_req_t *req) :
  PsychicHttpServerRequest(server, req)
{
  this->connection = new PsychicHttpWebSocketConnection(this->_server, httpd_req_to_sockfd(this->_req));
}

PsychicHttpWebSocketRequest::~PsychicHttpWebSocketRequest()
{
  delete this->connection;
}

esp_err_t PsychicHttpWebSocketRequest::reply(httpd_ws_frame_t * ws_pkt)
{
  return httpd_ws_send_frame(this->_req, ws_pkt);
} 

esp_err_t PsychicHttpWebSocketRequest::reply(httpd_ws_type_t op, const void *data, size_t len)
{
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  ws_pkt.payload = (uint8_t*)data;
  ws_pkt.len = len;
  ws_pkt.type = op;

  return this->reply(&ws_pkt);
}

esp_err_t PsychicHttpWebSocketRequest::reply(const char *buf)
{
  return this->reply(HTTPD_WS_TYPE_TEXT, buf, strlen(buf));
}

/*************************************/
/*  PsychicHttpWebSocketConnection   */
/*************************************/

PsychicHttpWebSocketConnection::PsychicHttpWebSocketConnection(httpd_handle_t server, int fd) :
  _server(server),
  _fd(fd)
{
}

esp_err_t PsychicHttpWebSocketConnection::queueMessage(httpd_ws_frame_t * ws_pkt)
{
  return httpd_ws_send_frame_async(this->_server, this->_fd, ws_pkt);
} 

esp_err_t PsychicHttpWebSocketConnection::queueMessage(httpd_ws_type_t op, const void *data, size_t len)
{
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  ws_pkt.payload = (uint8_t*)data;
  ws_pkt.len = len;
  ws_pkt.type = op;

  return this->queueMessage(&ws_pkt);
}

esp_err_t PsychicHttpWebSocketConnection::queueMessage(const char *buf)
{
  return this->queueMessage(HTTPD_WS_TYPE_TEXT, buf, strlen(buf));
}

String urlDecode(const char* encoded)
{
  size_t length = strlen(encoded);
  char* decoded = (char*)malloc(length + 1);
  if (!decoded) {
    return "";
  }

  size_t i, j = 0;
  for (i = 0; i < length; ++i) {
      if (encoded[i] == '%' && isxdigit(encoded[i + 1]) && isxdigit(encoded[i + 2])) {
          // Valid percent-encoded sequence
          int hex;
          sscanf(encoded + i + 1, "%2x", &hex);
          decoded[j++] = (char)hex;
          i += 2;  // Skip the two hexadecimal characters
      } else if (encoded[i] == '+') {
          // Convert '+' to space
          decoded[j++] = ' ';
      } else {
          // Copy other characters as they are
          decoded[j++] = encoded[i];
      }
  }

  decoded[j] = '\0';  // Null-terminate the decoded string

  String output(decoded);
  free(decoded);

  return output;
}