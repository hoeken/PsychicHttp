#include "PsychicResponse.h"
#include "PsychicRequest.h"
#include <http_status.h>

PsychicResponse::PsychicResponse(PsychicRequest *request) :
  _request(request),
  _code(200),
  _status(""),
  _contentLength(0),
  _body("")
{
}

PsychicResponse::~PsychicResponse()
{
  //clean up our header variables.  we have to do this since httpd_resp_send doesn't store copies
  for (HTTPHeader header : _headers)
  {
    free(header.field);
    free(header.value);
  }
  _headers.clear();
}

void PsychicResponse::addHeader(const char *field, const char *value)
{
  //these get freed during send()
  HTTPHeader header;
  header.field =(char *)malloc(strlen(field)+1);
  header.value = (char *)malloc(strlen(value)+1);

  strlcpy(header.field, field, strlen(field)+1);
  strlcpy(header.value, value, strlen(value)+1);

  _headers.push_back(header);
}

void PsychicResponse::setCookie(const char *name, const char *value, unsigned long max_age)
{
  String output;
  String v = urlEncode(value);
  output = String(name) + "=" + v;
  output += "; SameSite=Lax";
  output += "; Max-Age=" + String(max_age);

  addHeader("Set-Cookie", output.c_str());
}

void PsychicResponse::setCode(int code)
{
  _code = code;
}

void PsychicResponse::setContentType(const char *contentType)
{
  httpd_resp_set_type(_request->request(), contentType);
}

void PsychicResponse::setContent(const char *content)
{
  _body = content;
  setContentLength(strlen(content));
}

void PsychicResponse::setContent(const uint8_t *content, size_t len)
{
  _body = (char *)content;
  setContentLength(len);
}

const char * PsychicResponse::getContent()
{
  return _body;
}

size_t PsychicResponse::getContentLength()
{
  return _contentLength;
}

esp_err_t PsychicResponse::send()
{
  //esp-idf makes you set the whole status.
  sprintf(_status, "%u %s", _code, http_status_reason(_code));
  httpd_resp_set_status(_request->request(), _status);

  //get our global headers out of the way first
  for (HTTPHeader header : DefaultHeaders::Instance().getHeaders())
    httpd_resp_set_hdr(_request->request(), header.field, header.value);

  //now do our individual headers
  for (HTTPHeader header : _headers)
    httpd_resp_set_hdr(_request->request(), header.field, header.value);

  //now send it off
  esp_err_t err = httpd_resp_send(_request->request(), getContent(), getContentLength());

  //clean up our header variables.  we have to do this since httpd_resp_send doesn't store copies
  for (HTTPHeader header : _headers)
  {
    free(header.field);
    free(header.value);
  }
  _headers.clear();

  if (err != ESP_OK)
  {
    ESP_LOGE(PH_TAG, "Send response failed (%s)", esp_err_to_name(err));
  }

  return err;
}
