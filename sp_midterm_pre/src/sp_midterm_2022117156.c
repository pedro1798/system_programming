#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define BUFFERSIZE 1024

void print_file(char *path[]);
void do_cat(char *path); 

int main(int ac, char *av[]) {
    if (ac < 2) {
        do_cat(".");
    } else {
        for (int i = 1; i < ac; i++) {
            do_cat(av[i]);
        }
    }
    return 0;
}

void do_cat(char *path) {
    DIR *dir_ptr;
    struct dirent *direntp;
    char fullpath[PATH_MAX]
    
    if (stat(path, &info) == -1) { // stat 호출해 info에 파일 정보 저장
        perror(path);
        fprintf(stderr, "stat(path, &info) failed");
        return;
    }
    if (!S_ISDIR(info.st_mode)) { // 매개변수가 디렉토리가 아니면
        do_cat(path);
        return;
    }
    if ((dir_ptr = opendir(path)) == NULL) {
        perror(path);
        fprintf(stderr, "Cannot open %s\n", path);
        return
    }
    while ((direntp = readdir(dir_ptr)) != NULL) {
        snprintf(fullpath, PATH_MAX, "%s/%s", path, direntp->d_name);
        struct stat entry_info;

        if (stat(fullpath, &entry_info) == -1) {
            perror(fullpath);
            continue;
        }
        if (S_ISDIR(info.st_mode) &&
            strcmp(info->d_name))
         
    }


}

void print_file(char *path[]) {
    int in_fd, n_chars;
    char buf[BUFFERSIZE];

    if ((in_fd = open(path, O_RDONLY)) == -1) {
        perror(path);
    }
    while ((n_chars = read(in_fd, buf, BUFFERSIZE)) > 0) {
        if (write(0, buf, n_chars) != n_chars) {
            fprintf(stderr, "write error");
        }
        if (n_chars == -1) {
            fprintf(stderr, "Read error from %s", av[1]);
        }
        if (close(in_fd) == -1) {
            fprintf(stderr, "error closing file fd: %d", in_fd);
        }
}
