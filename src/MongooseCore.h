#ifndef MongooseCore_h
#define MongooseCore_h

#include "mongoose.h"

class MongooseCore
{
  private:
    struct mg_mgr mgr;

  public:
    MongooseCore();
    void begin();
    void end();
    void poll(int timeout_ms);

    struct mg_mgr *getMgr();
};

extern MongooseCore Mongoose;

#endif // MongooseCore_h
