#pragma once

#define NULL 0

#define SYS_PRINT 1
#define SYS_OPEN  3
#define SYS_CLOSE 4
#define SYS_READ  5
#define SYS_WRITE 6
#define SYS_LSEEK 7
#define SYS_FORK  8
#define SYS_BRK   9
#define SYS_CLR   10
#define SYS_RBT   11
#define SYS_CD    12
#define SYS_LS    13
#define SYS_EXEC  14
#define SYS_EXIT  15
#define SYS_WAIT  16

#define	SYSTEM_REBOOT	(1UL << 0)
#define	SYSTEM_POWEROFF	(1UL << 1)

typedef struct mall_des{
    struct mall_des* prev;
    struct mall_des* next;
    unsigned long end;
} mall_des;

unsigned long brk_st = 0;
unsigned long brk_ed = 0;