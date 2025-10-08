#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>

void change_mode(char *mode_param, const char *path);

mode_t parse_mode(const char *mode_str, mode_t old_mode);

int main(int ac, char *av[]) {
    if (ac < 3) {
        fprintf(stderr, "Usage: %s <mode> <file/dir>...\n", av[0]);
        return -1;
    }
    
    for (int i = 2; i < ac; i++) {
        change_mode(av[1], av[i]);
    }

    return 0;
}

void change_mode(char *mode_param, const char *path) {
    struct stat info;
    
    if (stat(path, &info) == -1) {
        perror(path);
        return;
    }
    
    mode_t mode = parse_mode(mode_param, info.st_mode);

    // 현재 경로 chmod
    if (chmod(path, mode) == -1) {
        perror("chmod failed");
    }

    // 디렉토리가 아니면 끝
    if (!S_ISDIR(info.st_mode)) {
        return;
    }

    DIR *dir_ptr;
    struct dirent *direntp;

    if ((dir_ptr = opendir(path)) == NULL) {
        perror(path);
        return;
    }

    char fullpath[PATH_MAX];

    while ((direntp = readdir(dir_ptr)) != NULL) {
        if (strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0)
            continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, direntp->d_name);

        struct stat entry_info;
        if (stat(fullpath, &entry_info) == -1) {
            perror(fullpath);
            continue;
        }

        // 파일이든 디렉토리든 chmod 적용
        if (chmod(fullpath, mode) == -1) {
            perror(fullpath);
        }

        // 하위 디렉토리면 재귀 호출
        if (S_ISDIR(entry_info.st_mode)) {
            change_mode(mode, fullpath);
        }
    }

    closedir(dir_ptr);
}

mode_t parse_mode(const char *mode_str, mode_t old_mode) {
    // 숫자 모드일 경우
    if (mode_str[0] >= '0' && mode_str[0] <= '7') {
        mode_t mode;
        sscanf(mode_str, "%o", &mode);  // 문자열을 8진수 정수로 변환
        return mode;
    }

    // 문자 모드일 경우
    mode_t new_mode = old_mode;  // 현재 권한 기준으로 수정
    const char *p = mode_str;

    while (*p) {
        int who = 0;   // u, g, o, a
        int op = 0;    // '+', '-', '='
        int perm = 0;  // r, w, x 비트

        // ① 대상자(u,g,o,a) 파싱
        while (*p == 'u' || *p == 'g' || *p == 'o' || *p == 'a') {
            switch (*p) {
                case 'u': who |= 1; break;
                case 'g': who |= 2; break;
                case 'o': who |= 4; break;
                case 'a': who |= 7; break;
            }
            p++;
        }

        // 기본값 (who 없으면 a)
        if (who == 0)
            who = 7;

        // ② 연산자 파싱
        if (*p == '+' || *p == '-' || *p == '=') {
            op = *p;
            p++;
        } else {
            fprintf(stderr, "Invalid mode format: missing operator (+, -, =)\n");
            return old_mode;
        }

        // ③ 권한 비트 파싱
        while (*p == 'r' || *p == 'w' || *p == 'x') {
            switch (*p) {
                case 'r': perm |= 4; break;  // 읽기
                case 'w': perm |= 2; break;  // 쓰기
                case 'x': perm |= 1; break;  // 실행
            }
            p++;
        }

        // ④ 비트 조작
        for (int i = 0; i < 3; i++) {  // u,g,o 순서
            if (!(who & (1 << i))) continue;

            mode_t mask = 0;
            switch (perm) {
                case 4: mask = (i == 0) ? S_IRUSR : (i == 1) ? S_IRGRP : S_IROTH; break;
                case 2: mask = (i == 0) ? S_IWUSR : (i == 1) ? S_IWGRP : S_IWOTH; break;
                case 1: mask = (i == 0) ? S_IXUSR : (i == 1) ? S_IXGRP : S_IXOTH; break;
                default:
                    mask = 0;
                    if (perm & 4) mask |= (i == 0 ? S_IRUSR : i == 1 ? S_IRGRP : S_IROTH);
                    if (perm & 2) mask |= (i == 0 ? S_IWUSR : i == 1 ? S_IWGRP : S_IWOTH);
                    if (perm & 1) mask |= (i == 0 ? S_IXUSR : i == 1 ? S_IXGRP : S_IXOTH);
                    break;
            }

            if (op == '+')
                new_mode |= mask;
            else if (op == '-')
                new_mode &= ~mask;
            else if (op == '=')
                new_mode = (new_mode & ~(mask)) | mask;
        }

        if (*p == ',') p++;  // 여러 연산자 구분자 처리
    }

    return new_mode;
}
