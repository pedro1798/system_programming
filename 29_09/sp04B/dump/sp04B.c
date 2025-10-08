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
 * chmod: 
 * 매개변수로 
 * 파일 받으면 그냥 모드 변경, 디렉토리 받으면 재귀적으로 전부 모드 바꿈
 * 두번째 매개변수는 플래그. 플래그 잘못되면 에러처리
 * 무슨 플래그 허용할지, -나 + if문 분기
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

    mode_t mode;
    sscanf(argv[1], "%o", &mode); // 매개변수로 받은 모드 8진수 변환

    for (int i = 2; i < ac; i++) {
        change_mode(mode, av[i]);
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


}
