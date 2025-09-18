# include <grp.h>
# include <stddef.h>
# include <stdio.h>

/*
 * git_to_name: GID를 그룹 이름으로 변환
 */
char * gid_to_name(gid_t gid) {
    /* getgrgid 원형과 포인터 선언. 보통 <grp.h>를 include 하면
     * 함수 원형은 헤더에서 제공되므로 변수만 선언하면 된다.
     * returns pointer to group number gid. used getgrgid(3)
     */
    struct group * getgrgid(), *grp_ptr;
    static char numstr[10];

    if ((grp_ptr = getgrgid(gid)) == NULL) {
        /* 해당 GID의 그룹 정보가 없으면 숫자를 문자열로 반환 
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
