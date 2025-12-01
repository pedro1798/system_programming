#ifndef UTMPLIB_H
#define UTMPLIB_H

#include <utmp.h>

int utmp_open(char *filename);
struct utmp * utmp_next(void);              // 다음 레코드 반환
int utmp_reload(void);
void utmp_close(void); 

#endif
