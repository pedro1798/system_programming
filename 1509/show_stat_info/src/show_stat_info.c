# include <sys/stat.h>
# include <stdio.h> // printf, perror 

int main(int argc, char *argv[]) {
    struct stat info; // 파일 정보 저장할 구조체
     
    if(argc > 1) { // 인자가 있는 경우 
        if (stat(argv[1], &info) != -1) { // stat 호출 성공 시
            show_stat_info(argv[1], &info); // 파일 이름과 정보 출력하는 함수 호출 
            return 0; // 정상 종료
        } else {
            perror(argv[1]); // stat() 실패 시 에러 메시지 출력 
        }
    }
    return 1; // 인자가 없거나 실패한 경우 비정상 종료 코드 반환
}

/* show_stat_info:
 * struct stat에 들어있는 파일 정보를 사람이 볼 수 있는 형태로 출력한다.
 */
void show_stat_info(char *fname, struct stat *buf) {
    printf(" mode: %o\n", buf->st_mode); // 파일 모드(권한, 파일 종류)
    printf(" links: %d\n", buf->st_nlink); // 하드 링크 수
    printf(" user: %d\n", buf->st_uid); // 소유자 UID
    printf(" group: %d\n", buf->st_gid); // 소유 그룹 GID
    printf(" size: %d\n", buf->st_size); // 파일 크기(바이트)
    printf(" modtime: %ld\n", buf->st_mtime); // 최근 수정 시간 (time_t 정수값)
    printf(" name: %s\n", fname); // 파일 이름 
}
