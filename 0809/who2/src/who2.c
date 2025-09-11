/* who2.c - read/etc/utmp and list info therein
 *      - suppresses empty records
 *      - formats time nicely 
 */

/* c 문법: ->는 포인터가 가리키는 구조체의 멤버 접근
 */
# include <stdio.h>
# include <unistd.h>
# include <utmp.h>
# include <fcntl.h>
# include <time.h> /* formats time nicely */
# include <stdlib.h>

/* #define SHOWHOST */

void showtime(long);
void show_info(struct utmp *); /* 메모리 주소를 넘김 */

int main() {
    struct utmp utbuf; /* read info into here */
    int utmpfd; /* read from this descriptor */

    if ((utmpfd = open(UTMP_FILE, O_RDONLY)) == -1) {
        perror(UTMP_FILE); /* utmp 파일 오픈 에러처리 */
        exit(1);
    }
    
    while (read(utmpfd, &utbuf, sizeof(utbuf)) == sizeof(utbuf)) { 
        /* 읽어 온 버퍼 크기가 일치하면 while문 반복 */
        show_info(&utbuf); /* 읽어 온 레코드 출력 */
    }
    close(utmpfd); /* utmp 파일 close */
    return 0;
}

/* show info()
 *      displays the contents of the utmp struct
 *      in human readable form
 *      displays nothing if record has no user name
 */

void show_info(struct utmp * utbufp) {
    if (utbufp->ut_type != USER_PROCESS) {
        return; /* 유저 프로세스만 취급 */
    } 
    printf("%-8.8s", utbufp->ut_name); /* the username */
    printf(" ");
    printf("%-8.8s", utbufp->ut_line);
    printf(" ");
    showtime(utbufp->ut_time); /* displays time */
    
    # ifdef SHOWHOST
        /* print if only string char list isn't empty */
        if (utbufp->ut_host[0] != '\0') { 
            /* the host */
            printf(" (%s)", utbufp->ut_host);
        }
    # endif 
        printf("\n"); /* newline */
}

/* void showtime(long timeval) : 
 *  - displays time in a format fit for human consumption
 *  - uses ctime to build a string then picks parts out of it
 *  - Note: %12.12s prints a string 12 chars wide and LIMITS
 *  - it to 12chars.
 */

void showtime(long timeval) {
    char *cp;
    cp = ctime(&timeval); /* epoch time -> 문자열 변환 */
    printf("%12.12s", cp+4);
}

