#include <sys/types.h>   // 기본 자료형 정의 (uid_t, gid_t 등)
#include <dirent.h>      // 디렉토리 처리 (opendir, readdir)
#include <sys/stat.h>    // 파일 상태 확인 (stat 구조체)
#include <stdio.h>       // 표준 입출력
#include <stddef.h>      // 표준 정의 (NULL 등)
#include <string.h>      // 문자열 처리 함수 (strcpy 등)
#include <grp.h>         // 그룹 정보 (getgrgid)
#include <pwd.h>         // 사용자 정보 (getpwuid)
#include <stdlib.h>      // 동적 메모리 할당 (malloc, realloc, free)
#include <time.h>       // 시간 관련 함수 (ctime)
#include <stdbool.h>

#define PATH_MAX 1024 // redefine PATH_MAX if not defined
#define MAX_LINE 1024
void do_find(char* name, char* mode);
void show_file_info(char * filename, struct stat *info_p);
char *uid_to_name(uid_t);  
char *gid_to_name(gid_t);
void mode_to_letters(int mode, char str[]);
bool check_mode(char* path, char* param_mode);

int main(int ac, char *av[]) {
    if (ac < 2) { // 매개변수가 없을 때
        do_find('.', '-1');
    } else {
        int idx_mode = -1;
        for (int i = 1; i < ac; i++) {
            if (av[1] >= '0' && av[i] <= '7') {
                idx_mode = i;
            }
        }
        if (ac == 2) { // 매개변수 하나일 때 
            if (idx_mode == -1) { // 모드 매개변수 없을 때
                do_find(av[1], '-1');
            } else { // 모드 있을 때
                do_find('.', av[1]);
            }
        } else { // 매개변수 둘 이상일 때
            if (idx_mode == -1) { // 모드가 없다면
                do_find(av[1], '-1');
            } else { // 모드가 있다면
                if (idx_mode == 1) {
                    do_ls(av[2], av[1]); 
                } else if (idx_mode == 2) {
                    do_ls(av[1], av[2]);
                }
            }
        }
    }

    return 0;
}

void do_find(char* name, char* mode) {
    struct stat info;
    DIR *dir_ptr;
    struct dirent *direntp;
    char fullpath[PATH_MAX];
    
    if ((dir_ptr = opendir('.')) == NULL) { // 현재 디렉토리 열기
        perror('.');
        fprintf(stderr, "Cannot open \n");
        return;
    }

    if (name == '.') { // mode만 따지거나 그냥 모두 출력 
        while ((direntp = readdir(dir_ptr)) != NULL) { // 현재 디렉토리에서 파일 읽기
            snprintf(fullpath, PATH_MAX, "%s/%s", '.', direntp->d_name); // 모든 이름 FULLPATH에 저장   
            
            struct stat entry_info; // 각 엔트리의 파일 상태 정보를 담을 구조체
            
            if (stat(fullpath, &entry_info) == -1) {
                perror(fullpath);
                continue;
            }
            
            if (check_mode(fullpath, mode)) {
                // 출력
                show_file_info(filename, &entry_info);
            } 
        }
    } else { // 이름 검색하고 모드 따지고 출력
        while ((direntp = readdir(dir_ptr)) != NULL) { // 현재 디렉토리에서 파일 읽기
            // name이 direntp->d_name에 매치되면
            if (strstr(direntp->d_name, name)) {    
                snprintf(fullpath, PATH_MAX, "%s/%s", '.', direntp->d_name); // 매치된 이름 FULLPATH에 저장   
                struct stat entry_info; // 각 엔트리의 파일 상태 정보를 담을 구조체
                
                if (stat(fullpath, &entry_info) == -1) {
                    perror(fullpath);
                    continue;
                }
                if (check_mode(fullpath, mode)) {
                    show_file_info(filename, &entry_info);
                } 
            } 
        }
    }
}

bool check_mode(char* path, char* param_mode) {
    struct stat info;
    
    if (param_mode == '-1') {
        return 1;
    }  

    if (stat(path, &info) == -1) { // 예외처리
        fprintf(stderr, "chmod failed on %s: ", path);
        perror(path);
        return;
    }

    mode_t mode;
    sscanf(param_mode, "%o", &mode);  // 문자열을 8진수 정수로 변환

    if (info.st_mode & mode) {
        return 1;
    }

    return -1;
}


void show_file_info(char *filename, struct stat *info_p) {
    char modestr[11]; // 권한 문자열 (-rwxr-xr-x)
    mode_to_letters(info_p->st_mode, modestr);

    /*
    // ctime()은 문자열 끝에 \n이 포함되어 있으므로 직접 자르기
    char *time_str = ctime(&info_p->st_mtime);
    if (time_str) time_str[24] = '\0'; // YYYY-MM-DD HH:MM:SS 형태로 자름
    */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&info_p->st_mtime);
    char time_str[13]; // "Mon DD HH:MM" 또는 "Mon DD  YYYY"

    if (difftime(now, info_p->st_mtime) > 15552000 || difftime(info_p->st_mtime, now) > 0) {
        // 6개월 이상 차이 or 미래 시간 → 년도 표시
        strftime(time_str, sizeof(time_str), "%b %e  %Y", tm_info);
    } else {
        // 6개월 이내 → 시간 표시
        strftime(time_str, sizeof(time_str), "%b %e %H:%M", tm_info);
    }

    printf("%s "               // 권한
           "%4d "             // 하드링크 수
           "%-8s "             // 소유자
           "%-8s "             // 그룹
           "%8ld "             // 파일 크기
           "%.12s "               // 수정 시간
           "%s\n",             // 파일 이름
           modestr,
           info_p->st_nlink,
           uid_to_name(info_p->st_uid),
           gid_to_name(info_p->st_gid),
           (long)info_p->st_size,
           time_str + 4,       // ctime 반환 문자열에서 "Mon DD HH:MM" 부분만
           filename);
}
void mode_to_letters(int mode, char str[]) {
    strcpy(str, "----------");

    if (S_ISDIR(mode)) str[0] = 'd'; // 디렉토리인가?
    if (S_ISCHR(mode)) str[0] = 'c'; // 문자 디바이스인가?
    if (S_ISBLK(mode)) str[0] = 'b'; // 블록 디바이스인가?
    if (S_ISFIFO(mode)) str[0] = 'p'; // 파이프(FIFO)인가?
    if (S_ISLNK(mode)) str[0] = 'l'; // 심볼릭 링크인가?
    if (S_ISSOCK(mode)) str[0] = 's'; // 소켓인가?

    // 소유자 read, write, execute
    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) str[3] = 'x';

    // 그룹 read, write, execute
    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) str[6] = 'x';

    // 기타 사용자 read, write, execute
    if (mode & S_IROTH) str[7] = 'r';
    if (mode & S_IWOTH) str[8] = 'w';
    if (mode & S_IXOTH) str[9] = 'x';
}

char * gid_to_name(gid_t gid) {
		// 파일의 그룹 ID (GID) 를 그룹 이름으로 바꿔서 반환
    struct group * grp_ptr;
    static char numstr[32];

    if ((grp_ptr = getgrgid(gid)) == NULL) {
        sprintf(numstr, "%d", gid); // GID가 시스템에 없으면 숫자 그대로 문자열로 변환
        return numstr;
    } else {
        return grp_ptr->gr_name; // 있으면 그룹 이름 반환
    }
}

char * uid_to_name(uid_t uid) {
		// 파일의 사용자 ID (UID) 를 사용자 이름으로 바꿔서 반환
    struct passwd * pw_ptr;
    static char numstr[32];

    if ((pw_ptr = getpwuid(uid)) == NULL) {
        sprintf(numstr, "%d", uid); // // UID가 시스템에 없으면 숫자 그대로 문자열로 변환
        return numstr;
    } else {
        return pw_ptr->pw_name; // // 있으면 사용자 이름 반환
    }
}
