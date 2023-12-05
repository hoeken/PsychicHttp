#include "PsychicHttpServerRequest.h"
#include "PsychicHttpServerResponse.h"
#include <http_status.h>

PsychicHttpServerRequest::PsychicHttpServerRequest(PsychicHttpServer *server, httpd_req_t *req) :
  _server(server),
  _req(req),
  _tempObject(NULL)
{
  //handle our session data
  if (req->sess_ctx != NULL)
    this->_session = (SessionData *)req->sess_ctx;
  else
  {
    this->_session = new SessionData();
    req->sess_ctx = this->_session;
  }

  //callback for freeing the session later
  req->free_ctx = this->freeSession;

  //load up some data
  this->_uri = String(this->_req->uri);

  //did we get a query string?
  size_t query_len = httpd_req_get_url_query_len(this->_req);
  if (query_len)
  {
    char query[query_len+1];
    httpd_req_get_url_query_str(this->_req, query, sizeof(query));
    this->_query.concat(query);
  }
}

PsychicHttpServerRequest::~PsychicHttpServerRequest()
{
  // if(this->_response) {
  //   delete this->_response;
  //   this->_response = NULL;
  // }

  if (_tempObject != NULL)
    free(_tempObject);
}

void PsychicHttpServerRequest::freeSession(void *ctx)
{
  if (ctx != NULL)
  {
    SessionData *session = (SessionData*)ctx;
    delete session;
  }
}

// bool PsychicHttpServerRequest::isUpload()
// {
//   if (this->header("Expect").equals("100-continue"))
//     return true;
    
//   if (this->hasHeader("Content-Disposition"))
//     return true;

//   if (this->isMultipart())
//     return true;

//   return false;
// }

const String PsychicHttpServerRequest::getFilename()
{
  //parse the content-disposition header
  if (this->hasHeader("Content-Disposition"))
  {
    ContentDisposition cd = this->getContentDisposition();
    if (cd.filename != "")
      return cd.filename;
  }

  //fall back to passed in query string
  if (this->hasParam("_filename"))
    return this->getParam("_filename");

  //fall back to parsing it from url (useful for wildcard uploads)
  String uri = this->uri();
  int filenameStart = uri.lastIndexOf('/') + 1;
  String filename = uri.substring(filenameStart);
  if (filename != "")
    return filename;

  //finally, unknown.
  ESP_LOGE(PH_TAG, "Did not get a valid filename from the upload.");
  return "unknown.txt";
}

const ContentDisposition PsychicHttpServerRequest::getContentDisposition()
{
  ContentDisposition cd;
  String header = this->header("Content-Disposition");
  int start;
  int end;

  if (header.indexOf("form-data") == 0)
    cd.disposition = FORM_DATA;
  else if (header.indexOf("attachment") == 0)
    cd.disposition = ATTACHMENT;
  else if (header.indexOf("inline") == 0)
    cd.disposition = INLINE;
  else 
    cd.disposition = NONE;

  start = header.indexOf("filename=");
  if (start)
  {
    end = header.indexOf('"', start+10);
    cd.filename = header.substring(start+10, end-1);
  }

  start = header.indexOf("name=");
  if (start)
  {
    end = header.indexOf('"', start+6);
    cd.name = header.substring(start+6, end-1);
  }

  return cd;
}


esp_err_t PsychicHttpServerRequest::loadBody()
{
  esp_err_t err = ESP_OK;

  this->_body = String();

  //Get header value string length and allocate memory for length + 1, extra byte for null termination
  size_t remaining = this->_req->content_len;
  char *buf = (char *)malloc(remaining+1);

  //while loop for retries
  while (remaining > 0)
  {
    //read our data from the socket
    int received = httpd_req_recv(this->_req, buf, this->_req->content_len);

    //Retry if timeout occurred
    if (received == HTTPD_SOCK_ERR_TIMEOUT)
      continue;
    //bail if we got an error
    else if (received == HTTPD_SOCK_ERR_FAIL)
    {
      err = ESP_FAIL;
      break;
    }

    //keep track of our 
    remaining -= received;
  }

  //null terminate and make our string
  buf[this->_req->content_len] = '\0';
  this->_body = String(buf);

  //keep track of that pesky memory
  free(buf);

  return err;
}

http_method PsychicHttpServerRequest::method() {
  return (http_method)this->_req->method;
}

const String PsychicHttpServerRequest::methodStr() {
  return String(http_method_str((http_method)this->_req->method));
}

const String& PsychicHttpServerRequest::uri() {
  return this->_uri;
}

const String PsychicHttpServerRequest::queryString() {
  return this->_query;
}

// no way to get list of headers yet....
// int PsychicHttpServerRequest::headers()
// {
// }

const String PsychicHttpServerRequest::header(const char *name)
{
  size_t header_len = httpd_req_get_hdr_value_len(this->_req, name);

  //if we've got one, allocated it and load it
  if (header_len)
  {
    char header[header_len+1];
    httpd_req_get_hdr_value_str(this->_req, name, header, sizeof(header));
    return String(header);
  }
  else
    return "";
}

bool PsychicHttpServerRequest::hasHeader(const char *name)
{
  return httpd_req_get_hdr_value_len(this->_req, name) > 0;
}

const String PsychicHttpServerRequest::host() {
  return this->header("Host");
}

const String PsychicHttpServerRequest::contentType() {
  return header("Content-Type");
}

size_t PsychicHttpServerRequest::contentLength() {
  return this->_req->content_len;
}

const String& PsychicHttpServerRequest::body()
{
  return this->_body;
}

bool PsychicHttpServerRequest::isMultipart()
{
  const String& type = this->contentType();

  return (this->contentType().indexOf("multipart/form-data") >= 0);
}

esp_err_t PsychicHttpServerRequest::redirect(const char *url)
{
  PsychicHttpServerResponse response(this);
  response.setCode(301);
  response.addHeader("Location", url);
  
  return response.send();
}


bool PsychicHttpServerRequest::hasCookie(const char *key)
{
  char cookie[MAX_COOKIE_SIZE];
  size_t cookieSize = MAX_COOKIE_SIZE;
  esp_err_t err = httpd_req_get_cookie_val(this->_req, key, cookie, &cookieSize);

  //did we get anything?
  if (err == ESP_OK)
    return true;
  else if (err == ESP_ERR_HTTPD_RESULT_TRUNC)
    ESP_LOGE(PH_TAG, "cookie too large (%d bytes).\n", cookieSize);

  return false;
}

const String PsychicHttpServerRequest::getCookie(const char *key)
{
  char cookie[MAX_COOKIE_SIZE];
  size_t cookieSize = MAX_COOKIE_SIZE;
  esp_err_t err = httpd_req_get_cookie_val(this->_req, key, cookie, &cookieSize);

  //did we get anything?
  if (err == ESP_OK)
    return String(cookie);
  else
    return "";
}

bool PsychicHttpServerRequest::hasParam(const char *key)
{
  String query = this->queryString();
  char value[query.length()];
  esp_err_t err = httpd_query_key_value(query.c_str(), key, value, query.length());

  //did we get anything?
  if (err == ESP_OK || err == ESP_ERR_HTTPD_RESULT_TRUNC)
    return true;
  else
    return false;
}

const String PsychicHttpServerRequest::getParam(const char *key)
{
  esp_err_t err;

  //TODO: we need urldecoding here.
  //POST parameters are in the body:
  if (this->method() == HTTP_POST && this->contentType() == "application/x-www-form-urlencoded")
  {
    String body = this->body();
    char pvalue[body.length()];
    err = httpd_query_key_value(body.c_str(), key, pvalue, body.length());
    if (err == ESP_OK)
      return urlDecode(pvalue);
  }
  
  //okay, look for it in our query string.
  String query = this->queryString();
  char gvalue[query.length()];
  err = httpd_query_key_value(query.c_str(), key, gvalue, query.length());
  if (err == ESP_OK)
    return urlDecode(gvalue);

  return "";
}

bool PsychicHttpServerRequest::hasSessionKey(const String& key)
{
  return this->_session->find(key) != this->_session->end();
}

const String PsychicHttpServerRequest::getSessionKey(const String& key)
{
  auto it = this->_session->find(key);
  if (it != this->_session->end())
    return it->second;
  else
    return "";
}

void PsychicHttpServerRequest::setSessionKey(const String& key, const String& value)
{
  this->_session->insert(std::pair<String, String>(key, value));
}

static const String md5str(const String &in){
  MD5Builder md5 = MD5Builder();
  md5.begin();
  md5.add(in);
  md5.calculate();
  return md5.toString();
}

bool PsychicHttpServerRequest::authenticate(const char * username, const char * password)
{
  if(hasHeader("Authorization"))
  {
    String authReq = header("Authorization");
    if(authReq.startsWith("Basic")){
      authReq = authReq.substring(6);
      authReq.trim();
      char toencodeLen = strlen(username)+strlen(password)+1;
      char *toencode = new char[toencodeLen + 1];
      if(toencode == NULL){
        authReq = "";
        return false;
      }
      char *encoded = new char[base64_encode_expected_len(toencodeLen)+1];
      if(encoded == NULL){
        authReq = "";
        delete[] toencode;
        return false;
      }
      sprintf(toencode, "%s:%s", username, password);
      if(base64_encode_chars(toencode, toencodeLen, encoded) > 0 && authReq.equalsConstantTime(encoded)) {
        authReq = "";
        delete[] toencode;
        delete[] encoded;
        return true;
      }
      delete[] toencode;
      delete[] encoded;
    }
    else if(authReq.startsWith(F("Digest")))
    {
      authReq = authReq.substring(7);
      String _username = _extractParam(authReq,F("username=\""),'\"');
      if(!_username.length() || _username != String(username)) {
        authReq = "";
        return false;
      }
      // extracting required parameters for RFC 2069 simpler Digest
      String _realm    = _extractParam(authReq, F("realm=\""),'\"');
      String _nonce    = _extractParam(authReq, F("nonce=\""),'\"');
      String _uri      = _extractParam(authReq, F("uri=\""),'\"');
      String _resp = _extractParam(authReq, F("response=\""),'\"');
      String _opaque   = _extractParam(authReq, F("opaque=\""),'\"');

      if((!_realm.length()) || (!_nonce.length()) || (!_uri.length()) || (!_resp.length()) || (!_opaque.length())) {
        authReq = "";
        return false;
      }
      if((_opaque != this->getSessionKey("opaque")) || (_nonce != this->getSessionKey("nonce")) || (_realm != this->getSessionKey("realm")))
      {
        authReq = "";
        return false;
      }
      // parameters for the RFC 2617 newer Digest
      String _nc,_cnonce;
      if(authReq.indexOf("qop=auth") != -1 || authReq.indexOf("qop=\"auth\"") != -1) {
        _nc = _extractParam(authReq, F("nc="), ',');
        _cnonce = _extractParam(authReq, F("cnonce=\""),'\"');
      }
      String _H1 = md5str(String(username) + ':' + _realm + ':' + String(password));
      ESP_LOGD(PH_TAG, "Hash of user:realm:pass=%s", _H1);
      String _H2 = "";
      if(_method == HTTP_GET){
          _H2 = md5str(String(F("GET:")) + _uri);
      }else if(_method == HTTP_POST){
          _H2 = md5str(String(F("POST:")) + _uri);
      }else if(_method == HTTP_PUT){
          _H2 = md5str(String(F("PUT:")) + _uri);
      }else if(_method == HTTP_DELETE){
          _H2 = md5str(String(F("DELETE:")) + _uri);
      }else{
          _H2 = md5str(String(F("GET:")) + _uri);
      }
      ESP_LOGD(PH_TAG, "Hash of GET:uri=%s", _H2);
      String _responsecheck = "";
      if(authReq.indexOf("qop=auth") != -1 || authReq.indexOf("qop=\"auth\"") != -1) {
          _responsecheck = md5str(_H1 + ':' + _nonce + ':' + _nc + ':' + _cnonce + F(":auth:") + _H2);
      } else {
          _responsecheck = md5str(_H1 + ':' + _nonce + ':' + _H2);
      }
      ESP_LOGD(PH_TAG, "The Proper response=%s", _responsecheck);
      if(_resp == _responsecheck){
        authReq = "";
        return true;
      }
    }
    authReq = "";
  }
  return false;
}

const String PsychicHttpServerRequest::_extractParam(const String& authReq, const String& param, const char delimit)
{
  int _begin = authReq.indexOf(param);
  if (_begin == -1)
    return "";
  return authReq.substring(_begin+param.length(),authReq.indexOf(delimit,_begin+param.length()));
}

const String PsychicHttpServerRequest::_getRandomHexString()
{
  char buffer[33];  // buffer to hold 32 Hex Digit + /0
  int i;
  for(i = 0; i < 4; i++) {
    sprintf (buffer + (i*8), "%08lx", esp_random());
  }
  return String(buffer);
}

esp_err_t PsychicHttpServerRequest::requestAuthentication(HTTPAuthMethod mode, const char* realm, const String& authFailMsg)
{
  //what is thy realm, sire?
  if(realm == NULL)
    this->setSessionKey("realm", "Login Required");
  else
    this->setSessionKey("realm", realm);

  PsychicHttpServerResponse response(this);
  String authStr;

  //what kind of auth?
  if(mode == BASIC_AUTH)
  {
    authStr = "Basic realm=\"" + this->getSessionKey("realm") + "\"";
    response.addHeader("WWW-Authenticate", authStr.c_str());
  }
  else
  {
    this->setSessionKey("nonce", _getRandomHexString());
    this->setSessionKey("opaque", _getRandomHexString());

    authStr = "Digest realm=\"" + this->getSessionKey("realm") + "\", qop=\"auth\", nonce=\"" + this->getSessionKey("nonce") + "\", opaque=\"" + this->getSessionKey("opaque") + "\"";

    response.addHeader("WWW-Authenticate", authStr.c_str());
  }

  response.setCode(401);
  response.setContentType("text/html");
  response.setContent(authFailMsg.c_str());
  return response.send();
}

esp_err_t PsychicHttpServerRequest::reply(int code)
{
  PsychicHttpServerResponse response(this);

  response.setCode(code);
  response.setContentType("text/plain");
  response.setContent(http_status_reason(code));

  return response.send();
}

esp_err_t PsychicHttpServerRequest::reply(const char *content)
{
  PsychicHttpServerResponse response(this);

  response.setCode(200);
  response.setContentType("text/plain");
  response.setContent(content);

  return response.send();
}

// esp_err_t PsychicHttpServerRequest::reply(int code, const char *content)
// {
//   PsychicHttpServerResponse response(this);

//   response.setCode(code);
//   response.setContentType("text/plain");
//   response.setContent(content);

//   return response.send();
// }

esp_err_t PsychicHttpServerRequest::reply(int code, const char *contentType, const char *content)
{
  PsychicHttpServerResponse response(this);

  response.setCode(code);
  response.setContentType(contentType);
  response.setContent(content);

  return response.send();
}