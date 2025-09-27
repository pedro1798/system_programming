# sp03A_2022117156.c

#include <sys/types.h>   // 기본 자료형 정의 (uid_t, gid_t 등)
#include <dirent.h>      // 디렉토리 처리 (opendir, readdir)
#include <sys/stat.h>    // 파일 상태 확인 (stat 구조체)
#include <stdio.h>       // 표준 입출력
#include <stddef.h>      // 표준 정의 (NULL 등)
#include <string.h>      // 문자열 처리 함수 (strcpy 등)
#include <grp.h>         // 그룹 정보 (getgrgid)
#include <pwd.h>         // 사용자 정보 (getpwuid)
#include <string.h>      // 문자열 처리 (strcpy, strcmp)
#include <stdlib.h>      // 동적 메모리 할당 (malloc, realloc, free)
#include <time.h>       // 시간 관련 함수 (ctime)
#define PATH_MAX 1024 // redefine PATH_MAX if not defined

void do_ls(char dirname[]);
void show_file_info(char * filename, struct stat *info_p);
char *uid_to_name(uid_t);  
char *gid_to_name(gid_t);
char *ctime(const time_t *timep);
void mode_to_letters(int mode, char str[]);
int compare_person_name(const void *a, const void *b);

struct filename_info { // realloc에 저장할 구조체
    char *name;
    struct stat info;
};
int main(int ac, char *av[]) {
    if (ac < 2) {
    // 파일/디렉토리 이름 없이 명령어만 입력한 경우에는 “.”을 파일 이름으로 입력한 것과 동일하다
        do_ls(".");
    } else {
        for (int i = 1; i < ac; i++) {
            do_ls(av[i]);
        }
    }
}

void do_ls(char *path) {
    struct stat info; // 매개변수로 받은 주소의 stat 구조체 담을 변수
    DIR *dir_ptr; /* the directory, 디렉토리 포인터 */ 
    struct dirent *direntp; /* each entry, 디렉토리 엔트리 구조체 */
    char fullpath[PATH_MAX]; // 상대 경로 담을 변수
    long total_blocks = 0; // total 계산용

    if (stat(path, &info) == -1) { // stat 호출해 info에 파일 정보 저장
        perror(path); // 에러처리1
        fprintf(stderr, "stat(path, &info) failed"); // 에러처리2
        return;
    }

    if (!S_ISDIR(info.st_mode)) { // 매개변수가 디렉토리가 아닌 파일이면 그냥 출력
        show_file_info(path, &info);
        return; // 출력 후 종료
    }

    if ((dir_ptr = opendir(path)) == NULL) { // 디렉토리 열기
        perror(path); // 에러 처리
        fprintf(stderr, "sp03A: Cannot open %s\n", path);
        return;
    } 
    
    struct filename_info *files_in_directory = NULL; // 동적 할당으로 파일 담아서 정렬할 구조체 포인터
    int file_idx = 0; // realloc 인덱스
    int capacity = 0; // realloc 사이즈

    while ((direntp = readdir(dir_ptr)) != NULL) { // 디렉토리 포인터에서 디렉토리 엔트리 포인터가 읽어지면
        snprintf(fullpath, PATH_MAX, "%s/%s", path, direntp->d_name); // direntp의 상대경로 저장
        
        struct stat entry_info; // 각 엔트리의 파일 상태 정보를 담을 구조체
        if (stat(fullpath, &entry_info) == -1) { // 에러처리
            perror(fullpath);
            continue;
        }
        
        if (file_idx == capacity) { // realloc 용량 처리
            capacity = (capacity == 0) ? 32 : capacity * 2; // 초기 용량 32로 설정 및 두 배로 증가

            struct filename_info *tmp = realloc(files_in_directory, sizeof(struct filename_info) * capacity);
            // realloac 실패 처리
            if (!tmp) {
                perror("realloc");
                // 기존 메모리 해제
                for (int i = 0; i < file_idx; i++) {
	                  // 동적 할당한 name 해제
	                  free(files_in_directory[i].name);
	              }
                free(files_in_directory); // 동적 할당한 배열 해제
                closedir(dir_ptr); // 디렉토리 포인터 close
                return;
            }
            files_in_directory = tmp;
        } 
        
        files_in_directory[file_idx].info = entry_info; // 배열에 넣기 위해 assign
        // strdup: 새로운 메모리를 힙에 할당하고 문자열을 복사
        files_in_directory[file_idx].name = strdup(direntp->d_name); // 
        // strdup 실패 처리
        if (!files_in_directory[file_idx].name) {
            perror("strdup");
            // 기존 메모리 해제
            for (int i = 0; i < file_idx; i++) free(files_in_directory[i].name);
            free(files_in_directory);
            closedir(dir_ptr);
            return;
        }

        file_idx++; // 파일 인덱스 증가, 부족하면 realloc

        total_blocks += entry_info.st_blocks; // total(파일이 차지하는 블록, ls는 1Kb/block) 계산
    }
    // files_in_directory 배열에 저장된 파일들 정렬해서 출력
    qsort(files_in_directory, file_idx, sizeof(struct filename_info), compare_person_name);
    if (strcmp(path, ".") != 0) { // .: ls -al과 비슷하게 출력 생략
        printf("%s:\n", path);
    }

    // ############################## 출력부 ##############################
    printf("total %ld\n", total_blocks); // ls는 1K 단위로 표시
    for (int i = 0; i < file_idx; i++) {
        // files_in_directory 속 파일들 출력  
        show_file_info(files_in_directory[i].name, &files_in_directory[i].info);
    }
    printf("\n");
    // ############################## 출력부 ##############################
    
    // 디렉토리 DFS 재귀 호출
    rewinddir(dir_ptr); // dir_ptr을 디렉토리 처음으로 되돌린다
    while ((direntp = readdir(dir_ptr)) != NULL) {
        if (strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0) {
            continue; // 기저조건
        }

        snprintf(fullpath, PATH_MAX, "%s/%s", path, direntp->d_name); // 상대경로 저장

        struct stat entry_info; // 디렉토리 엔트리 stat 저장할 구조체
        if (stat(fullpath, &entry_info) == -1) { // 실패하면 continue
            continue;
        }

        if (S_ISDIR(entry_info.st_mode)) { // 디렉토리면 재귀(DFS)
            do_ls(fullpath);
        }
    }
    // 메모리 해제
    for (int i = 0; i < file_idx; i++)
        free(files_in_directory[i].name); // 동적 할당한 원소 해제
    free(files_in_directory); // 동적 할당한 배열 해제
    closedir(dir_ptr); // 디렉토리 포인터 close
}

void show_file_info(char *filename, struct stat *info_p) {
		// 파일 정보 출력 함수 
    char modestr[11]; // 권한 문자열 (-rwxr-xr-x)
    mode_to_letters(info_p->st_mode, modestr); // (파일 모드를 rwx로 변환)

    /*
    // ctime()은 문자열 끝에 \n이 포함되어 있으므로 직접 자르기
    char *time_str = ctime(&info_p->st_mtime);
    if (time_str) time_str[24] = '\0'; // YYYY-MM-DD HH:MM:SS 형태로 자름
    */
    
    /*
     * •	info_p->st_mtime → 파일 메타데이터(struct stat)에 저장된 마지막 수정 시간
	   * •	localtime(&info_p->st_mtime) → 이 time_t 값을 사람이 읽기 좋은 지역 시간 구조체(struct tm)로 변환
    */
    time_t now = time(NULL); // time_t 자료형 변수 선언
    struct tm *tm_info = localtime(&info_p->st_mtime); // 파일의 마지막 수정 시간
    char time_str[13]; // "Mon DD HH:MM" 또는 "Mon DD  YYYY"

    if (difftime(now, info_p->st_mtime) > 15552000 || difftime(info_p->st_mtime, now) > 0) {
        // 6개월 이상 차이 or 미래 시간 → 년도 표시
        // strftime은 struct tm에 저장된 시간 정보를 사람이 읽을 수 있는 문자열로 변환하는 C 표준 라이브러리 함수
        strftime(time_str, sizeof(time_str), "%b %e  %Y", tm_info);
    } else {
        // 6개월 이내 → 시간 표시
        strftime(time_str, sizeof(time_str), "%b %e %H:%M", tm_info);
    }

    printf("%s "               // 권한
           "%2ld "             // 하드링크 수
           "%-8s "             // 소유자
           "%-8s "             // 그룹
           "%1ld "             // 파일 크기
           "%s "               // 수정 시간
           "%s\n",             // 파일 이름
           modestr,
           (long)info_p->st_nlink,
           uid_to_name(info_p->st_uid),
           gid_to_name(info_p->st_gid),
           (long)info_p->st_size,
           time_str + 4,       // ctime 반환 문자열에서 "Mon DD HH:MM" 부분만
           filename);
}

// qsort에 매개변수로 넘길 비교 함수
int compare_person_name(const void *a, const void *b) {
    const struct filename_info *pa = (const struct filename_info *)a;
    const struct filename_info *pb = (const struct filename_info *)b;
    return strcasecmp(pa->name, pb->name);  // strcmp: 알파벳 순 비교, 대문자 우선, strcasecmp: 대소문자 구분 없이 비교
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
