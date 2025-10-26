// cat
#include <sys/types.h>   // 기본 자료형 정의 (uid_t, gid_t 등)
#include <dirent.h>      // 디렉토리 처리 (opendir, readdir)
#include <unistd.h>
#include <sys/stat.h>    // 파일 상태 확인 (stat 구조체)
#include <stdio.h>       // 표준 입출력
#include <stddef.h>      // 표준 정의 (NULL 등)
#include <string.h>      // 문자열 처리 함수 (strcpy 등)
#include <grp.h>         // 그룹 정보 (getgrgid)
#include <pwd.h>         // 사용자 정보 (getpwuid)
#include <stdlib.h>      // 동적 메모리 할당 (malloc, realloc, free)
#include <time.h>       // 시간 관련 함수 (ctime)
#include <fcntl.h>
// #define PATH_MAX 1024 // redefine PATH_MAX if not defined
#define BUFFERSIZE 1024

void do_ls(char dirname[]);
void show_file_info(char * filename, struct stat *info_p);
char *uid_to_name(uid_t);  
char *gid_to_name(gid_t);
void mode_to_letters(int mode, char str[]);
int compare_person_name(const void *a, const void *b);

struct filename_info { // realloc에 저장할 구조체
    char *name;
    struct stat info;
};

int main(int ac, char *av[]) {
    if (ac < 2) {
        do_ls(".");
    } else {
        for (int i = 1; i < ac; i++) {
            do_ls(av[i]);
        }
    }
    return 0;
}

void do_ls(char *path) {
    struct stat info;
    DIR *dir_ptr; /* the directory, 디렉토리 포인터 */ 
    struct dirent *direntp; /* each entry, 디렉토리 엔트리 구조체 */
    char fullpath[PATH_MAX]; // 상대 경로 담을 변수
    long total_blocks = 0; // total 계산용
    int in_fd, out_fd, n_chars;
    char buf[BUFFERSIZE];
    
    if (stat(path, &info) == -1) { // stat 호출해 info에 파일 정보 저장
        perror(path);
        fprintf(stderr, "stat(path, &info) failed");
        return;
    }

    if (!S_ISDIR(info.st_mode)) { // 매개변수가 디렉토리가 아니면
        /* 출력하기  */
        if (in_fd = open(path, O_RDONLY) == -1) {
            printf("입력오류");
            perror(path);
        }
        printf("in_fd is %d", in_fd);
        
        while((n_chars = read(in_fd, buf, BUFFERSIZE)) > 0) {
            if (write(0, buf, n_chars) != n_chars) {
                fprintf(stderr, "read error");
            }
        }
        printf("test%s\n", buf);
        //show_file_info(path, &info);
        return;
    }

    if ((dir_ptr = opendir(path)) == NULL) { // 디렉토리 열기
        perror(path);
        fprintf(stderr, "sp03A: Cannot open %s\n", path);
        return;
    } 
    
    struct filename_info *files_in_directory = NULL;
    int file_idx = 0;
    int capacity = 0;

    while ((direntp = readdir(dir_ptr)) != NULL) {
        snprintf(fullpath, PATH_MAX, "%s/%s", path, direntp->d_name); // direntp의 상대경로 저장
        
        struct stat entry_info; // 각 엔트리의 파일 상태 정보를 담을 구조체
        if (stat(fullpath, &entry_info) == -1) {
            perror(fullpath);
            continue;
        }
        
        if (file_idx == capacity) {
            capacity = (capacity == 0) ? 32 : capacity * 2; // 초기 용량 32로 설정 및 두 배로 증가

            struct filename_info *tmp = realloc(files_in_directory, sizeof(struct filename_info) * capacity);
            // realloac 실패 처리
            if (!tmp) {
                perror("realloc");
                // 기존 메모리 해제
                for (int i = 0; i < file_idx; i++) free(files_in_directory[i].name);
                free(files_in_directory);
                closedir(dir_ptr);
                return;
            }
            files_in_directory = tmp;
        } 
        
        files_in_directory[file_idx].info = entry_info;

        files_in_directory[file_idx].name = strdup(direntp->d_name);
        // strdup 실패 처리
        if (!files_in_directory[file_idx].name) {
            perror("strdup");
            // 기존 메모리 해제
            for (int i = 0; i < file_idx; i++) free(files_in_directory[i].name);
            free(files_in_directory);
            closedir(dir_ptr);
            return;
        }

        file_idx++;

        total_blocks += entry_info.st_blocks; // total 계산
    }
    // files_in_directory 배열에 저장된 파일들 정렬해서 출력
    qsort(files_in_directory, file_idx, sizeof(struct filename_info), compare_person_name);
    if (strcmp(path, ".") != 0) { // .: 출력 생략
        printf("%s:\n", path);
    }

    printf("total %ld\n", total_blocks); // st_blocks는 512바이트 단위, ls는 1K 단위로 표시
    for (int i = 0; i < file_idx; i++) {
        show_file_info(files_in_directory[i].name, &files_in_directory[i].info);
    }
    printf("\n");
    
    // 정렬된 리스트에서 디렉토리 DFS 재귀 호출
    for (int i = 0; i < file_idx; i++) {
        if (S_ISDIR(files_in_directory[i].info.st_mode) &&
            strcmp(files_in_directory[i].name, ".") != 0 &&
            strcmp(files_in_directory[i].name, "..") != 0) {

            char subpath[PATH_MAX];
            snprintf(subpath, PATH_MAX, "%s/%s", path, files_in_directory[i].name);
            do_ls(subpath);
        }
    }

    // 메모리 해제
    for (int i = 0; i < file_idx; i++)
        free(files_in_directory[i].name);
    free(files_in_directory);
    closedir(dir_ptr);
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
           "%4ld "             // 하드링크 수
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

// qsort에서 사용할 비교 함수
int compare_person_name(const void *a, const void *b) {
    const struct filename_info *pa = (const struct filename_info *)a;
    const struct filename_info *pb = (const struct filename_info *)b;
    // strcmp: 알파벳 순 비교, 대문자 우선, strcasecmp: 대소문자 구분 없이 비교
    return strcoll(pa->name, pb->name);  
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
