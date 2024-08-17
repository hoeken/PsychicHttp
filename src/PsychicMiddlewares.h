#pragma once

#include "PsychicMiddleware.h"

class AuthcMiddleware : public PsychicMiddleware
{
  protected:
    String _username;
    String _password;
    HTTPAuthMethod _method;
    String _realm;
    String _authFailMsg;
    bool _authc = false;

  public:
    AuthcMiddleware(const char* username, const char* password, HTTPAuthMethod method = BASIC_AUTH, const char* realm = "", const char* authFailMsg = "")
        : _username(username), _password(password), _method(method), _realm(realm), _authFailMsg(authFailMsg), _authc(!_username.isEmpty() && !_password.isEmpty()) {}

    esp_err_t run(PsychicMiddlewareCallback next, PsychicRequest* request, PsychicResponse* response) override
    {
      if (_authc && !request->authenticate(_username.c_str(), _password.c_str())) {
        return request->requestAuthentication(_method, _realm.c_str(), _authFailMsg.c_str());
      }
      return next(request, response);
    }
};

class PermissiveCorsMiddleware : public PsychicMiddleware
{
  public:
    esp_err_t run(PsychicMiddlewareCallback next, PsychicRequest* request, PsychicResponse* response) override
    {
      // gives the chance to other most important middleware to run first
      esp_err_t ret = next(request, response);
      // add CORS headers
      if (ret == ESP_OK && request->hasHeader("Origin")) {
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, Accept");
        response->addHeader("Access-Control-Max-Age", "86400");
      }
      return ret;
    }
};
