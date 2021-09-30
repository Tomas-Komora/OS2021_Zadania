#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  //if argc is not equal 2 write error or program continue
  if(argc != 2) {
    fprintf(2, "usage: sleep <ticks\n");
    exit(-1);
  }
  //change to int
  int ticks = atoi(argv[1]);
  sleep(ticks);

  exit(0);
}

