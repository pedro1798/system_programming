#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>

ino_t get_inode(char *); // 파일 이름->inode 번호 반환 
void printpathto(ino_t); // 현재 디렉토리 경로 출력
void inum_to_name(ino_t, char *, int); // inode->디렉토리 이름 반환

int main() {
    // 현재 디렉토리(".")의 inode를 가져와 printpathto() 호출
    printpathto(get_inode("."));
    putchar('\n');
    return 0;
}

ino_t get_inode(char *fname) {
    /*
     * returns inode number of the file
     * stat() 시스템콜을 사용해서 fname의 inode 정보를 얻는다.
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
     * - 상위 디렉토리("..")로 올라가서
     * - 현재 디렉토리 이름을 inode로 찾아내고
     * - 재귀적으로 상위 경로를 먼저 출력한 뒤 현재 이름을 출력
     */
    ino_t my_inode;
    char its_name[BUFSIZ];
    // 루트('/')에 도달하지 않은 경우 
    // (현재 디렉토리와 상위 디렉토리 inode가 다름)
    if (get_inode("..") != this_inode ) {
        chdir(".."); // 한 단계 상위로 이동
        inum_to_name(this_inode, its_name, BUFSIZ); // inode에 해당하는 디렉토리 이름 찾기

        my_inode = get_inode("."); // 상위 디렉토리의 inode 번호 구하기
        printpathto(my_inode); // 재귀 호출로 상위 경로 출력
        printf("/%s", its_name); // 되돌아오며 경로 출력
    } else {
        printf("/"); // 루트면 "/" 출력
    }
}

void inum_to_name(ino_t inode_to_find, char *namebuf, int buflen) {
    /* 
     * looks through current directory for a file with this inode number
     * and copies its name into namebuf
     */
    DIR *dir_ptr; // the directory
    struct dirent *direntp; /* each entry */
    
    // 현재 디렉토리 열기
    dir_ptr = opendir(".");
    if (dir_ptr == NULL ) {
        perror(".");
        exit(1);
    }
    /* 
     * search directory for a file with specified inum
     * 디렉토리 내 엔트리 순회
     */
    while ((direntp = readdir(dir_ptr)) != NULL) {
        // inode 번호가 일치하면 이름 복사 후 반환
        if (direntp->d_ino == inode_to_find) {
            strncpy(namebuf, direntp->d_name, buflen);
            namebuf[buflen-1] = '\0'; /* just in case */
            closedir(dir_ptr);
            return;
        }
    }
    // 끝까지 찾지 못한 경우 오류
    fprintf(stderr, "error looking for inum %ld\n", inode_to_find);
    closedir(dir_ptr);
    exit(1);
}
