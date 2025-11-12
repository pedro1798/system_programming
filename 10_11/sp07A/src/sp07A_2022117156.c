#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

int main() {
    /* waitpid state 변수, pid 변수 선언 */
    int son_st, daughter_st, grandson_st, granddaughter_st;
    pid_t son, daughter, grandson, granddaughter;
    
    fprintf(stdout, "I am parent.\n");
    
    son = fork(); // son fork();
    if (son == -1) perror("fork"); // fork 에러처리
    else if (son == 0) { // son 스코프
        sleep(1); // 출력 순서 조정 
        fprintf(stdout, "I am son.\n");

        grandson = fork(); // grandson 출력 
        if (grandson == -1) perror("fork"); // 에러처리 
        else if (grandson == 0) { // grandson 스코프 
            sleep(2);
            fprintf(stdout, "I am grandson.\n");
            sleep(3);
            fprintf(stdout, "grandson exits.\n");
            exit(3); // grandson 종료 
        } else { // son 스코프 
            granddaughter = fork(); // granddaughter fork 
            if (granddaughter == -1) perror("fork"); // 에러처리 
            else if (granddaughter == 0) { // granddaughter 스코프
                sleep(2);
                fprintf(stdout, "I am granddaughter.\n");
                sleep(3);
                fprintf(stdout, "granddaughter eixts.\n");
                exit(4); // granddaughter 종료 
            } else { // son 스코프, 자식 종료 확인 
                if (grandson == waitpid(grandson, &grandson_st, 0)) {
                    fprintf(stdout, "son finished waiting for grandson.\n");
                } // grandson 종료 확인 
                if (granddaughter == waitpid(granddaughter, &granddaughter_st, 0)) {
                    fprintf(stdout, "son finished waiting for granddaughter.\n");
                } // granddaughter 종료 확인 
                fprintf(stdout, "son eixts.\n");
                exit(1); // son 종료 
            }
       }
    } else { // parent 스코프 
        daughter = fork(); // daughter fork 
        if (daughter == -1) perror("fork"); // error handling 
        else if (daughter == 0) { // daughter scope
            sleep(1);
            fprintf(stdout, "I am daughter.\n");
            sleep(6);
            fprintf(stdout, "daughter exits.\n");
            exit(3); // daughter exits
        } else { // parent scope 
            if (son == waitpid(son, &son_st, 0)) { // 종료하면 그 프로세스의 pid 반환
                fprintf(stdout, "parent finished waiting for son.\n");
            } // son 종료 확인 
            if (daughter == waitpid(daughter, &daughter_st, 0)) {
                fprintf(stdout, "parent finished waiting for daughter.\n");
            } // daughter 종료 확인 
            /* if son and daughter exits */
        }
    }
    /* 모든 자식 종료 확인 후 종료 */
    fprintf(stdout, "parent exits.\n");
    return 0;
}
