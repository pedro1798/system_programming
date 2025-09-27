#include <stdio.h>
#include <sys/types.h>
#include<sys/stat.h>
#include <dirent.h>

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
     * kindof recursive
     */
    ino_t my_inode;
    char its_name[BUFSIZ];

    if (get_inode("..") != this_inode ) {
        chdir("..");
        inum_to_name(this_inode, its_name, BUFSIZ);

        my_inode = get_inode(".");
        printpathto(my_inode);
        printf("%s", its_name);
    }
}
