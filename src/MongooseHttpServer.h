/*
  Asynchronous WebServer library for Espressif MCUs

  Copyright (c) 2016 Hristo Gochkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef MongooseHttpServer_h
#define MongooseHttpServer_h

#include "Arduino.h"

#include <functional>

class MongooseHttpServer;
class MongooseHttpServerRequest;
class MongooseHttpServerResponse;

typedef enum {
  HTTP_GET     = 0b00000001,
  HTTP_POST    = 0b00000010,
  HTTP_DELETE  = 0b00000100,
  HTTP_PUT     = 0b00001000,
  HTTP_PATCH   = 0b00010000,
  HTTP_HEAD    = 0b00100000,
  HTTP_OPTIONS = 0b01000000,
  HTTP_ANY     = 0b01111111,
} HttpRequestMethod;

typedef uint8_t HttpRequestMethodComposite;
//typedef std::function<void(void)> ArDisconnectHandler;

/*
 * REQUEST :: Each incoming Client is wrapped inside a Request and both live together until disconnect
 * */

//typedef enum { RCT_NOT_USED = -1, RCT_DEFAULT = 0, RCT_HTTP, RCT_WS, RCT_EVENT, RCT_MAX } RequestedConnectionType;

//typedef std::function<size_t(uint8_t*, size_t, size_t)> AwsResponseFiller;
//typedef std::function<String(const String&)> AwsTemplateProcessor;

class MongooseHttpServerRequest {
//  using File = fs::File;
//  using FS = fs::FS;
//  friend class MongooseHttpServer;
  private:
//    AsyncClient* _client;
//    MongooseHttpServer* _server;
//    AsyncWebHandler* _handler;
//    MongooseHttpServerResponse* _response;
//    StringArray _interestingHeaders;
//    ArDisconnectHandler _onDisconnectfn;

//    String _temp;
//    uint8_t _parseState;

//    uint8_t _version;
//    HttpRequestMethodComposite _method;
//    String _url;
//    String _host;
//    String _contentType;
//    String _boundary;
//    String _authorization;
//    RequestedConnectionType _reqconntype;
//    void _removeNotInterestingHeaders();
//    bool _isDigest;
//    bool _isMultipart;
//    bool _isPlainPost;
//    bool _expectingContinue;
//    size_t _contentLength;
//    size_t _parsedLength;

//    LinkedList<AsyncWebHeader *> _headers;
//    LinkedList<AsyncWebParameter *> _params;

//    uint8_t _multiParseState;
//    uint8_t _boundaryPosition;
//    size_t _itemStartIndex;
//    size_t _itemSize;
//    String _itemName;
//    String _itemFilename;
//    String _itemType;
//    String _itemValue;
//    uint8_t *_itemBuffer;
//    size_t _itemBufferIndex;
//    bool _itemIsFile;

//    void _onPoll();
//    void _onAck(size_t len, uint32_t time);
//    void _onError(int8_t error);
//    void _onTimeout(uint32_t time);
//    void _onDisconnect();
//    void _onData(void *buf, size_t len);

//    void _addParam(AsyncWebParameter*);

//    bool _parseReqHead();
//    bool _parseReqHeader();
//    void _parseLine();
//    void _parsePlainPostChar(uint8_t data);
//    void _parseMultipartPostByte(uint8_t data, bool last);
//    void _addGetParams(const String& params);

//    void _handleUploadStart();
//    void _handleUploadByte(uint8_t data, bool last);
//    void _handleUploadEnd();

  public:
//    File _tempFile;
//    void *_tempObject;

    MongooseHttpServerRequest(MongooseHttpServer*);
    ~MongooseHttpServerRequest();

//    uint8_t version() const { return _version; }
//    HttpRequestMethodComposite method() const { return _method; }
//    const String& url() const { return _url; }
//    const String& host() const { return _host; }
//    const String& contentType() const { return _contentType; }
//    size_t contentLength() const { return _contentLength; }
//    bool multipart() const { return _isMultipart; }
//    const char * methodToString() const;
//    const char * requestedConnTypeToString() const;
//    RequestedConnectionType requestedConnType() const { return _reqconntype; }
//    bool isExpectedRequestedConnType(RequestedConnectionType erct1, RequestedConnectionType erct2 = RCT_NOT_USED, RequestedConnectionType erct3 = RCT_NOT_USED);
//    void onDisconnect (ArDisconnectHandler fn);

    //hash is the string representation of:
    // base64(user:pass) for basic or
    // user:realm:md5(user:realm:pass) for digest
//    bool authenticate(const char * hash);
//    bool authenticate(const char * username, const char * password, const char * realm = NULL, bool passwordIsHash = false);
//    void requestAuthentication(const char * realm = NULL, bool isDigest = true);

//    void setHandler(AsyncWebHandler *handler){ _handler = handler; }
//    void addInterestingHeader(const String& name);

    void redirect(const String& url);

//    void send(MongooseHttpServerResponse *response);
    void send(int code, const String& contentType=String(), const String& content=String());
//    void send(FS &fs, const String& path, const String& contentType=String(), bool download=false, AwsTemplateProcessor callback=nullptr);
//    void send(File content, const String& path, const String& contentType=String(), bool download=false, AwsTemplateProcessor callback=nullptr);
//    void send(Stream &stream, const String& contentType, size_t len, AwsTemplateProcessor callback=nullptr);
//    void send(const String& contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback=nullptr);
//    void sendChunked(const String& contentType, AwsResponseFiller callback, AwsTemplateProcessor templateCallback=nullptr);
//    void send_P(int code, const String& contentType, const uint8_t * content, size_t len, AwsTemplateProcessor callback=nullptr);
//    void send_P(int code, const String& contentType, PGM_P content, AwsTemplateProcessor callback=nullptr);

//    MongooseHttpServerResponse *beginResponse(int code, const String& contentType=String(), const String& content=String());
//    MongooseHttpServerResponse *beginResponse(FS &fs, const String& path, const String& contentType=String(), bool download=false, AwsTemplateProcessor callback=nullptr);
//    MongooseHttpServerResponse *beginResponse(File content, const String& path, const String& contentType=String(), bool download=false, AwsTemplateProcessor callback=nullptr);
//    MongooseHttpServerResponse *beginResponse(Stream &stream, const String& contentType, size_t len, AwsTemplateProcessor callback=nullptr);
//    MongooseHttpServerResponse *beginResponse(const String& contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback=nullptr);
//    MongooseHttpServerResponse *beginChunkedResponse(const String& contentType, AwsResponseFiller callback, AwsTemplateProcessor templateCallback=nullptr);
//    AsyncResponseStream *beginResponseStream(const String& contentType, size_t bufferSize=1460);
//    MongooseHttpServerResponse *beginResponse_P(int code, const String& contentType, const uint8_t * content, size_t len, AwsTemplateProcessor callback=nullptr);
//    MongooseHttpServerResponse *beginResponse_P(int code, const String& contentType, PGM_P content, AwsTemplateProcessor callback=nullptr);

//    size_t headers() const;                     // get header count
//    bool hasHeader(const String& name) const;   // check if header exists
//    bool hasHeader(const __FlashStringHelper * data) const;   // check if header exists

//    AsyncWebHeader* getHeader(const String& name) const;
//    AsyncWebHeader* getHeader(const __FlashStringHelper * data) const;
//    AsyncWebHeader* getHeader(size_t num) const;

//    size_t params() const;                      // get arguments count
    bool hasParam(const char *name) const;
    bool hasParam(const String& name) const;
    bool hasParam(const __FlashStringHelper * data) const;

    bool getParam(const char *name, char *dst, size_t dst_len) const;
    bool getParam(const String& name, char *dst, size_t dst_len) const;
    bool getParam(const __FlashStringHelper * data, char *dst, size_t dst_len) const; 

    String getParam(const char *name) const;
    String getParam(const String& name) const;
    String getParam(const __FlashStringHelper * data) const; 

//    String getParam(size_t num) const;

//    size_t args() const { return params(); }     // get arguments count
//    const String& arg(const String& name) const; // get request argument value by name
//    const String& arg(const __FlashStringHelper * data) const; // get request argument value by F(name)    
//    const String& arg(size_t i) const;           // get request argument value by number
//    const String& argName(size_t i) const;       // get request argument name by number
//    bool hasArg(const char* name) const;         // check if argument exists
//    bool hasArg(const __FlashStringHelper * data) const;         // check if F(argument) exists

//    const String& header(const char* name) const;// get request header value by name
//    const String& header(const __FlashStringHelper * data) const;// get request header value by F(name)    
//    const String& header(size_t i) const;        // get request header value by number
//    const String& headerName(size_t i) const;    // get request header name by number
//    String urlDecode(const String& text) const;
};

/*
 * RESPONSE :: One instance is created for each Request (attached by the Handler)
 * */

typedef enum {
  RESPONSE_SETUP, RESPONSE_HEADERS, RESPONSE_CONTENT, RESPONSE_WAIT_ACK, RESPONSE_END, RESPONSE_FAILED
} WebResponseState;

class MongooseHttpServerResponse {
  protected:
//    int _code;
//    LinkedList<AsyncWebHeader *> _headers;
//    String _contentType;
//    size_t _contentLength;
//    bool _sendContentLength;
//    bool _chunked;
//    size_t _headLength;
//    size_t _sentLength;
//    size_t _ackedLength;
//    size_t _writtenLength;
//    WebResponseState _state;
//    const char* _responseCodeToString(int code);

  public:
//    MongooseHttpServerResponse();
//    virtual ~MongooseHttpServerResponse();
//    virtual void setCode(int code);
//    virtual void setContentLength(size_t len);
//    virtual void setContentType(const String& type);
//    virtual void addHeader(const String& name, const String& value);
//    virtual String _assembleHead(uint8_t version);
//    virtual bool _started() const;
//    virtual bool _finished() const;
//    virtual bool _failed() const;
//    virtual bool _sourceValid() const;
//    virtual void _respond(MongooseHttpServerRequest *request);
//    virtual size_t _ack(MongooseHttpServerRequest *request, size_t len, uint32_t time);
};

/*
 * SERVER :: One instance
 * */

typedef std::function<void(MongooseHttpServerRequest *request)> ArRequestHandlerFunction;
typedef std::function<void(MongooseHttpServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final)> ArUploadHandlerFunction;
typedef std::function<void(MongooseHttpServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)> ArBodyHandlerFunction;

class MongooseHttpServer 
{
  protected:
    struct mg_connection *nc;

    static void eventHandler(struct mg_connection *nc, int ev, void *p, void *u);
    void eventHandler(struct mg_connection *nc, int ev, void *p); 

  public:
    MongooseHttpServer();
    ~MongooseHttpServer();

    void begin(uint16_t port);

#if MG_ENABLE_SSL
    void begin(uint16_t port, const char *cert, const char *private_key_file, const char *password);
#endif

//    AsyncWebRewrite& addRewrite(AsyncWebRewrite* rewrite);
//    bool removeRewrite(AsyncWebRewrite* rewrite);
//    AsyncWebRewrite& rewrite(const char* from, const char* to);

//    AsyncWebHandler& addHandler(AsyncWebHandler* handler);
//    bool removeHandler(AsyncWebHandler* handler);

    void on(const char* uri, ArRequestHandlerFunction onRequest);
    void on(const char* uri, HttpRequestMethodComposite method, ArRequestHandlerFunction onRequest);
//    void on(const char* uri, HttpRequestMethodComposite method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload);
//    void on(const char* uri, HttpRequestMethodComposite method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload, ArBodyHandlerFunction onBody);

//    AsyncStaticWebHandler& serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_control = NULL);

    void onNotFound(ArRequestHandlerFunction fn);  //called when handler is not assigned
//    void onFileUpload(ArUploadHandlerFunction fn); //handle file uploads
//    void onRequestBody(ArBodyHandlerFunction fn); //handle posts with plain body content (JSON often transmitted this way as a request)

    void reset(); //remove all writers and handlers, with onNotFound/onFileUpload/onRequestBody 

//    void _handleDisconnect(MongooseHttpServerRequest *request);
//    void _attachHandler(MongooseHttpServerRequest *request);
//    void _rewriteRequest(MongooseHttpServerRequest *request);
};

#endif /* _MongooseHttpServer_H_ */
