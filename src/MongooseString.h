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
    
#ifdef ARDUINO
    operator String()
    {
      mg_str copy = mg_strdup_nul(_string);
      String ret = String(copy.p);
      mg_strfree(&copy);

      return ret;
    }
#endif

    void get(const char * &p, size_t &len) {
      p = _string.p;
      len = _string.len;
    }

    size_t length() {
      return _string.len;
    }

};

#endif // MongooseString_h