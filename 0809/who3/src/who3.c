/* who3.c - who with buffered reads
 *        - surpresses empty records
 *        - formats time nicely
 *        - buffers input (using utmplib)
 * utmp_open, utmp_next, utmp_reload, utmp_close
 */

#include <time.h> /* formats time nicely */
#include <stdio.h>
#include <stdlib.h>
#include <utmp.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "utmplib.h"

#define SHOWHOST

void showtime(long);
void show_info(struct utmp *utbufp); 

int main() {
    struct utmp *utbufp; 
    /* struct utmp *utmp_next(); utmplib.c에서 정의 */

    if (utmp_open(UTMP_FILE) == -1) {
        perror(UTMP_FILE); /* 오픈 시 에러 처리 */
        exit(1);
    }
    while ((utbufp = utmp_next()) != (struct utmp *) NULL) {
        /* utbufp가 NULL을 가리키지 않을 때 while 반복 */
        show_info(utbufp);
    }
    utmp_close(); /* utmplib.c 함수 */
    return 0;
}

/* show_info()
 *  displays the contents of the utpm struct in human readable form
 *  displays nothing if record has no user name
 */

void show_info(struct utmp *utbufp) {
    if ( utbufp->ut_type != USER_PROCESS ) { /* users only */
        return;
    }
    printf("%-8.8s", utbufp->ut_name); /* the username */
    printf(" ");
    printf("%-8.8s", utbufp->ut_line);
    printf(" ");
    showtime(utbufp->ut_time);
    printf(" ");

#ifdef SHOWHOST /* if SHOWHOST is defined, compile this code */
    printf("(%s)", utbufp->ut_host);
#endif
    printf("\n");
}

void showtime(long timeval) {
    char *cp;
    cp = ctime(&timeval); /* epoch time -> 문자열 변환 */
    printf("%12.12s", cp+4);
}
