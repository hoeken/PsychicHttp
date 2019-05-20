
#include "MongooseCore.h"

MongooseCore::MongooseCore()
{
}

void MongooseCore::begin() 
{
  mg_mgr_init(&mgr, this);
}

void MongooseCore::end() 
{
  mg_mgr_free(&mgr);
}

void MongooseCore::poll(int timeout_ms) 
{
  mg_mgr_poll(&mgr, timeout_ms);
}

struct mg_mgr *MongooseCore::getMgr()
{
  return &mgr;
}

MongooseCore Mongoose;
