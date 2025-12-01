process_rq(char *rq, int fd) {
    char cmd[11], arg[513];

    if (fork() != 0) return;

    sscanf(rq, "%10s %512s", cmd, arg);
    if (strcmp(cmd, "GET") != 0) cannot_do(fd);
    else if (not_exist(arg)) do_404(arg, fd);
    else if (isadir(arg)) do_ls(arg, fd);
    else if (ends_in_cgi(arg)) do_exec(arg, fd);
    else do_cat(arg, fd);
}

do_ls(char *dir, int fd) {
    FILE *fp;
    fp = fdopen(fd, "w");
    header(fp, "text/plain");
    fprintf(fp, "\r\n");
    fflush(fp);

    dup2(fd, 1);
    dup2(fd, 2);
    close(fd);
    execl("/bin/ls", "ls", "-l", dir, NULL);
    perror(dir);
    exit(1);
}
