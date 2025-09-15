#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

void do_ls(char [], char *);

int main(int ac, char *av[]){
    char * cmd = av[0];
    if (ac == 1) {
        do_ls(".", cmd);
    } else {
        while (--ac) {
            printf("%s:\n", *++av);
            do_ls(*av, cmd);
        }

    }
}

void do_ls(char dirname[], char * cmd) {
    DIR *dir_ptr; // the directory
    struct dirent *direntp; // each entry
    if ((dir_ptr = opendir(dirname)) == NULL) {
        fprintf(stderr, "%s: ls1: cannot open %s\n", cmd, dirname); 
    } else {
        while ((direntp = readdir(dir_ptr)) != NULL) {
            printf("%s: %s\n", cmd, direntp->d_name);
        }
        closedir(dir_ptr);
    }
}
