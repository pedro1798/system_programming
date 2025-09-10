/* who3.c - who with buffered reads
 *        - surpresses empty records
 *        - formats time nicely
 *        - buffers input (using utmplib)
 */

#include <stdio.h>
#include <stdlib.h>
#include <utmp.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "utmplib.h"

#define SHOWHOST

void showtime(time_t);
void show_info(struct utmp *utbufp); 

int main() {
    struct utmp *utbufp, 
                *utmp_next();
    if (utmp_open(UTMP_FILE) == -1) {
        perror(UTMP_FILE);
        exit(1);
    }
    while ((utbufp = utmp_next()) != (struct utmp *) NULL) {
        show_info(utbufp);
    }
    utmp_close();
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
    printf("%-8.8d", utbufp->ut_time);
    printf(" ");

#ifdef SHOWHOST /* if SHOWHOST is defined, compile this code */
    printf("(%s)", utbufp->ut_host);
#endif
    printf("\n");
}
