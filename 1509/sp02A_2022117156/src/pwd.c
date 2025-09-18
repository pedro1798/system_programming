# include <pwd.h>
# include <stddef.h>
# include <stdio.h>

/*
 * uid_to_name: UID를 사용자 이름으로 변환 
 */
char * uid_to_name(uid_t uid) {
    /* 옛 스타일로 getpwuid 함수 원형을 선언하고 pw_ptr 변수도 같이 선언.
     * modern C에서는 <pwd.h>를 include 하고 아래처럼 
     * 변수만 선언하는 편이 낫습니다:
     * struct passwd *pw_ptr;
     * returns to pointer to username associated with uid, uses getpw() 
     */
    
    struct passwd * getpwuid(), *pw_ptr;
    static char numstr[10];
    /*
     * 반환용 정적 버퍼
     * - static이라서 함수 밖으로 포인터 반환해도 유효(라이프타임 전역)
     *   단, static 버퍼는 스레드 안전하지 않음(다중 스레드에서 덮어씀).
     *   10바이트 크기는 일부 시스템의 큰 UID(예: 32/64비트)에서 
     *   부족할 수 있음. (NULL 문저 포함해야 하므로 최소 11바이트
     *   필요할 수 있다.
     */

    if ((pw_ptr = getpwuid(uid)) == NULL) {
        /* getpwuid가 실패했을 때(해당 UID의 사용자 정보가 없을 때 */
        
        sprintf(numstr, "%d", uid);
        /* UID 수치를 문자열로 변환하여 numstr에 저장 후 반환
         * - sprintf 사용: 버퍼 크기를 체크하지 않으므로 
         *   **버퍼 오버플로우 위험**.
         *  - uid_t가 부호/크기가 시스템마다 다르므로 
         *  포맷 문자열이 안전하지 않을 수 있음.
         *  - safer: snprintf(numstr, sizeof numstr, "%lu", (unsigned long)uid);
         */

        return numstr;
    } else {
        /* getpwuid가 성공하면 pw_ptr->pw_name(유저 이름 문자열)을 반환.
         * 주의: pw_ptr와 내부 문자열은 라이브러리 내부의 정적 메모리를 가리킴.
         * 즉, 이후 getpwent/getpwuid 등의 호출로 덮어써질 수 있음.
         */
        return pw_ptr->pw_name;
    }
}

/*
int sprintf(char *str, const char *format, ...);
sprintf는 문자열을 포맷팅해서 버퍼에 저장하는 함수이다.
쉽게 말해 printf는 터미널(표준 출력)에 출력하는 함수고, sprintf는 문자 배열(char[])에 출력하는 함수이다.
    •	str : 결과를 저장할 버퍼(문자 배열)
    •	format : 출력 형식 지정 문자열 (%d, %s, %f 등)
    •	... : 출력할 값들
    •	반환값 : 실제로 기록한 문자 개수 (널 종료 문자는 제외)
*/
