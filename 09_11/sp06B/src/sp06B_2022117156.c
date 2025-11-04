#include <stdio.h>
#include <signal.h>
#define MAXARGS 20 /* cmdline args */
#define ARGLEN 100 /* token lenght */

int main() {
    char *arglist[MAXARGS+1]; /* an array of ptrs */
    int numargs; /* index into array */
    char argbuf[ARGLEN]; /* read stuff here */
    char *makestring(); /* malloc etc */
    numargs = 0;
    while (numargs < MAXARGS) {
        printf("Arg[%d]? ", numargs);
        if (fgets(argbuf, ARGLEN, stdin) && *argbuf != 'n') {
            arglist[numargs++] = makestring(argbuf);
        } else if (numargs > 0 ){
            arglist[numargs] = NULL; /* close list */
            execute(arglist); /* do it */
            numargs = 0; /* and reset */
        }
    }
}

execute(char *arglist[]) {
}
