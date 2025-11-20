#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <string.h>

void usr1_handler(int sig);
void chld_handler(int sig);
void usr2_handler(int sig);

volatile sig_atomic_t tick = 0;
volatile int flag = 0;

/*
 * 1. child 프로세스는 부모에게 sigusr1 or sigusr2 시그널을 1초 간격으로
 * 부모 프로세스에게 보낸다
 * parent 프로세스는 시그널 받을 때 화면에 출력한다
 * parent는 child 프로세스가 종료되면 sigchld received 출력하고 종료한다
 */
int main(int ac, char **argv) {
    int state;
    pid_t pid;
    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) { /* 자식 프로세스 */ 
        srand(time(NULL));
        pid_t parent_p = getppid();

        for (int i = 0; i < 3; i++) {
            /* 1초마다 3번 시그널을 보낸다 */
            int r = rand();
            if (r % 2 == 0) 
                kill(parent_p, 10);
            else 
                kill(parent_p, 12);
            sleep(1);
        }
        printf("sigchld terminated\n");
        exit(0);
    } else { /* 부모 프로세스 */ 
        struct sigaction sa_usr1, sa_usr2, sa_chld;
        sa_usr1.sa_handler = usr1_handler;
        sa_usr2.sa_handler = usr2_handler;
        sa_chld.sa_handler = chld_handler;

        sigemptyset(&sa_usr1.sa_mask);
        sigemptyset(&sa_usr2.sa_mask);
        sigemptyset(&sa_chld.sa_mask);

        sa_usr1.sa_flags = 0;
        sa_usr2.sa_flags = 0;
        sa_chld.sa_flags = 0;

        sigaction(10, &sa_usr1, NULL);
        sigaction(12, &sa_usr2, NULL);
        sigaction(17, &sa_chld, NULL);

        int cnt = 0;

        while (cnt < 3) {
            pause();
            if (tick == 1) {
                fprintf(stdout, "SIGUSR1 is received.\n");
                tick = 0;
            }
            if (tick == 2) {
                fprintf(stdout, "SIGUSR2 is received.\n");
                tick = 0;
            }
        }
        if (flag) fprintf(stdout, "SIGCHLD is received.\n");
        fprintf(stdout, "parent is terminated.\n");
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
    while ( waitpid(-1, NULL, WNOHANG) > 0) {
        flag = 1;
    }
}
