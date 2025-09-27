#include <sys/types.h>   // 기본 자료형 정의 (uid_t, gid_t 등)
#include <dirent.h>      // 디렉토리 처리 (opendir, readdir)
#include <sys/stat.h>    // 파일 상태 확인 (stat 구조체)
#include <stdio.h>       // 표준 입출력
#include <stddef.h>      // 표준 정의 (NULL 등)
#include <string.h>      // 문자열 처리 함수 (strcpy 등)
#include <grp.h>         // 그룹 정보 (getgrgid)
#include <pwd.h>         // 사용자 정보 (getpwuid)

/* 
 * int stat(const char *pathname, struct stat *statbuf);
 * pathname: 정보를 얻고 싶은 파일 이름 (예: "test.txt")
 * statbuf: struct stat 구조체의 주소. 여기로 파일 정보가 채워져 돌아옴.
 * 리턴값: 성공 시 0, 실패 시 -1 (이때 errno에 오류 코드가 저장됨)
 */


// struct stat 주요 필드 설명
// - st_dev     : 파일이 위치한 장치 ID
// - st_ino     : 아이노드 번호 (고유 식별자)
// - st_mode    : 파일 모드 (종류 + 권한 비트)
// - st_nlink   : 하드 링크 개수
// - st_uid     : 소유자 UID
// - st_gid     : 소유 그룹 GID
// - st_size    : 파일 크기 (바이트 단위)
// - st_blksize : 파일 시스템 I/O 블록 크기
// - st_blocks  : 할당된 디스크 블록 수 (512B 단위)
// - st_atim    : 마지막 접근 시각
// - st_mtim    : 마지막 수정 시각
// - st_ctim    : 아이노드 변경 시각

// 디렉토리 탐색 (디렉토리 이름을 받아 내부 파일 출력)
void do_ls(char dirname[]);

// 파일 정보 출력 (stat 구조체를 이용해 정보 표시)
void dostat(char *filename);

// UID를 사용자 이름으로 변환
char *uid_to_name(uid_t);  

// GID를 그룹 이름으로 변환
char *gid_to_name(gid_t); 

int main(int ac, char *av[]) {
    if (ac == 1) {
        // 인자가 없으면 현재 디렉토리(".")의 내용 출력
        do_ls(".");
    } else {
        for (int i = 1; i < ac; i++) {
            do_ls(av[i]);
        }
    }
}

/*
 * do_ls: 주어진 디렉토리 이름에 대해 파일 목록을 읽고 dostat() 호출
 */
void do_ls(char *path) {
    /*
     * list files in directory called dirname
     */
    struct stat info;

    if (stat(path, &info) == -1) {
        fprintf(stderr, "Error!!");
        perror(path);
        return;
    }

    if (!S_ISDIR(info.st_mode)) {
        // 파일이면 자기 자신 출력
        show_file_info(path, &info);
        return;
    }

    // 디렉토리 처리
    DIR *dir_ptr; /* the directory, 디렉토리 포인터 */ 
    struct dirent *direntp; /* each entry, 디렉토리 엔트리 구조체 */
    
    if ((dir_ptr = opendir(path)) == NULL) {
        perror(path);
        fprintf(stderr, "sp03A: Cannot open %s\n", path);
        return;
    } 
    
    char fullpath[PATH_MAX]; // 상대경로 담을 변수
    // 디렉토리 안 파일들을 하나씩 읽음
    while ((direntp = readdir(dir_ptr)) != NULL) {
        // ".", ".."는 건너뜀
        if (strcmp(direntp->d_name, ".") == 0 || 
            (strcmp(direntp->d_name, "..") == 0)) {
            continue;
        }
        // 파일 이름 전달해 상세 정보 출력
        snprintf(fullpath, sizeof(path), "%s/%s", path, direntp->d_name);
        dostat(fullpath); // dostat 호출!!!!
    }
    closedir(dir_ptr); // 디렉토리 닫기 
}

/* 
 * dostat: 파일의 이름을 받아 stat() 호출 후 show_file_info()로 전달
 */

void dostat(char * filename) {
    struct stat info;
    printf("파일이름: %s\n", filename);
    if (stat(filename, &info) == -1) { /* cannot stat */
        perror(filename);              /* say why */
    } else {                           /* else show info */
        show_file_info(filename, &info); /* 성공하면 정보 출력 */
    }
}

/*
 * show_file_info: stat 구조체를 사람이 읽을 수 있는 형태로 출력
 */
void show_file_info( char * filename, struct stat *info_p) {
    /*
     * display the info about filename. 
     * The info is stored in struct at *info_p
     */
    char *uid_to_name(), *ctime(), *gid_to_name(), *filemode();
    void mode_to_letters();
    char modestr[11]; // 권한을 문자열로 변환한 값 저장 (-rwxr-xr 이렇게)

    mode_to_letters(info_p->st_mode, modestr); // 파일 모드를 문자열로 변환해 modestr에 저장
    
    printf("%s ", modestr); // 파일 유형+권한
    printf("%4d ", (int) info_p->st_nlink); // 하드 링크 수 
    printf("%-8s ", gid_to_name(info_p->st_uid)); // 소유자 
    printf("%-8s ", uid_to_name(info_p->st_gid)); // 그룹
    printf("%8ld ", (long) info_p->st_size); // 파일 크기
    printf("%.12s ", 4+ctime(&info_p->st_mtime)); // 최근 수정 시간
    printf("%s\n", filename); // 파일 이름
}

/*
 * mode_to_letters: 파일 모드를 문자열로 변환(rwx등)
 */
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

/*
 * gid_to_name: GID를 그룹 이름으로 변환
 */
char * gid_to_name(gid_t gid) {
    /* 
     * getgrgid 원형과 포인터 선언.
     * 보통 <grp.h>를 include 하면 함수 원형은 헤더에서 제공되므로
     * 변수만 선언하면 된다.
     * returns pointer to group number gid. used getgrgid(3)
     */
    struct group * getgrgid(), *grp_ptr;
    static char numstr[32];

    if ((grp_ptr = getgrgid(gid)) == NULL) {
        /* 
         * 해당 GID의 그룹 정보가 없으면 숫자를 문자열로 반환
         * 문제점:
         * - sprintf: 버퍼오버플로우 위험
         * - gid_t의 스펙(부호/크기) 때문에 포맷이 안전하지 않을 수 있음
         */
        sprintf(numstr, "%d", gid);
        return numstr;
    } else {
        /* 성공하면 그룹 이름 문자열 반환(라이브러리 내부 정적 메모리) */
        return grp_ptr->gr_name;
    }
}


/*
 * uid_to_name: UID를 사용자 이름으로 변환
 */
char * uid_to_name(uid_t uid) {
    /* 
     * 옛 스타일로 getpwuid 함수 원형을 선언하고 pw_ptr 변수도 같이 선언.
     * modern C에서는 <pwd.h>를 include 하고 아래처럼
     * 변수만 선언하는 편이 낫습니다:
     *   struct passwd *pw_ptr;
     * returns to pointer to username associated with uid, uses getpw()
     */

    struct passwd * getpwuid(), *pw_ptr;
    static char numstr[32];
    /*
     * 반환용 정적 버퍼
     * - static이라서 함수 밖으로 포인터 반환해도 유효(라이프타임 전역)
     *   단, static 버퍼는 스레드 안전하지 않음(다중 스레드에서 덮어씀).
     *   10바이트 크기는 일부 시스템의 큰 UID(예: 32/64비트)에서
     *   부족할 수 있음. (NULL 문자 포함해야 하므로 최소 11바이트
     *   필요할 수 있다.
     */

    if ((pw_ptr = getpwuid(uid)) == NULL) {
        /* getpwuid가 실패했을 때(해당 UID의 사용자 정보가 없을 때) */

        // UID → 문자열 변환
        // ⚠️ sprintf는 오버플로우 위험 → snprintf 권장
        sprintf(numstr, "%d", uid);
        
        return numstr;
    } else {
        /* getpwuid가 성공하면 pw_ptr->pw_name(유저 이름 문자열)을 반환. */
        return pw_ptr->pw_name;
    }
}
