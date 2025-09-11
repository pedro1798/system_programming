/* 
 * logout_tty(char * line)
 * UTMP를 열어 ut_line(터미널 이름)이 인자로 받은 line과 일치하는
 * 레코드를 찾아 그 레코드의 타입을 DEAD_PROCESS로 바꾸고(로그아웃 처리)
 * 그 시간을 현재 시간으로 갱신한 뒤 파일에 덮어쓰는 역할을 한다.
 * marks a utmp recoed as logged out
 * does not blank username or remote host
 * return -1 on error, 0 on success
 */

#include <string.h>
#include <time.h>
#include <utmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/* UTMP 복사본으로 안전하게 테스트 */ 
/*
#undef UTMP_FILE
#define UTMP_FILE "/home/ubuntu/utmp.test" 
*/

int logout_tty(char *line) {
    int fd; /* utmp 파일 디스크립터 */
    struct utmp rec; /* 한 레코드 */
    int len = sizeof(struct utmp); /*레코드 크기(바이트) */
    int retval = -1; 
    /* return value, pessimism, 기본 실패(-1), 성공 시 0으로 설정*/

    if ((fd = open(UTMP_FILE, O_RDWR)) == -1) { /* utmp 파일 오픈 */
        return -1; /* 에러처리 */
    }

    /* search and replace 
     * 레코드 순차 검색 */
    while (read(fd,&rec, len) == len) {
        /* rec.ut_line: 로그인한 터미널 이름이 인자 line과 같다면
         * 그 레코드를 수정 대상으로 삼는다.
         * strncmp로 비교(안전하게 길이 제한 비교)
         */

        /* 디버그 출력 */
        printf("DEBUG: rec.ut_line='%s'\n", rec.ut_line);
        if (strncmp(rec.ut_line, line, sizeof(rec.ut_line)) == 0) {
            /* 해당 터미널 이름 찾으면 로그아웃 시킨다 */
            rec.ut_type = DEAD_PROCESS; 
            rec.ut_tv.tv_sec = time(NULL);
            rec.ut_tv.tv_usec = 0;

            if (lseek(fd, -len, SEEK_CUR) != -1) {
                if (write(fd, &rec, len) == len) {
                    retval = 0; 
                }
            }

            /* time 관련 코드 최신 버전에 맞게 수정
            if (time(&rec.ut_time) != -1) {
                if (lseek(fd, -len, SEEK_CUR) != -1) {
                    if (write(fd, &rec, len) == len) {
                        retval = 0; 
                    }
                }
            }
            */
            break;
        }
    }

    /* close the file */
    if (close(fd) == -1) {
        retval = -1; 
        /* 파일 닫기 실패 시 retval을 -1로 설정. */
    } 
    return retval;
}
