#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <string.h>

/* 시그널 핸들러 */
void usr1_handler(int sig);
void chld_handler(int sig);
void usr2_handler(int sig);

/* 
 * 컴파일러의 캐싱 최적화를 막아 코드가 변수에 접근할 때 마다 메모리에서 다시 읽어오고, 값을 쓸 때도 메모리에 직접 쓰도록 강제
 * volatile이어야지 컴파일러는 flag의 값이 바뀔 수 있음을 알고 캐싱된 값이 아니라 메모리에서 읽어와서 체크한다.
 * 시그널 핸들러와 관련된 변수를 선언할 때 sig_atomic_tick 타입을 함께 사용하는 것이 관례이다.
 * sig_atomic_t는 시그널 핸들러 내에서 원자적으로 접근할 수 있음이 보장된 정수 타입이다.
 */
volatile sig_atomic_t tick = 0;
volatile int flag = 0;

/*
 * 1. child 프로세스는 부모에게 sigusr1 or sigusr2 시그널을 1초 간격으로
 * 부모 프로세스에게 보낸다
 * parent 프로세스는 시그널 받을 때 화면에 출력한다
 * parent는 child 프로세스가 종료되면 sigchld received 출력하고 종료한다
 */
int main(int ac, char **argv) {
    int state; /* waitpid 상태 담을 변수 */
    pid_t pid;
    pid = fork(); 

    if (pid < 0) { /* fork 예외처리 */
        perror("fork");
        exit(1);
    } else if (pid == 0) { /* 자식 프로세스 */ 
        srand(time(NULL)); /* rand 시드 설정 */
        pid_t parent_p = getppid(); /* 부모 프로세스 pid 저장 */

        for (int i = 0; i < 3; i++) {
            /* 1초마다 3번 시그널을 보낸다 */
            int r = rand(); /* USR1, USR2 중 랜덤한 시그널을 보낸다 */
            if (r % 2 == 0) 
                kill(parent_p, 10); /* 10: SIGUSR1*/
            else  
                kill(parent_p, 12); /* 12: SIGUSR2 */
            sleep(1); /* 1초마다 */
        }
        exit(0); /* 정상 종료 */
    } else { /* 부모 프로세스 */ 
        /* 설정 정보를 담는 구조체 변수 */
        struct sigaction sa_usr1, sa_usr2, sa_chld;
        /* sigaction 변수에 시그널 핸들러 등록 */
        sa_usr1.sa_handler = usr1_handler;
        sa_usr2.sa_handler = usr2_handler;
        sa_chld.sa_handler = chld_handler;
    
        /* 시그널 핸들러가 실행되는 동안 차단할 시그널 집합 빈 상태로 초기화 */
        sigemptyset(&sa_usr1.sa_mask);
        sigemptyset(&sa_usr2.sa_mask);
        sigemptyset(&sa_chld.sa_mask);

        /* 
         * 시그널 핸들러 동작 설정 플래그
         * 0: 모든 옵션을 사용하지 않음
         * SA_RESTART: 시그널에 의해 중단된 시스템 호출을 자동 재시작
         * 등등 존재한다
         */
        sa_usr1.sa_flags = 0;
        sa_usr2.sa_flags = 0;
        sa_chld.sa_flags = 0;

        /* 시그널 핸들러 등록 */
        sigaction(10, &sa_usr1, NULL);
        sigaction(12, &sa_usr2, NULL);
        sigaction(17, &sa_chld, NULL);

        int cnt = 0;

        while (cnt < 3) {
            /* 세 번 받을때까지 */
            pause(); /* busy wait 방지 */
            if (tick == 1) {
                fprintf(stdout, "SIGUSR1 is received.\n");
                cnt++;
                tick = 0;
            }
            if (tick == 2) {
                fprintf(stdout, "SIGUSR2 is received.\n");
                cnt++;
                tick = 0;
            }
        }
        if (flag == 0) {
            /* SIGCHLD 시그널 아직 안 들어왔으면 대기 */
            if (waitpid(pid, &state, 0) == -1) {
                /* waitpid 예외처리 */
                perror("waitpid");
                exit(1);
            } 
        }
        fprintf(stdout, "SIGCHLD is received.\n");
        fprintf(stdout, "parent is terminated.\n");
        fflush(stdout);
        return 0;           
    }
}

void usr1_handler(int sig) {
    tick = 1;
}
void usr2_handler(int sig) {
    tick = 2;
}
void chld_handler(int sig) {
    flag = 1;
}
