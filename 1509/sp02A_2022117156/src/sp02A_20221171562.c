# include <sys/types.h> /* 시스템 자료형 라이브러리 (uid_t, gid_t 등) */
# include <dirent.h> /* 디렉토리 처리 함수 (opendir, readdir 등)  */
# include <sys/stat.h> /* 파일 상태 확인용 (stat 구조체) */
# include <stdio.h> 
# include <stddef.h>
# include <string.h> /* 문자열 처리 함수 (strcpy 등) */

/* int stat(const char *pathname, struct stat *statbuf);
 * pathname: 정보를 얻고 싶은 파일 이름 (예: "test.txt")
 * statbuf: struct stat 구조체의 주소. 여기로 파일 정보가 채워져 돌아옴.
 * 리턴값: 성공 시 0, 실패 시 -1 (이때 errno에 오류 코드가 저장됨)
 */

/*
st_dev - 파일이 위치한 장치 ID
st_ino - 파일의 아이노드 번호 (고유 식별자)
st_mode - 파일 모드 (파일 종류 + 접근 권한)
st_nlink - 하드 링크 개수
st_uid - 소유자 UID
st_gid - 소유 그룹 GID
st_rdev - 특수 파일의 장치 ID (문자/블록 장치일 때)
st_size - 파일 크기 (바이트 단위)
st_blksize - 파일 시스템 I/O 블록 크기 (성능 힌트)
st_blocks - 할당된 디스크 블록 수 (512B 단위)
st_atim - 마지막 접근 시각 (struct timespec)
st_mtim - 마지막 수정 시각 (struct timespec)
st_ctim - 아이노드 상태 변경 시각 (struct timespec)
*/

/* 동작 과정
 * 1.stat("파일명", &info) 호출 → 커널이 파일 시스템에 요청 보냄 
 * 2.커널은 해당 파일의 아이노드(inode) 정보를 읽음
 *  • 아이노드에는 파일의 데이터가 아니라 메타데이터가 저장돼 있음
 * 3. 그 정보를 struct stat info 구조체에 채워줌
 * 4. 프로그램은 info.st_mode, info.st_size, info.st_uid 같은 필드를 사용해서 필요한 정보를 가져옴
 */

void do_ls(char[]); // 디렉토리 탐색
void dostat(char *); // 파일 정보 출력
void show_file_info(char *, struct stat *); // stat 구조체를 사람이 읽을 수 있게 출력 
void mode_to_letters(int, char[]); // 파일 모드를 rwx 등 문자로 변환
char *uid_to_name(uid_t); // UID를 사용자 이름으로 변환 
char *gid_to_name(gid_t); // GID를 그룹 이름으로 변환 

int main(int ac, char *av[]) {
    if (ac == 1) {
        // 인자가 없으면 현재 디렉토리(".")의 내용 출력
        do_ls(".");
    } else {
        for (int i = 1; i < ac; i++) {
            printf("%s:\n", av[i]);
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
        fprintf(stderr, "cannot open %s\n", path);
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
        dostat(fullpath);
    }
    closedir(dir_ptr); // 디렉토리 닫기 
}

/* 
 * dostat: 파일의 이름을 받아 stat() 호출 후 show_file_info()로 전달
 */

void dostat(char * filename) {
    struct stat info;
    
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
    
    // 파일 종류
    printf("pathname: \"%s\"\n", filename);
    printf("type: ");
    if (S_ISREG(info_p->st_mode)) printf("regular file\n");
    else if (S_ISDIR(info_p->st_mode)) printf("directory\n");
    else if (S_ISCHR(info_p->st_mode)) printf("character device\n");
    else if (S_ISBLK(info_p->st_mode)) printf("block device\n");
    else if (S_ISFIFO(info_p->st_mode)) printf("FIFO\n");
    else if (S_ISLNK(info_p->st_mode)) printf("symbolic link\n");
    else if (S_ISSOCK(info_p->st_mode)) printf("socket\n");
    else printf("unknown\n");

    // 권한: 8진수 3자리
    printf("mode: %o\n", info_p->st_mode & 0777);

    // 나머지 정보
    printf("inode #: %lu\n", (unsigned long)info_p->st_ino);
    printf("number of links = %lu\n", (unsigned long)info_p->st_nlink);
    printf("uid = %u\n", info_p->st_uid);
    printf("gid = %u\n", info_p->st_gid);
    printf("size = %ld\n", (long)info_p->st_size);
    printf("preferred I/O block size = %ld\n", (long)info_p->st_blksize);
    printf("number of 512-byte blocks = %ld\n", (long)info_p->st_blocks);
    printf("-----------------------------------------\n");
}

/*
 * mode_to_letters: 파일 모드를 문자열로 변환(rwx등)
 */
void mode_to_letters(int mode, char str[]) {
    strcpy(str, "----------");

    if (S_ISDIR(mode)) str[0] = 'd'; // 디렉토리인가?
    if (S_ISCHR(mode)) str[0] = 'c'; // 문자 디바이스인가?
    if (S_ISBLK(mode)) str[0] = 'b'; // 블록 디바이스인가?
    
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
