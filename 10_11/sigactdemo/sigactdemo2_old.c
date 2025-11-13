#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <unistd.h>
#include <stdlib.h>

#define INPUTLEN 100

void inthandler(int s);

int main() {
    struct sigaction newhandler;
    sigset_t blocked;
    char x[INPUTLEN];

    newhandler.sa_handler = inthandler;
    newhandler.sa_flags = SA_RESETHAND | SA_RESTART | SA_SIGINFO;

    sigemptyset(&blocked);
    sigaddset(&blocked, SIGQUIT);
    newhandler.sa_mask = blocked;

    if (sigaction(SIGINT, &newhandler, NULL) == -1) {
        perror("sigaction");
    } else {
        while(1) {
            fgets(x, INPUTLEN, stdin);
            printf("input: %s", x);
        }
    }
}

void inthandler(int s) {
    printf("Called with signal %d\n", s);
    sleep(s);
    printf("done handling signal %d\n", s);
}
