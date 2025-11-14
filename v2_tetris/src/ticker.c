#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#define FRAME 100000

void sigalrm_handler(int sig); 

void set_ticker() {
    sigset_t block_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGALRM);

    if (sigprocmask(SIG_BLOCK, &block_set, NULL) < 0) {
        perror("sigprocmask");
        exit(1);
    }

    struct sigaction sa_sigalrm;
    sa_sigalrm.sa_handler = sigalrm_handler;
    sigemptyset(&sa_sigalrm.sa_mask);
    sa_sigalrm.sa_flags = 0;
    sigaction(SIGALRM, &sa_sigalrm, NULL);

    /* 타이머 설정 */
    struct itimerval timer = {{0, FRAME}, {0, FRAME}};
    setitimer(ITIMER_REAL, &timer, NULL);

    if (sigprocmask(SIG_UNBLOCK, &block_set, NULL) < 0) {
        perror("sigprocmask");
        exit(1);
    }
}

void sigalrm_handler(int sig) {
    fprintf(stderr, "%d received\n", sig);    
}
