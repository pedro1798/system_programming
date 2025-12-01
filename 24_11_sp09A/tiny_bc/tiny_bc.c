#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define opps(m,x) {perror(m); exit(x);}

void be_dc(int in[2], int out[2]);
void be_bc(int todc[2], int fromdc[2]);
void fatal(char mess[]);

int main() {
    int pid, todc[2], fromdc[2]; /* equipment */
    /* make two pipes */
    if (pipe(todc) == -1 || pipe(fromdc) == -1)
        opps("pipe fialed", 1);
    /* get a process for user interface */
    if (pid = fork() == -1)
        opps("cannot fork", 2);
    if (pid == 0)
        be_dc(todc, fromdc);
    else {
        be_bc(todc, fromdc);
        wait(NULL);
    }
}

void be_dc(int in[2], int out[2]) {
    /*
     * set up stdin and stdout, then execl dc
     */
    /* set up etdin from pipein */
    if (dup2(in[0], 0) == -1) /* copy read end to 0 */
        opps("dc: cannot readirect stdin", 3);
    close(in[0]); /* move to fd 0 */
    close(in[1]); /* won't write here */
    /* setup stdout to pipeout */
    if (dup2(out[1], 1) == -1)
        opps("dc: cannot redirect stdout", 4);
    close(out[1]);
    close(out[0]);
    /* now execl dc with the - option */
    execlp("dc", "dc", "-", NULL);
    opps("cannot run dc", 5);
}
void be_bc(int todc[2], int fromdc[2]) {
    int num1, num2;
    char operation[BUFSIZ], message[BUFSIZ];
    FILE *fpout, *fpin;
    /* setup */
    close(todc[0]); /* won't read from pipe to dc */
    close(fromdc[1]); /* won't write to pipe from dc */
    fpout = fdopen(todc[1], "w");
    fpin = fdopen(fromdc[0], "r");
    if (fpout == NULL || fpin == NULL)
        fatal("Error converting pipes to streams");
    /* main loop */
    while (printf("tinbc: "), fgets(message, BUFSIZ, stdin) != NULL) {
        /* parse input */
        if (sscanf(message, "%d%[-+*/^]%d", &num1, operation,
                    &num2) != 3) {
            printf("syntax error\n");
            continue;
        }
        if (fprintf(fpout, "%d\n%d\n%c\np\n", num1, num2, 
                    *operation) == EOF) {
            fatal("Error writing");
        }
        fflush(fpout);
        if (fgets(message, BUFSIZ, fpin) == NULL) {
            break;
        }
        printf("%d %c %d = %s", num1, *operation, num2, message);
    }
    fclose(fpout); /* close pipe */
    fclose(fpin); /* dc will see EOF */
}

void fatal(char mess[]) {
    fprintf(stderr, "Error: %s\n", mess);
    exit(1);
}
