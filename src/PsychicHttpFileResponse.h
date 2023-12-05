#ifndef PsychicHttpFileResponse_h
#define PsychicHttpFileResponse_h

#include "PsychicCore.h"
#include "PsychicHttpServerResponse.h"
#include "PsychicHttpServerRequest.h"

class PsychicHttpFileResponse: public PsychicHttpServerResponse
{
  using File = fs::File;
  using FS = fs::FS;
  private:
    File _content;
    String _path;
    bool _sendContentLength;
    bool _chunked;
    String _contentType;
    void _setContentType(const String& path);
  public:
    PsychicHttpFileResponse(PsychicHttpServerRequest *request, FS &fs, const String& path, const String& contentType=String(), bool download=false);
    PsychicHttpFileResponse(PsychicHttpServerRequest *request, File content, const String& path, const String& contentType=String(), bool download=false);
    ~PsychicHttpFileResponse();
    esp_err_t send();
};

#endif // PsychicHttpFileResponse_h