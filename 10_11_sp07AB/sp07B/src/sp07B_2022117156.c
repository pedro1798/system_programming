#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>

/* 시그널 핸들러 시그니쳐 */
void sigint_handler(int signal);
void sigalrm_handler(int signal);

// volatile int tick = 0; // 시그널 핸들러에서 접근하는 전역 변수
/*
 * sig_atomic_t: 표준 수준에서 시그널 핸들러에서 
 * 깨지지 않고 안전하게 읽고 쓸 수 있는 자료형.
 */
volatile sig_atomic_t tick = 0;

int main(int argc, char *argv[]) {
    /* 이전 실행 시 생성된 시그널 차단 위해 블록 설정 */
    sigset_t block_set;
    sigemptyset(&block_set); // 시그널 set을 비운다 
    sigaddset(&block_set, SIGALRM); // 시그널 set에 SIGALRM 추가 
    
    /* 프로그램 실행 전 발생한 SIGALRM 시그널 무시 */
    if (sigprocmask(SIG_BLOCK, &block_set, NULL) < 0) {
        perror("sigprocmask");
        exit(1); // 에러 output
    }
    
    /* 인자 개수 예외처리 */
    int cnt = 3; // 기본 3 설정
    if (argc > 2) { // 인자 2개 이상 시 예외처리 
        fprintf(stderr, "Please enter a maximum of 1 integer\n");
        exit(1); // 0: 정상 종료, 1: 오류
    } // 인자 1개 있을 때 1 이상이라면 

    if (argc == 2) {
        int arg = atoi(argv[1]);
        if (arg == 0) {
            fprintf(stderr, "You entered: %s, please enter a maximum of 1 integer.\n", argv[1]);
            exit(1);
        }
        if (arg > 0) cnt = arg;
    }

    /* 핸들러 등록 */
    struct sigaction sa_sigint, sa_sigalrm;
    sa_sigint.sa_handler = sigint_handler;
    sa_sigalrm.sa_handler = sigalrm_handler;

    sigemptyset(&sa_sigint.sa_mask); // 처리 중 다른 시그널 블록 X
    sigemptyset(&sa_sigalrm.sa_mask);
    
    sa_sigint.sa_flags = 0; // 시그널 기본 처리. 특별한 동작 없음.
    sa_sigalrm.sa_flags = 0;

    sigaction(SIGINT, &sa_sigint, NULL); // 시그널 핸들러 등록 
    sigaction(SIGALRM, &sa_sigalrm, NULL);
    
    // 타이머(ITIMER_REAL) 설정
    // struct itimerval timer = {{1,0},{1,0}};
    struct itimerval timer; // REAL 시간 타이머 선언
    timer.it_value.tv_sec = 1; // 처음 만료까지 1초
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1; // 이후 1초마다 반복
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL); // 타이머 시작 
    
    /* 시그널 블록 해제 */
    if (sigprocmask(SIG_UNBLOCK, &block_set, NULL) < 0) {
        perror("sigprocmask");
        exit(1);
    }

    while(cnt > 0) {
        pause(); // 시그널 받기 전까지 정지 
        if (tick) { // 시그널 들어오면 이 블럭 실행.
            fprintf(stdout, "after %ds\n", cnt--);
            fflush(stdout); // 버퍼 출력
            tick = 0;
        }
    }
    fprintf(stdout, "time out\n"); // SIGINT 없었다면 출력 후 정상 종료.
    return 0;
}

void sigint_handler(int signal) { // ctrl + c 입력받으면 종료
    fprintf(stdout, "\nuser interrupt\n");
    exit(0); // 정상 종료 
}

void sigalrm_handler(int signal) { // tick으로 while문 제어 
    tick = 1;
}
