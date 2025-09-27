/* cp1.c
 * version 1 of cp - uses read and write with tunable buffer size   
 * usage: cp1 src dest
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define BUFFERSIZE 1024
#define COPYMODE 0644

void oops(char *s1, char *s2);

int main(int ac, char *av[]) {
    int in_fd, out_fd, n_chars;
    char buf[BUFFERSIZE];

    if (ac != 3) { /* check args */
        fprintf(stderr, "usage:%s source destination\n", *av);
        /* av어레이의 첫 번째 요소는 파일 이름 */
        exit(1);
    }

    /* open files */
    if ((in_fd = open(av[1], O_RDONLY)) == -1) { 
        /* 매개변수로 받은 src-file을 연다.  */
        oops("Cannot open", av[1]); /* 에러처리 */
    }

    if ((out_fd = creat(av[2], COPYMODE)) == -1) { 
        /* 매개변수로 받은 dest-file을 생성한다 */
        oops("Cannot create", av[2]); /* 에러처리 */
    }

    /* copy files */
    while((n_chars = read(in_fd, buf, BUFFERSIZE)) > 0) { /* 읽어온 데이터가 있을 때 */
        if (write(out_fd, buf, n_chars) != n_chars) {
            oops("Write error to", av[2]);
        }
        if (n_chars == -1) {
            oops("Read error from", av[1]);
        }
        if (close(in_fd) == -1 || close(out_fd) == -1) {
            oops("error closing file", "");
        }
    }
}

void oops(char *s1, char *s2) {
    fprintf(stderr, "error:%s", s1); 
    /* stderr: 표준 에러 출력 스트림(fd 2)
     * 버퍼링되지 않고 바로 출력된다.
     * 프로그래머가 지정한 문자열을 출력한다.
     */
    
    perror(s2);
    /* perror는 직전에 실패한 시스템 콜이나 라이브러리 함수가 남겨놓은 
     * 전역 변수 errno 값을 확인해서, 그에 맞는 시스템 에러 메시지를 출력합니다.
     * 출력 형식: 
     *  s2: <시스템에 정의된 에러 설명 문자열>
     */
    exit(1);
}
