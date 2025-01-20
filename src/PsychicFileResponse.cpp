#include "PsychicFileResponse.h"
#include "PsychicRequest.h"
#include "PsychicResponse.h"
#include <http_status.h>

PsychicFileResponse::PsychicFileResponse(PsychicResponse* response, FS& fs, const String& path, const String& contentType, bool download) : PsychicResponseDelegate(response)
{
  //_code = 200;
  String _path(path);

  if (!download && !fs.exists(_path) && fs.exists(_path + ".gz")) {
    _path = _path + ".gz";
    addHeader("Content-Encoding", "gzip");
  }

  _content = fs.open(_path, "r");
  setContentLength(_content.size());

  if (contentType == "")
    _setContentTypeFromPath(path);
  else
    setContentType(contentType.c_str());

  int filenameStart = path.lastIndexOf('/') + 1;
  char buf[26 + path.length() - filenameStart];
  char* filename = (char*)path.c_str() + filenameStart;

  if (download) {
    // set filename and force download
    snprintf(buf, sizeof(buf), "attachment; filename=\"%s\"", filename);
  } else {
    // set filename and force rendering
    snprintf(buf, sizeof(buf), "inline; filename=\"%s\"", filename);
  }
  addHeader("Content-Disposition", buf);
}

PsychicFileResponse::PsychicFileResponse(PsychicResponse* response, File content, const String& path, const String& contentType, bool download) : PsychicResponseDelegate(response)
{
  String _path(path);

  if (!download && String(content.name()).endsWith(".gz") && !path.endsWith(".gz")) {
    addHeader("Content-Encoding", "gzip");
  }

  _content = content;
  setContentLength(_content.size());

  if (contentType == "")
    _setContentTypeFromPath(path);
  else
    setContentType(contentType.c_str());

  int filenameStart = path.lastIndexOf('/') + 1;
  char buf[26 + path.length() - filenameStart];
  char* filename = (char*)path.c_str() + filenameStart;

  if (download) {
    snprintf(buf, sizeof(buf), "attachment; filename=\"%s\"", filename);
  } else {
    snprintf(buf, sizeof(buf), "inline; filename=\"%s\"", filename);
  }
  addHeader("Content-Disposition", buf);
}

PsychicFileResponse::~PsychicFileResponse()
{
  if (_content)
    _content.close();
}

void PsychicFileResponse::_setContentTypeFromPath(const String& path)
{
  const char* _contentType;

  if (path.endsWith(".html"))
    _contentType = "text/html";
  else if (path.endsWith(".htm"))
    _contentType = "text/html";
  else if (path.endsWith(".css"))
    _contentType = "text/css";
  else if (path.endsWith(".json"))
    _contentType = "application/json";
  else if (path.endsWith(".js"))
    _contentType = "application/javascript";
  else if (path.endsWith(".png"))
    _contentType = "image/png";
  else if (path.endsWith(".gif"))
    _contentType = "image/gif";
  else if (path.endsWith(".jpg"))
    _contentType = "image/jpeg";
  else if (path.endsWith(".ico"))
    _contentType = "image/x-icon";
  else if (path.endsWith(".svg"))
    _contentType = "image/svg+xml";
  else if (path.endsWith(".eot"))
    _contentType = "font/eot";
  else if (path.endsWith(".woff"))
    _contentType = "font/woff";
  else if (path.endsWith(".woff2"))
    _contentType = "font/woff2";
  else if (path.endsWith(".ttf"))
    _contentType = "font/ttf";
  else if (path.endsWith(".xml"))
    _contentType = "text/xml";
  else if (path.endsWith(".pdf"))
    _contentType = "application/pdf";
  else if (path.endsWith(".zip"))
    _contentType = "application/zip";
  else if (path.endsWith(".gz"))
    _contentType = "application/x-gzip";
  else
    _contentType = "text/plain";

  setContentType(_contentType);
}

esp_err_t PsychicFileResponse::send()
{
  esp_err_t err = ESP_OK;

  // just send small files directly
  size_t size = getContentLength();
  if (size < FILE_CHUNK_SIZE) {
    uint8_t* buffer = (uint8_t*)malloc(size);
    if (buffer == NULL) {
      ESP_LOGE(PH_TAG, "Unable to allocate %" PRIu32 " bytes to send chunk", size);
      httpd_resp_send_err(request(), HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to allocate memory.");
      return ESP_FAIL;
    }

    size_t readSize = _content.readBytes((char*)buffer, size);

    setContent(buffer, readSize);
    err = _response->send();

    free(buffer);
  } else {
    /* Retrieve the pointer to scratch buffer for temporary storage */
    char* chunk = (char*)malloc(FILE_CHUNK_SIZE);
    if (chunk == NULL) {
      ESP_LOGE(PH_TAG, "Unable to allocate %" PRIu32 " bytes to send chunk", FILE_CHUNK_SIZE);
      httpd_resp_send_err(request(), HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to allocate memory.");
      return ESP_FAIL;
    }

    /* Set status and content type before sending headers */
    char* statusBuffer = (char*)malloc(60);
    int code = _response->getCode();
    sprintf(statusBuffer, "%u %s", code, http_status_reason(code));
    httpd_resp_set_status(request(), statusBuffer);

    // set the content type
    httpd_resp_set_type(request(), getContentType().c_str());

    // now the headers
    sendHeaders();

    size_t chunksize;
    do {
      /* Read file in chunks into the scratch buffer */
      chunksize = _content.readBytes(chunk, FILE_CHUNK_SIZE);
      if (chunksize > 0) {
        err = sendChunk((uint8_t*)chunk, chunksize);
        if (err != ESP_OK)
          break;
      }

      /* Keep looping till the whole file is sent */
    } while (chunksize != 0);

    // keep track of our memory
    free(chunk);

    if (err == ESP_OK) {
      ESP_LOGD(PH_TAG, "File sending complete");
      finishChunking();
    }

    // Once sent, free our buffer
    free(statusBuffer);
  }

  return err;
}
