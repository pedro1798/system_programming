#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static volatile sig_atomic_t tick = 0;
void sigalrm_handler(int sig);

void set_ticker(int tick) {
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
    sa_sigalrm.sa_flags = SA_RESTART; // getch() 재시작 
    sigaction(SIGALRM, &sa_sigalrm, NULL);

    /* 타이머 설정 */
    struct itimerval timer = {{0, tick}, {0, tick}};
    setitimer(ITIMER_REAL, &timer, NULL);
    
    /* 시그널 블록 해제 */
    if (sigprocmask(SIG_UNBLOCK, &block_set, NULL) < 0) {
        perror("sigprocmask");
        exit(1);
    }
}

void sigalrm_handler(int sig) {
    (void)sig; // 이 변수는 사용하지 않음.
    tick = 1;
}

int get_tick() {
    return tick;
}

void set_tick() {
    tick = 0;
}
