#ifndef PsychicResponseDelegate_h
#define PsychicResponseDelegate_h

#include "PsychicCore.h"
#include "PsychicResponse.h"
#include "PsychicRequest.h"

class PsychicRequest;

class PsychicResponseDelegate
{
  protected:
    PsychicResponse* _r;

  public:
    PsychicResponseDelegate(PsychicRequest* request) { _r = request->getResponse(); }
    virtual ~PsychicResponseDelegate() {}

    //
    // These are are delegated functions below
    //

    PsychicRequest* request() { return _r->request(); }

    void setCode(int code) { _r->setCode(code); }
    void setContentType(const char* contentType) { _r->setContentType(contentType); }
    void setContentLength(int64_t contentLength) { _r->setContentLength(contentLength); }
    int64_t getContentLength(int64_t contentLength) { return _r->getContentLength(); }

    void addHeader(const char* field, const char* value) { _r->addHeader(field, value); }
    void setCookie(const char* key, const char* value, unsigned long max_age = 60 * 60 * 24 * 30, const char* extras = "") { _r->setCookie(key, value, max_age, extras); }

    void setContent(const char* content) { _r->setContent(content); }
    void setContent(const uint8_t* content, size_t len) { _r->setContent(content, len); }

    const char* getContent() { return _r->getContent(); }
    size_t getContentLength() { return _r->getContentLength(); }

    virtual esp_err_t send() { return _r->send(); }
    void sendHeaders() { _r->sendHeaders(); }
    esp_err_t sendChunk(uint8_t* chunk, size_t chunksize) { return _r->sendChunk(chunk, chunksize); }
    esp_err_t finishChunking() { return _r->finishChunking(); }
};

#endif // PsychicResponseDelegate_h