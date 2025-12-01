process_request(fd) {
    int pid = fork();
    switch(pid) {
        case -1: return;
        case 0: dup2(fd, 1);
                close(fd);
                execvl("/bin/date", "date", NULL);
                oops("execlp");
        default: wait(NULL);
    }
}
