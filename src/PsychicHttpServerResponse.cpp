#include "PsychicHttpServerResponse.h"
#include "PsychicHttpServerRequest.h"
#include <http_status.h>

PsychicHttpServerResponse::PsychicHttpServerResponse(PsychicHttpServerRequest *request) :
  _request(request)
{
  this->setCode(200);
}

PsychicHttpServerResponse::~PsychicHttpServerResponse()
{
}

void PsychicHttpServerResponse::addHeader(const char *field, const char *value)
{
  //these get freed during send()
  HTTPHeader header;
  header.field =(char *)malloc(strlen(field)+1);
  header.value = (char *)malloc(strlen(value)+1);

  strlcpy(header.field, field, strlen(field)+1);
  strlcpy(header.value, value, strlen(value)+1);

  this->headers.push_back(header);
}

void PsychicHttpServerResponse::setCookie(const char *name, const char *value, unsigned long max_age)
{
  String output;
  String v = urlEncode(value);
  output = String(name) + "=" + v;
  output += "; SameSite=Lax";
  output += "; Max-Age=" + String(max_age);

  this->addHeader("Set-Cookie", output.c_str());
}

void PsychicHttpServerResponse::setCode(int code)
{
  //esp-idf makes you set the whole status.
  sprintf(this->_status, "%u %s", code, http_status_reason(code));

  httpd_resp_set_status(this->_request->_req, this->_status);
}

void PsychicHttpServerResponse::setContentType(const char *contentType)
{
  httpd_resp_set_type(this->_request->_req, contentType);
}

void PsychicHttpServerResponse::setContent(const char *content)
{
  this->body = content;
  setContentLength(strlen(content));
}

void PsychicHttpServerResponse::setContent(const uint8_t *content, size_t len)
{
  this->body = (char *)content;

  setContentLength(len);
}

const char * PsychicHttpServerResponse::getContent()
{
  return this->body;
}

size_t PsychicHttpServerResponse::getContentLength()
{
  return this->_contentLength;
}

esp_err_t PsychicHttpServerResponse::send()
{
  TRACE();

  //get our headers out of the way first
  for (HTTPHeader header : this->headers)
  {
    DUMP(header.field);
    DUMP(header.value);
    httpd_resp_set_hdr(this->_request->_req, header.field, header.value);
  }

  TRACE();

  //now send it off
  esp_err_t err = httpd_resp_send(this->_request->_req, this->getContent(), this->getContentLength());

  TRACE();

  //clean up our header variables.  we have to do this since httpd_resp_send doesn't store copies
  for (HTTPHeader header : this->headers)
  {
    free(header.field);
    free(header.value);
  }
  this->headers.clear();

  TRACE();

  if (err != ESP_OK)
  {
    ESP_LOGE(PH_TAG, "Send response failed (%s)", esp_err_to_name(err));
  }

  return err;
}
