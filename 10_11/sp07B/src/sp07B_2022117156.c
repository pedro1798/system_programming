#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

int signal_handler(int signal);

int main(int argc, char *argv[]) {
    if (argc > 2) {
        fprintf(stderr, "Too many arguments!");
        exit(-1);
    }
    struct sigaction sa;
    sa.sa_handler = signal_handler();
    sigemptyset(&sa.sa_mask); // 처리 중 다른 시그널 블록 X
    // sa.sa_flags = 0;
    sigcation(SIGINT, &sa, NULL);
}

int signal_handler(int signal) {
    fprintf(stdout, "user interrupt");
    exit(0);
}
