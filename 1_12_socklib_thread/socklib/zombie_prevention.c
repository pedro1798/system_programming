main() {
    int sock, fd;

    signal(SIGCHLD, child_waiter);
    if ((sock = make_server_socket(PORTNUM)) == -1) oops("make_server_socket");
    while(1) {
        fd = accept(sock, NULL, NULL);
        if (fd == -1) break;
        process_request(fd);
        close(fd);
    }
}
void child_waiter(int signum) {
    wait(NULL);
}
void process_request(int fd) {
    if (fork() == 0) {
        dup2(fd, 1);
        close(fd);
        execlp("date", "date", NULL);
        oops("execlp date");
    }
}
