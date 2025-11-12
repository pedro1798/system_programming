#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

int main() {
    fprintf(stdout, "I am parents\n");
    
    int son_stat, daughter_stat, grandson_stat, granddaughter_stat;
    pid_t pid_son, pid_daughter, pid_grandson, pid_granddaughter; 
    
    pid_son = fork(); // son fork
    
    if (pid_son == -1) { /* 실패 시 */
        perror("fork");
    } else if (pid_son == 0) { /* son 프로세스 스코프 */
        fprintf(stdout, "I am son.\n");
        
        pid_grandson = fork(); // grandson fork

        if (pid_grandson == -1) {
            perror("perror");
        } else if (pid_grandson == 0) { // grandson 프소세스 스코프
            fprintf(stdout, "I am grandson.\n");
            fprintf(stdout, "grandson exits.\n");
            exit(1);
        } else { // son 프로세스 스코프 
            if (waitpid(pid_grandson, &grandson_stat, 0) == -1) { // grandson 종료될 때 까지 기다린다.
                perror("waitpid");
            }
        }
        
        pid_granddaughter = fork(); // granddaughter fork()
        
        if (pid_granddaughter == -1) perror("fork");
        else if (pid_granddaughter == 0) { // granddaughter 프로세스 스코프
            fprintf(stdout, "I am granddaughter.\n");
            fprintf(stdout, "granddaughter exits.\n");
            exit(1);
        } else { // son 프로세스 스코프
            /* granddaughter 종료될 때 까지 기다린다. */
            if (waitpid(pid_granddaughter, &granddaughter_stat, 0) == -1) perror("waitpid");
        }
        
        fprintf(stdout, "son exits.\n");
        exit(1);
    } else { // parent 프로세스
        /* son 끝날 때 까지 기다린다. */
        if (waitpid(pid_son, &son_stat, 0) == -1) perror("waitpid"); // 에러처리
        
        pid_daughter = fork(); // daughter fork()

        if (pid_daughter == -1) perror("fork"); 
        else if (pid_daughter == 0) { // daughter 프로세스 스코프 
            fprintf(stdout, "I am daughter.\n");
            fprintf(stdout, "daughter exits.\n");
            exit(1);
        } else { // parent 프로세스 스코프
            if (waitpid(pid_daughter, &daughter_stat, 0) == -1) perror("waitpid"); 
        }
    }

    fprintf(stdout, "parent exits.\n");
}
