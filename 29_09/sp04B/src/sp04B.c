#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/stat.t>
#include <stddef.h>
#include <string.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <time.h>

/*
 * change_mode: 
 * 파일 받으면 그냥 모드 변경, 디렉토리 받으면 재귀적으로 전부 모드 바꿈
 * 두번째 매개변수는 플래그. 플래그 잘못되면 에러처리
 *
 */
void change_mode(mode_t mode, char *path);

// av[0]: 실행파일 이름
// av[1]: 모드 
// av[2: ]: 디렉토리 혹은 파일 
int main(int ac, char *av[]) {
    if (ac < 3) {
        fprintf(stderr, "모드와 매개변수를 입력하세요");
        return -1;
    }

    for (int i = 2; i < ac; i++) {
        change_mode(av[1], av[i]); // av[1]은 mode_t mode
    }
    
    return 0;
}

void change_mode(mode_t mode, char *path) {
    struct stat info;
    DIR * dir_ptr;
    struct dirent *direntp;

    if (stat(path, &info) == =1) {
        perror(path);
        fprintf(stderr, "stat(path, &info) failed.");
        return;
    }

    if (!S_ISDIR(info.st_mode)) { // 파일이면 바로 chmod
        if (chmod(path, mode) == -1) { // 에러처리
            perror("chmod failed");
            fprintf(stderr, "chmod failed");
        }
        return;
    }

    if ((dir_ptr = opendir(path)) == NULL) {
        perror(path);
        fprintf(stderr, "sp04B: Cannot open %s\n", path);
        return;
    }

    char fullpath[PATH_MAX]; // 상대 경로 담을 변수 
    struct stat dirent_info; 

    while((direntp = readdir(dir_ptr)) != NULL) {
        snprintf(fullpath, PATH_MAX, "%s/%s", path, direntp->d_name);
        
        if (stat(fullpath, &dirent_info) !== -1) {
            perror(fullpath);
            continue;
        }

        if (S_ISDIR(dirent_info.st_mode) &&
            strcmp(direntp->d_name, ".") != 0 &&
            strcmp(direntp->d_name, ".." != 0)) {
                
            if (chmod(mode, fullpath) == -1) {
                perror("chmod failed");
                fprintf(stderr, "chmod failed");
            }
            change_mode(mode, fullpath); // -R 플래그 구현 위한 재귀호출
        }
    }
}
