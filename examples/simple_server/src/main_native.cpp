#ifndef ARDUINO

#include "simple_server.h"

int main()
{
  server_setup();

  for(;;) {
    server_loop();
  }
}

#endif
