#ifndef PsychicRequest_h
#define PsychicRequest_h

#include "PsychicCore.h"
#include "PsychicHttpServer.h"
#include "PsychicClient.h"
#include "PsychicWebParameter.h"

typedef std::map<String, String> SessionData;

enum Disposition { NONE, INLINE, ATTACHMENT, FORM_DATA};

struct ContentDisposition {
  Disposition disposition;
  String filename;
  String name;
};

class PsychicRequest {
  friend PsychicHttpServer;

  protected:
    PsychicHttpServer *_server;
    httpd_req_t *_req;
    SessionData *_session;
    PsychicClient *_client;

    http_method _method;
    String _uri;
    String _query;
    String _body;

    std::list<PsychicWebParameter*> _params;

    void _addParams(const String& params);
    void _parseGETParams();
    void _parsePOSTParams();

    const String _extractParam(const String& authReq, const String& param, const char delimit);
    const String _getRandomHexString();

  public:
    PsychicRequest(PsychicHttpServer *server, httpd_req_t *req);
    virtual ~PsychicRequest();

    void *_tempObject;

    PsychicHttpServer * server();
    httpd_req_t * request();
    virtual PsychicClient * client();

    bool isMultipart();
    esp_err_t loadBody();

    const String header(const char *name);
    bool hasHeader(const char *name);

    static void freeSession(void *ctx);
    bool hasSessionKey(const String& key);
    const String getSessionKey(const String& key);
    void setSessionKey(const String& key, const String& value);

    bool hasCookie(const char * key);
    const String getCookie(const char * key);

    http_method method();
    const String methodStr();
    const String& uri();
    const String& url() { return uri(); }
    const String host();
    const String contentType();
    size_t contentLength();
    const String& body();
    const ContentDisposition getContentDisposition();
    const String queryString();

    void loadParams();
    PsychicWebParameter * addParam(PsychicWebParameter *param);
    PsychicWebParameter * addParam(const String &name, const String &value, bool decode = true);
    bool hasParam(const char *key);
    PsychicWebParameter * getParam(const char *name);

    const String getFilename();

    bool authenticate(const char * username, const char * password);
    esp_err_t requestAuthentication(HTTPAuthMethod mode, const char* realm, const char* authFailMsg);

    esp_err_t redirect(const char *url);
    esp_err_t reply(int code);
    esp_err_t reply(const char *content);
    esp_err_t reply(int code, const char *contentType, const char *content);
};

#endif // PsychicRequest_h