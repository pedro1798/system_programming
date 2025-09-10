#include <stdio.h>
#include <stdlib.h>
#include <utmp.h>
#include <fcntl.h>
#include <unistd.h>

#define SHOWHOST

void show_info(struct utmp *utbufp); 

int main() {
    struct utmp current_record; 
    int utmpfd; /* UTMP File Discriptor */
    int reclen = sizeof(current_record); /* record length */

    if ((utmpfd = open(UTMP_FILE, O_RDONLY)) == -1) {
        perror(UTMP_FILE);
        exit(1);
    }

    while (read(utmpfd, &current_record, reclen) == reclen) {
        show_info(&current_record);
    }
    close(utmpfd);
    return 0;
}

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
