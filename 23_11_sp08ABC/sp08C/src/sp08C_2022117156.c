#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    /* replace standard i/o into pipe */
    /* ls 프로세스(부모)에서 쓰고, wc프로세스(자식)에서 읽으면 됨 */
    pid_t pid;
    int pipefd[2]; /* 파이프 생성할 변수 */
    pipe(pipefd); /* 파이프 생성 */
    
    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) { /* 자식 프로세스 */
        close(pipefd[1]); /* 자식 프로세스는 파이프 읽기만 하기 때문에 쓰기 끝을 닫는다. */
        dup2(pipefd[0], 0); /* stdin을 pipefd[0]으로 대체, ls에서 넘어온 데이터를 파이프로 받는다 */
        char *argv_wc[] = {"wc", NULL}; /* execvp로 wc 실행하기 위한 char* 배열 선언 */
        execvp(argv_wc[0], argv_wc); /* execvp로 wc로 프로세스 갈아끼우기 */
        perror("execvp failed");
        return 1;
        
    } else {
        close(pipefd[0]); /* 읽기 끝 필요없다 */
        /* 표준 출력 대신 pipefd[1]에 쓴다 */
        dup2(pipefd[1], 1);
        char *argv_ls[] = {"ls", NULL}; /* execvp로 ls 실행하기 위한 char* 배열 */
        execvp(argv_ls[0], argv_ls);
        perror("execvp failed"); /* 실행되지 않는다 */
        return 1;
    }
}
