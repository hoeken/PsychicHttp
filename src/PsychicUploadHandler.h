#ifndef PsychicUploadHandler_h
#define PsychicUploadHandler_h

#include "PsychicCore.h"
#include "PsychicHttpServer.h"
#include "PsychicHttpServerRequest.h"
#include "PsychicWebHandler.h"

/*
* HANDLER :: Can be attached to any endpoint or as a generic request handler.
*/

class PsychicUploadHandler : public PsychicWebHandler {
  protected:
    PsychicHttpBasicUploadHandler _uploadCallback;
    PsychicHttpMultipartUploadHandler _multipartCallback;

    esp_err_t _multipartUploadHandler(PsychicHttpServerRequest *request);
    esp_err_t _basicUploadHandler(PsychicHttpServerRequest *request);

  public:
    PsychicUploadHandler();
    ~PsychicUploadHandler();

    bool canHandle(PsychicHttpServerRequest *request) override;
    esp_err_t handleRequest(PsychicHttpServerRequest *request) override;

    PsychicUploadHandler * onUpload(PsychicHttpBasicUploadHandler fn);
    PsychicUploadHandler * onMultipart(PsychicHttpBasicUploadHandler fn);
    //PsychicUploadHandler * onMultipartChunk(PsychicHttpBasicUploadHandler handler);
};

#endif // PsychicUploadHandler_h