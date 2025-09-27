#include <sys/types.h>   // 기본 자료형 정의 (uid_t, gid_t 등)
#include <dirent.h>      // 디렉토리 처리 (opendir, readdir)
#include <sys/stat.h>    // 파일 상태 확인 (stat 구조체)
#include <stdio.h>       // 표준 입출력
#include <stddef.h>      // 표준 정의 (NULL 등)
#include <string.h>      // 문자열 처리 함수 (strcpy 등)
#include <grp.h>         // 그룹 정보 (getgrgid)
#include <pwd.h>         // 사용자 정보 (getpwuid)

void do_ls(char dirname[]);
void show_file_info(char * filename, struct stat *info_p);
char *uid_to_name(uid_t);  
char *gid_to_name(gid_t); 

int main(int ac, char *av[]) {
    if (ac == 1) {
        do_ls(".");
    } else {
        for (int i = 1; i < ac; i++) {
            do_ls(av[i]);
        }
    }
}

void do_ls(char *path) {
    struct stat info;
    DIR *dir_ptr; /* the directory, 디렉토리 포인터 */ 
    struct dirent *direntp; /* each entry, 디렉토리 엔트리 구조체 */

    if (stat(path, &info) == -1) { // stat 호출  
        perror(path);
        fprintf(stderr, "stat(path, &info) failed");
        return;
    }

    if (S_ISDIR(info.st_mode)) {
        printf("\n%s:\n", path);
    } else { // 디렉토리가 아니면 출력하고 종료 
        show_file_info(path, &info);
        return;
    }

    if ((dir_ptr = opendir(path)) == NULL) {
        perror(path);
        fprintf(stderr, "sp03A: Cannot open %s\n", path);
        return;
    } 
    while ((direntp = readdir(dir_ptr)) != NULL) {
        char fullpath[PATH_MAX]; // 상대경로 담을 변수 
        snprintf(fullpath, PATH_MAX, "%s/%s", path, direntp->d_name);
        
        struct stat entry_info;
        if (stat(fullpath, &entry_info) == -1) {
            perror(fullpath);
            continue;
        }
        
        show_file_info(fullpath, &entry_info);
        
        if (S_ISDIR(entry_info.st_mode) &&
            strcmp(direntp->d_name, ".") != 0 &&
            strcmp(direntp->d_name, "..") != 0) {
            do_ls(fullpath);
        }
    }
    closedir(dir_ptr);
}

void show_file_info( char * filename, struct stat *info_p) {
    char *uid_to_name(), *ctime(), *gid_to_name(), *filemode();
    void mode_to_letters();
    char modestr[11]; // 권한을 문자열로 변환한 값 저장 (-rwxr-xr 이렇게)

    mode_to_letters(info_p->st_mode, modestr); // 파일 모드를 문자열로 변환해 modestr에 저장

    printf("%s ", modestr); // 파일 유형+권한
    printf("%4d ", (int) info_p->st_nlink); // 하드 링크 수 
    printf("%-8s ", uid_to_name(info_p->st_uid)); // 소유자 
    printf("%-8s ", gid_to_name(info_p->st_gid)); // 그룹
    printf("%8ld ", (long) info_p->st_size); // 파일 크기
    printf("%.12s ", 4+ctime(&info_p->st_mtime)); // 최근 수정 시간
    printf("%s\n", filename); // 파일 이름
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
    struct group * getgrgid(), *grp_ptr;
    static char numstr[32];

    if ((grp_ptr = getgrgid(gid)) == NULL) {
        sprintf(numstr, "%d", gid);
        return numstr;
    } else {
        return grp_ptr->gr_name;
    }
}

char * uid_to_name(uid_t uid) {
    struct passwd * getpwuid(), *pw_ptr;
    static char numstr[32];

    if ((pw_ptr = getpwuid(uid)) == NULL) {
        sprintf(numstr, "%d", uid);
        
        return numstr;
    } else {
        return pw_ptr->pw_name;
    }
}
