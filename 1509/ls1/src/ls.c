#include <stdio.h> /* fprintf등 사용 */
#include <sys/types.h> /* 데이터 타입 정의 (DIR, struct dirent 등에서 필요)  */
#include <dirent.h> /* 디렉토리 처리 함수 */

void do_ls(char [], char *);

int main(int ac, char *av[]){
    char * cmd = av[0]; /* 실행된 명령어 이름 저장  */
    
    if (ac == 1) { /* 인자 없는 경우  */
        do_ls(".", cmd); /* 현재 디렉토리 내용 출력 */
    } else { /* 인자를 받으면 */
        while (--ac) { /* 인자 개수만큼 반복  */
            printf("%s:\n", *++av); /* 현재 처리할 디렉토리 이름 출력 */
            do_ls(*av, cmd); /* do_ls 함수로 해당 디렉토리 내용 출력 */
        }

    }
}

/* 디렉토리 내용 출력 함수 */
void do_ls(char dirname[], char * cmd) {
    DIR *dir_ptr; // the directory /* 디렉토리 지시하는 포인터 */
    struct dirent *direntp; /* each entry, 디렉토리 엔트리 구조체 포인터 (각 파일/디렉토리 항목 */

    /* 열기 실패한 경우 에러처리, stderr 스트림 사용  */
    if ((dir_ptr = opendir(dirname)) == NULL) {
        fprintf(stderr, "%s: ls1: cannot open %s\n", cmd, dirname); 
    } else { /* 실행 명령어 이름과 해당 항목 이름 출력 */
        while ((direntp = readdir(dir_ptr)) != NULL) {
            printf("%s: %s\n", cmd, direntp->d_name);
        }
        closedir(dir_ptr); /* 다 읽은 후 디렉토리 닫기 */
    }
}
