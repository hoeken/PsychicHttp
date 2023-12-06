#include "PsychicHttpFileResponse.h"
#include "PsychicHttpServerResponse.h"
#include "PsychicHttpServerRequest.h"


PsychicHttpFileResponse::PsychicHttpFileResponse(PsychicHttpServerRequest *request, FS &fs, const String& path, const String& contentType, bool download)
 : PsychicHttpServerResponse(request) {
  //_code = 200;
  _path = path;

  if(!download && !fs.exists(_path) && fs.exists(_path+".gz")){
    _path = _path+".gz";
    addHeader("Content-Encoding", "gzip");
    _sendContentLength = true;
    _chunked = false;
  }

  _content = fs.open(_path, "r");
  _contentLength = _content.size();

  if(contentType == "")
    _setContentType(path);
  else
    _contentType = contentType;
  setContentType(_contentType.c_str());

  int filenameStart = path.lastIndexOf('/') + 1;
  char buf[26+path.length()-filenameStart];
  char* filename = (char*)path.c_str() + filenameStart;

  if(download) {
    // set filename and force download
    snprintf(buf, sizeof (buf), "attachment; filename=\"%s\"", filename);
  } else {
    // set filename and force rendering
    snprintf(buf, sizeof (buf), "inline; filename=\"%s\"", filename);
  }
  addHeader("Content-Disposition", buf);
}

PsychicHttpFileResponse::PsychicHttpFileResponse(PsychicHttpServerRequest *request, File content, const String& path, const String& contentType, bool download)
 : PsychicHttpServerResponse(request) {
  _path = path;

  if(!download && String(content.name()).endsWith(".gz") && !path.endsWith(".gz")){
    addHeader("Content-Encoding", "gzip");
    _sendContentLength = true;
    _chunked = false;
  }

  _content = content;
  _contentLength = _content.size();
  setContentLength(_contentLength);

  if(contentType == "")
    _setContentType(path);
  else
    _contentType = contentType;
  setContentType(_contentType.c_str());

  int filenameStart = path.lastIndexOf('/') + 1;
  char buf[26+path.length()-filenameStart];
  char* filename = (char*)path.c_str() + filenameStart;

  if(download) {
    snprintf(buf, sizeof (buf), "attachment; filename=\"%s\"", filename);
  } else {
    snprintf(buf, sizeof (buf), "inline; filename=\"%s\"", filename);
  }
  addHeader("Content-Disposition", buf);
}

PsychicHttpFileResponse::~PsychicHttpFileResponse()
{
  if(_content)
    _content.close();
}

void PsychicHttpFileResponse::_setContentType(const String& path){
  if (path.endsWith(".html")) _contentType = "text/html";
  else if (path.endsWith(".htm")) _contentType = "text/html";
  else if (path.endsWith(".css")) _contentType = "text/css";
  else if (path.endsWith(".json")) _contentType = "application/json";
  else if (path.endsWith(".js")) _contentType = "application/javascript";
  else if (path.endsWith(".png")) _contentType = "image/png";
  else if (path.endsWith(".gif")) _contentType = "image/gif";
  else if (path.endsWith(".jpg")) _contentType = "image/jpeg";
  else if (path.endsWith(".ico")) _contentType = "image/x-icon";
  else if (path.endsWith(".svg")) _contentType = "image/svg+xml";
  else if (path.endsWith(".eot")) _contentType = "font/eot";
  else if (path.endsWith(".woff")) _contentType = "font/woff";
  else if (path.endsWith(".woff2")) _contentType = "font/woff2";
  else if (path.endsWith(".ttf")) _contentType = "font/ttf";
  else if (path.endsWith(".xml")) _contentType = "text/xml";
  else if (path.endsWith(".pdf")) _contentType = "application/pdf";
  else if (path.endsWith(".zip")) _contentType = "application/zip";
  else if(path.endsWith(".gz")) _contentType = "application/x-gzip";
  else _contentType = "text/plain";
}

esp_err_t PsychicHttpFileResponse::send()
{
  esp_err_t err = ESP_OK;

  //just send small files directly
  size_t size = getContentLength();
  if (size < FILE_CHUNK_SIZE)
  {
    uint8_t *buffer = (uint8_t *)malloc(size);
    int readSize = _content.readBytes((char *)buffer, size);

    this->setContent(buffer, size);
    err = PsychicHttpServerResponse::send();
    
    free(buffer);
  }
  else
  {
    //get our headers out of the way first
    for (HTTPHeader header : this->headers)
      httpd_resp_set_hdr(this->_request->request(), header.field, header.value);

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *chunk = (char *)malloc(FILE_CHUNK_SIZE);
    size_t chunksize;
    do {
        /* Read file in chunks into the scratch buffer */
        chunksize = _content.readBytes(chunk, FILE_CHUNK_SIZE);
        if (chunksize > 0)
        {
          /* Send the buffer contents as HTTP response chunk */
          err = httpd_resp_send_chunk(this->_request->request(), chunk, chunksize);
          if (err != ESP_OK)
          {
            ESP_LOGE(PH_TAG, "File sending failed (%s)", esp_err_to_name(err));

            /* Abort sending file */
            httpd_resp_sendstr_chunk(this->_request->request(), NULL);

            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(this->_request->request(), HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");

            //bail
            break;
          }
        }

        /* Keep looping till the whole file is sent */
    } while (chunksize != 0);

    //keep track of our memory
    free(chunk);

    if (err == ESP_OK)
    {
      ESP_LOGI(PH_TAG, "File sending complete");

      /* Respond with an empty chunk to signal HTTP response completion */
      httpd_resp_send_chunk(this->_request->request(), NULL, 0);
    }

    /* Close file after sending complete */
    _content.close();

    //clean up our header variables.  we have to do this since httpd_resp_send doesn't store copies
    for (HTTPHeader header : this->headers)
    {
      free(header.field);
      free(header.value);
    }
    this->headers.clear();
  }

  return err;
}
