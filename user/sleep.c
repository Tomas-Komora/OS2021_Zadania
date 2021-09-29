#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  //ak argc sa nerovna 2 vyhodi chybu inak ide program dalej
  if(argc != 2) {
    fprintf(2, "usage: sleep <ticks\n");
    exit(-1);
  }
  //ymeni pocet na int
  int ticks = atoi(argv[1]);
  sleep(ticks);

  exit(0);
}

