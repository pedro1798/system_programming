#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include<sys/stat.h>
#include <dirent.h>
#include <stdlib.h>

ino_t get_inode(char *);
void printpathto(ino_t);
void inum_to_name(ino_t, char *, int);

int main() {
    printpathto(get_inode("."));
    putchar('\n');
    return 0;
}

ino_t get_inode(char *fname) {
    /*
     * returns inode number of the file
     */
    struct stat info;
    if (stat(fname, &info) == -1) {
        fprintf(stderr, "Cannot stat ");
        perror(fname);
        exit(1);
    }
    return info.st_ino;
}

void printpathto(ino_t this_inode) {
    /*
     * prints path leading down to an object with this inode
     * kind of recursive
     */
    ino_t my_inode;
    char its_name[BUFSIZ];

    if (get_inode("..") != this_inode ) {
        chdir("..");
        inum_to_name(this_inode, its_name, BUFSIZ);

        my_inode = get_inode(".");
        printpathto(my_inode);
        printf("/%s", its_name);
    } else {
        printf("/");
    }
}

void inum_to_name(ino_t inode_to_find, char *namebuf, int buflen) {
    /* 
     * looks through current directory for a file with this inode number
     * and copies its name into namebuf
     */
    DIR *dir_ptr; // the directory
    struct dirent *direntp; /* each entry */
    
    dir_ptr = opendir(".");
    if (dir_ptr == NULL ) {
        perror(".");
        exit(1);
    }
    /* 
     * search directory for a file with specified inum
     */
    while ((direntp = readdir(dir_ptr)) != NULL) {
        if (direntp->d_ino == inode_to_find) {
            strncpy(namebuf, direntp->d_name, buflen);
            namebuf[buflen-1] = '\0'; /* just in case */
            closedir(dir_ptr);
            return;
        }
    }
    fprintf(stderr, "error looking for inum %ld\n", inode_to_find);
    closedir(dir_ptr);
    exit(1);
}
