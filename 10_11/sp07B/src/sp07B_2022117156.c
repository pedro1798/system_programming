#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>

void sigint_handler(int signal);
void sigalrm_handler(int signal);

volatile int tick = 0;

int main(int argc, char *argv[]) {
    if (argc > 2) {
        fprintf(stderr, "Too many arguments!");
        exit(-1);
    }
    int cnt = 3; // 초기값 3 
    if (argc == 2) cnt = atoi(argv[1]); // int로 형변환

    /* 이전 실행 시 생성된 시그널 차단 위해 블록 설정 */
    sigset_t block_set, prev_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGALRM);
    if (sigprocmask(SIG_BLOCK, &block_set, &prev_set) < 0) {
        perror("sigprocmask");
        exit(1);
    }

    struct sigaction sa_sigint, sa_sigalrm;
    sa_sigint.sa_handler = sigint_handler;
    sa_sigalrm.sa_handler = sigalrm_handler;

    sigemptyset(&sa_sigint.sa_mask); // 처리 중 다른 시그널 블록 X
    sigemptyset(&sa_sigalrm.sa_mask);
    // sa.sa_flags = 0;
    sigaction(SIGINT, &sa_sigint, NULL);
    sigaction(SIGALRM, &sa_sigalrm, NULL);
    
    // ITIMER_REAL 설정
    // struct itimerval timer = {{1,0},{1,0}};
    
    struct itimerval timer; // REAL 시간 타이머 선언
    timer.it_value.tv_sec = 1; // 처음 만료까지 1초
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1; // 이후 1초마다 반복
    timer.it_interval.tv_usec = 0;

    /* 시그널 블록 해제 */
    if (sigprocmask(SIG_SETMASK, &prev_set, NULL) < 0) {
        perror("sigprocmask restore");
        exit(1);
    }

    setitimer(ITIMER_REAL, &timer, NULL);

    while(cnt > 0) {
        pause();
        if (tick) {
            fprintf(stdout, "after %ds\n", cnt--);
            fflush(stdout);
            tick = 0;
        }
    }
    fprintf(stdout, "time out\n");
    return 0;
}

void sigint_handler(int signal) { // ctrl + c 입력받으면 종료
    fprintf(stdout, "\nuser interrupt\n");
    exit(0);
}

void sigalrm_handler(int signal) {
    tick = 1;
    // fprintf(stdout, "sigalrm received\n");
}
