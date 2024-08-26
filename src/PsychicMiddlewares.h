#ifndef PsychicMiddlewares_h
#define PsychicMiddlewares_h

#include "PsychicMiddleware.h"

#include <Stream.h>
#include <http_status.h>

// curl-like logging middleware
class LoggingMiddleware : public PsychicMiddleware
{
  private:
    Stream* _out;

  public:
    explicit LoggingMiddleware(Stream& out) : _out(&out) {}

    esp_err_t run(PsychicRequest* request, PsychicResponse* response, PsychicMiddlewareCallback next) override
    {
      _out->print("* Connection from ");
      _out->print(request->client()->remoteIP().toString());
      _out->print(":");
      _out->println(request->client()->remotePort());

      _out->print("< ");
      _out->print(request->methodStr());
      _out->print(" ");
      _out->print(request->uri());
      _out->print(" ");
      _out->println(request->version());

      // TODO: int n = request->headerCount();
      //  for (int i = 0; i < n; i++) {
      //    String v = server.header(i);
      //    if (!v.isEmpty()) {
      //      // because these 2 are always there, eventually empty: "Authorization", "If-None-Match"
      //      _out->print("< ");
      //      _out->print(server.headerName(i));
      //      _out->print(": ");
      //      _out->println(server.header(i));
      //    }
      //  }

      _out->println("<");

      esp_err_t ret = next(request, response);

      if (ret != HTTPD_404_NOT_FOUND) {
        _out->println("* Processed!");

        _out->print("> ");
        _out->print(response->version());
        _out->print(" ");
        _out->print(response->getCode());
        _out->print(" ");
        _out->println(http_status_reason(response->getCode()));

        // iterate over response->headers()
        std::list<HTTPHeader>::iterator it = response->headers().begin();
        while (it != response->headers().end()) {
          HTTPHeader h = *it;
          _out->print("> ");
          _out->print(h.field);
          _out->print(": ");
          _out->println(h.value);
          it++;
        }

        _out->println(">");

      } else {
        _out->println("* Not processed!");
      }

      return ret;
    }
};

class AuthenticationMiddleware : public PsychicMiddleware
{
  private:
    String _username;
    String _password;
    HTTPAuthMethod _method;
    String _realm;
    String _authFailMsg;
    bool _authc = false;

  public:
    AuthenticationMiddleware(const char* username, const char* password, HTTPAuthMethod method = BASIC_AUTH, const char* realm = "", const char* authFailMsg = "")
        : _username(username), _password(password), _method(method), _realm(realm), _authFailMsg(authFailMsg), _authc(!_username.isEmpty() && !_password.isEmpty()) {}

    esp_err_t run(PsychicRequest* request, PsychicResponse* response, PsychicMiddlewareCallback next) override
    {
      if (_authc && !request->authenticate(_username.c_str(), _password.c_str())) {
        return request->requestAuthentication(_method, _realm.c_str(), _authFailMsg.c_str());
      }
      return next(request, response);
    }
};

class CorsMiddleware : public PsychicMiddleware
{
  private:
    String _origin = "*";
    String _methods = "*";
    String _headers = "*";
    bool _credentials = true;
    uint32_t _maxAge = 86400;

  public:
    CorsMiddleware& origin(const char* origin)
    {
      _origin = origin;
      return *this;
    }

    CorsMiddleware& methods(const char* methods)
    {
      _methods = methods;
      return *this;
    }

    CorsMiddleware& headers(const char* headers)
    {
      _headers = headers;
      return *this;
    }

    CorsMiddleware& allowCredentials(bool credentials)
    {
      _credentials = credentials;
      return *this;
    }

    CorsMiddleware& maxAge(uint32_t seconds)
    {
      _maxAge = seconds;
      return *this;
    }

    esp_err_t run(PsychicRequest* request, PsychicResponse* response, PsychicMiddlewareCallback next) override
    {
      if (request->method() == HTTP_OPTIONS && request->hasHeader("Origin")) {
        response->addHeader("Access-Control-Allow-Origin", _origin.c_str());
        response->addHeader("Access-Control-Allow-Methods", _methods.c_str());
        response->addHeader("Access-Control-Allow-Headers", _headers.c_str());
        response->addHeader("Access-Control-Allow-Credentials", _credentials ? "true" : "false");
        response->addHeader("Access-Control-Max-Age", String(_maxAge).c_str());
        return response->send(200);
      }
      return next(request, response);
    }
};

#endif
