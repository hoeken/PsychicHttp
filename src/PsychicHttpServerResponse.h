#ifndef PsychicHttpServerResponse_h
#define PsychicHttpServerResponse_h

#include "PsychicCore.h"

class PsychicHttpServerRequest;

class PsychicHttpServerResponse
{
  protected:
    PsychicHttpServerRequest *_request;
    int64_t _contentLength;
    char _status[60];
    const char * body;

    std::list<HTTPHeader> headers;

  public:
    PsychicHttpServerResponse(PsychicHttpServerRequest *request);
    virtual ~PsychicHttpServerResponse();

    void setCode(int code);

    void setContentType(const char *contentType);
    void setContentLength(int64_t contentLength) { _contentLength = contentLength; }
    int64_t getContentLength(int64_t contentLength) { return _contentLength; }

    void addHeader(const char *field, const char *value);
    void setCookie(const char *key, const char *value, unsigned long max_age = 60*60*24*30);

    void setContent(const char *content);
    void setContent(const uint8_t *content, size_t len);

    const char * getContent();
    size_t getContentLength();

    esp_err_t send();
};

#endif // PsychicHttpServerResponse_h