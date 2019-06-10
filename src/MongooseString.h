#ifndef MongooseString_h
#define MongooseString_h

#ifdef ARDUINO
#include "Arduino.h"
#endif

#include "mongoose.h"

class MongooseString
{
  private:
    mg_str _string;

    // use a function pointer to allow for "if (s)" without the
    // complications of an operator bool(). for more information, see:
    // http://www.artima.com/cppsource/safebool.html
    typedef void (MongooseString::*StringIfHelperType)() const;
    void StringIfHelper() const {
    }

  public:
    MongooseString() : 
      _string(MG_NULL_STR)
    {
    }
    MongooseString(const mg_str *string) :
      _string(MG_NULL_STR)
    {
      if(string) {
        _string = *string; 
      }
    }
    MongooseString(const mg_str string) :
      _string(string) {
    }

    operator mg_str ()
    {
      return _string;
    }

    operator mg_str *()
    {
      return &_string;
    }

    operator const char *() const
    {
      return _string.p;
    }

    // use a function pointer to allow for "if (s)" without the
    // complications of an operator bool(). for more information, see:
    // http://www.artima.com/cppsource/safebool.html
    operator StringIfHelperType() const
    {
      return NULL != _string.p ? &MongooseString::StringIfHelper : 0;
    }
    
#ifdef ARDUINO
    operator String()
    {
      mg_str copy = mg_strdup_nul(_string);
      String ret = String(copy.p);
      mg_strfree(&copy);

      return ret;
    }
#endif

    MongooseString & operator = (const char *cstr) {
      _string = mg_mk_str(cstr);
      return *this;
    }
    MongooseString & operator = (const mg_str *rhs) {
      _string.p = rhs ? rhs->p : NULL;
      _string.len = rhs ? rhs->len : 0;
      return *this;
    }
    MongooseString & operator = (const mg_str &rhs) {
      _string = rhs;
      return *this;
    }
//    MongooseString & operator = (const String &rhs);
//    MongooseString & operator = (const __FlashStringHelper *str);

    void get(const char * &p, size_t &len) {
      p = _string.p;
      len = _string.len;
    }

    size_t length() {
      return _string.len;
    }

};

#endif // MongooseString_h