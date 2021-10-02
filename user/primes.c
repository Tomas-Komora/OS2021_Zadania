#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define Read 0
#define Write 1

void
primes(int *leftPipe)
{
    int p;
    int rightPipe[2];
    read(leftPipe[Read], &p, sizeof(p));
    // Exiting condition.
    if(p == 36) {
        exit(0);
    }
    printf("prime %d\n", p);

    pipe(rightPipe);

    if(fork() != 0) {
        close(rightPipe[Read]);
        int buf;
        while(read(leftPipe[Read], &buf, sizeof(buf)) && buf != 36) {
            //If it prime write it to next pipe.
            if(buf % p != 0) {
                write(rightPipe[Write], &buf, sizeof(buf));
            }
        }
        write(rightPipe[Write], &buf, sizeof(buf));
        close(leftPipe[Read]);
        // Waiting for child process.
        wait(0);
        exit(0);
    } else {
        // Close left pipe for reading and right for writing and recall primes.
        close(leftPipe[Read]);
        close(rightPipe[Write]);
        primes(rightPipe);
    }
}

int
main(int argc, char *argv[])
{
    int input_pipe[2];
    // Create pipe.
    pipe(input_pipe);

    if(fork() == 0) {
        // Child process.
        // Close pipe for writing.
        close(input_pipe[1]);
        primes(input_pipe);
        exit(0);
    } else {
        // Parent process.
        // Close pipe for reading.
        close(input_pipe[0]);
        int i;
        for(i=2;i<=36;i++){
            write(input_pipe[1], &i, sizeof(i));
        }
    }
    wait(0);
    exit(0);
}
