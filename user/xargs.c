#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
  int i = 0;
  char c;
  char buf[512];
  if(argc < 2){
    fprintf(2, "usage: xargs missing arguments");
    exit(-1);
  }
  char *args[MAXARG];
  // Copy arguments from argv to args without 'xargs'.
  for(int x = 1; x < argc; x++)
  {
    args[x- 1]=argv[x];
  }
  // Till reach end of file.
  while (read(0, &c, 1))
  {
    // Write every read char to buf if it not special character '\n'.
    if(c != '\n'){
      buf[i] = c;
      i++;
      continue;
    }
    // If it is special character end string with '\0'
    // and write buf to args.
    if(c == '\n'){
      buf[i] = '\0';
      i = 0;
      int x = argc;
      args[x - 1] = buf;
      // Child process.
      if(fork() == 0){
        exec(args[0], args);
        exit(0);
      // Waiting for child process to finish.
      } else {
          wait(0);
      }
    }
  }
  exit(0);
}
