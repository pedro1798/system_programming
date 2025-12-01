#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define CHILD_MESS "I want a cookie\n"
#define PAR_MESS "testing..\n"
#define opps(m, x) {perror(m); exit(x);}

int main() {
    int pipefd[2];
    int len;
    char buf[BUFSIZ];
    int read_len;

    pipe(pipefd);
    switch(fork()) {
        case -1:
            opps("cannot fork", 2);
        case 0:
            len = strlen(CHILD_MESS);
            while(1) {
                if (write(pipefd[1], CHILD_MESS, len) != len) 
                    opps("write", 3);
                sleep(5);
            } 
        default:
            len = strlen(PAR_MESS);
            while(1) {
                if(write(pipefd[1], PAR_MESS, len) != len) 
                    opps("write", 4);
                sleep(1);
                read_len = read(pipefd[0], buf, BUFSIZ);
                if(read_len <= 0) break;
                write(1, buf, read_len);
            }
    }
}
