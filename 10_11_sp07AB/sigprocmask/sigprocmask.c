#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include<sys/signal.h>
#include <stdlib.h>

int main() {
    int i = 5;
    sigset_t sigs, prevsigs;

    sigemptyset(&sigs);
    sigaddset(&sigs, SIGINT);

    printf("Critical setction in \n");
    sigprocmask(SIG_BLOCK, &sigs, &prevsigs);
    while (i--) {
        sleep(1);
    }
    sigprocmask(SIG_SETMASK, &prevsigs, NULL);
    printf("Critical section out \n");
    while (i--) {
        sleep(1);
    }
}
