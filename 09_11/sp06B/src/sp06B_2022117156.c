#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#define MAXARGS 20 /* cmdline args */
#define ARGLEN 100 /* token length */
/* 
 * 1. 입력을 받는다 
 * 2. 받은 입력으로 while문 돌린다
 * 3. while문에서 실행하고 마지막에서 다시 입력을 받는다
 * 4. exit이면 종료한다
 */
int get_arg_list(char **arglist); 
void execute(char** arglist);

int main() {
    /*
     * use fork and execvp and wait to do it
     */
    char **arglist_ptr = (char**)malloc((MAXARGS + 1) * sizeof(char*));
    
    if (arglist_ptr == NULL) {
        perror("malloc");
        exit(1);
    }
    
    while(strcmp(arglist_ptr[0], "exit\0") != 0) {
        execute(arglist_ptr);
    }
}

void execute(char** arglist_ptr) {
    /*
     * use fold and execvp and wait to do it
     */
    int pid, exitstatus; /* of child */
    
    pid = fork();
    
    switch(pid) {
        case -1: /* 에러 처리 */
            perror("fork failed");
            exit(1);
        case 0: /* child면 */
            int arg_count = get_arg_list(arglist_ptr);
            if (arg_count > 0) {
                execvp(arglist_ptr[0], arglist_ptr);
            }
            perror("execvp failed"); /* execvp 정상적으로 실행되면 실행 안 됨 */
            exit(1);
        default:
            while(wait(&exitstatus) != pid) {
                printf("child exited with status %d,%d\n", 
                        exitstatus>>8, exitstatus&0377);
            }
    }
}

int get_arg_list(char** arglist_ptr) { /* argbuf에 입력 받고 arglist_ptr에 파싱 */
    char argbuf[ARGLEN]; /* read stuff here, 버퍼 */
    int arg_count = 0;
    /* 
     *  안전하게 지정된 파일 스트림으로부터 문자열을 읽어 버퍼에 저장
     *  char *fgets(char *s, int size, FILE *stream);
     *  s: 읽은 문자열을 저장할 메모리 버퍼의 포인터
     *  size: 버퍼 s의 최대 크기(읽을 문자의 최대 개수
     *  stream: 데이터를 읽어올 파일 스트림(stdin, 파일 포인터 등)
     *  성공: 읽은 문자열이 저장된 버퍼 s의 포인터를 반환
     *  실패: EOF에 도달하거나 읽기 오류 발생하면 NULL 반환
     *  특징: '\n'를 만날 경우, 이 문자까지 읽어 버퍼에 저장한다.
     *  EOF 처리: (size-1)개의 문자를 읽기 전에 스트림의 끝(EOF)에 도달하면,
     *  지금까지 읽은 문자열을 버퍼에 저장하고 널 문자를 추가한 후 포인터 
     *  s를 반환한다. 아무 문자도 읽지 못했을 때만 NULL을 반환한다.
     */
    if (fgets(argbuf, ARGLEN, stdin)) { // argbuf 버퍼에 인수 문자열 받는다
        if (strcmp(argbuf, "exit\n") == 0) { /* exit\n 만나면 종료 */
            fprintf(stderr, "stopped here, meet exit\n");
            return 0;
        }

        if (argbuf[strlen(argbuf)-1] == '\n') {
            printf("change last character from \\n to \\0");
            argbuf[strlen(argbuf)-1] = '\0';
        }
        /* 
         * 현재 실행 중인 프로세스의 메모리 이미지를 새로운 프로그램의 메모리
         * 이미지로 완전히 덮어씌우는 기능을 한다
         * int execvp(const char *file, char *const argv[]);
         * file: 실행할 파일의 이름(경로가 포함될 수 있음)
         * argv[]: 새로운 프로그램에 전달할 인수 문자열 포인터들의 배열
         * (관례상 첫 번째 argv[0]은 프로그램 이름과 동일해야 한다
         * v->vector, 인수를 개별적인 목록(vector, 즉 배열) 형태로 전달받는다  
         * p-> path: 실행할 파일에 경로가 지정되지 않은 경우에도 환경 변수 PATH에 
         * 등록된 디렉터리들을 검색하여 해당 실행 파일을 찾아준다(그래서 /bin/ls라고 명시하지 않아도 됨 
         */
        char *token = strtok(argbuf, " "); /* " "를 구분자로 tokenize*/
        
        while (token != NULL && arg_count < MAXARGS) {
            /* 토큰(문자열)의 주소를 새로운 배열에 저장 */
            arglist_ptr[arg_count++] = token;
            token = strtok(NULL, " "); /* 다음 토큰 가져오기 */
        }
        arglist_ptr[arg_count] = NULL; /* 배열의 끝 표시 */
    }
    return arg_count;
}
