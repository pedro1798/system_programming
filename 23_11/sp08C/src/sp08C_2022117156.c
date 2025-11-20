#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    /* replace standard i/o into pipe */
    /* ls 프로세스(부모)에서 쓰고, wc프로세스(자식)에서 읽으면 됨 */
    pid_t pid;
    int pipefd[2];
    pipe(pipefd);
    
    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) { /* 자식 프로세스 */
        close(pipefd[1]);
        dup2(pipefd[0], 0);
        char *argv_wc[] = {"wc", NULL};
        execvp(argv_wc[0], argv_wc);
        
    } else {
        close(pipefd[0]); /* 읽기 끝 필요없다 */
        /* 표준 출력 대신 pipefd[1]에 쓴다 */
        dup2(pipefd[1], 1);
        char *argv_ls[] = {"ls", NULL};
        execvp(argv_ls[0], argv_ls);
        perror("execvp failed");
        return 1;
    }
}
