#include "lib.h"
#include "fat32.h"
#define KERNEL_CS 	(0x08) >> 3
#define	KERNEL_DS 	(0x10) >> 3
#define	USER_DS		(0x18) >> 3
#define CALL_SS		(0x20) >> 3

#define CLONE_VM		(1 << 0)	/* shared Virtual Memory between processes */
#define CLONE_FS		(1 << 1)	/* shared fs info between processes */
#define CLONE_SIGNAL	(1 << 2)	/* shared signal between processes */

#pragma region Task
extern unsigned long kallsyms_addresses[] __attribute__((weak));
extern long kallsyms_syms_num __attribute__((weak));
extern long kallsyms_index[] __attribute__((weak));
extern char* kallsyms_names __attribute__((weak));
typedef struct{
    unsigned long r15;
    unsigned long r14;
    unsigned long r13;
    unsigned long r12;
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long rsi;
    unsigned long rdi;
    unsigned long rbp;
    unsigned long rdx;
    unsigned long rcx;
    unsigned long rbx;
    unsigned long ds;
    unsigned long es;
    unsigned long rax;
} pt_regs;

long global_pid;
mm_struct init_mm;

void Init_Task(task_union *tsk_un, mm_struct* mm_st,unsigned long rsp,unsigned long rsp0);
void switch_to(task_struct *prev, task_struct *next);
task_struct *get_task(long pid);
task_struct *do_fork(unsigned long clone_flags, unsigned char *path);
unsigned long do_unfork(task_struct *tsk);
#pragma endregion

#pragma region System call
#define KERNEL_GS_BASE 0xC0000102
#define GS_BASE        0xC0000101
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

#define PUSH_ALL "\
    pushq	%rax\n\t\
    movq    %es, %rax\n\t\
    pushq	%rax\n\t\
    movq    %ds, %rax\n\t\
    pushq	%rax\n\t\
    pushq	%rbx\n\t\
    pushq	%rcx\n\t\
    pushq	%rdx\n\t\
    pushq	%rbp\n\t\
    pushq	%rdi\n\t\
    pushq	%rsi\n\t\
    pushq	%r8\n\t\
    pushq	%r9\n\t\
    pushq	%r10\n\t\
    pushq	%r11\n\t\
    pushq	%r12\n\t\
    pushq	%r13\n\t\
    pushq	%r14\n\t\
    pushq	%r15\n\t"
#define POP_ALL "\
    popq	%r15\n\t\
    popq	%r14\n\t\
    popq	%r13\n\t\
    popq	%r12\n\t\
    popq	%r11\n\t\
    popq	%r10\n\t\
    popq	%r9\n\t\
    popq	%r8\n\t\
    popq	%rsi\n\t\
    popq	%rdi\n\t\
    popq	%rbp\n\t\
    popq	%rdx\n\t\
    popq	%rcx\n\t\
    popq	%rbx\n\t\
    popq	%rax\n\t\
    movq    %rax, %ds\n\t\
    popq	%rax\n\t\
    movq    %rax, %es\n\t\
    popq	%rax\n\t"
void Sys_Call_Kernel(void);
void fork_ret_entry(void);
unsigned long Sys_Func(unsigned long Idx, unsigned long Param);
unsigned long no_system_call(unsigned long param);
unsigned long sys_printf(unsigned long param);
unsigned long sys_open(char *filename,unsigned long flags);
unsigned long sys_close(unsigned long fd);
unsigned long sys_read(unsigned long fd, void *buf, unsigned long count);
unsigned long sys_write(unsigned long fd, void *buf, unsigned long count);
unsigned long sys_lseek(unsigned long fd, long offset, unsigned long whence);
unsigned long sys_fork(void);
unsigned long sys_brk(unsigned long brk);
unsigned long sys_clear(void);
unsigned long sys_reboot(unsigned long cmd);
unsigned long sys_lsdir(char* filename);
unsigned long sys_chdir(char* filename);
typedef unsigned long (* system_call_t)(unsigned long Param);//Syscall function ptr
typedef struct{
    unsigned int EIP;
    selector CALL_CS;
    selector RET_CS;
} star;
typedef struct{
    unsigned long SCE:1;
    unsigned long RAZ:7;
    unsigned long LME:1;
    unsigned long MBZ0:1;
    unsigned long LMA:1;
    unsigned long NXE:1;
    unsigned long SVME:1;
    unsigned long LMSLE:1;
    unsigned long FFXSR:1;
    unsigned long TCE:1;
    unsigned long MBZ1:64-16;
} efer;
#pragma endregion

#pragma region User
extern void user_init(void);
#pragma endregion

#pragma region schedule
void Init_schedule(void);
void insert_task_queue(task_struct *tsk);
task_struct * get_next_task();
#pragma endregion

void backtrace(unsigned long *rbp, unsigned long rip, unsigned long rsp);