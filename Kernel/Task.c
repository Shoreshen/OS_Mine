#include "Task.h"
#include "Mem.h"
#include "TSS.h"
#include "fat32.h"

star STAR;
efer EFER;

#pragma region Task
task_struct *get_task(long pid)
{
    task_struct *tsk = NULL;
    for (tsk = init_task_union.Task.next; tsk != &init_task_union.Task; tsk = tsk->next) {
        if (tsk->pid == pid) {
            return tsk;
        }
    }
    return NULL;
}
void Init_Task(task_union *tsk_un, mm_struct *mm_st, unsigned long rsp, unsigned long rsp0)
{
    int i;
    // color_printk(GREEN,BLACK,"RSP:%p, RSP0:%p\n",rsp,rsp0);
    tsk_un->Task.mm = mm_st;
    if ((unsigned long)mm_st->PML4E != Get_CR3())
    {
        mm_st->PML4E = (pml4e *)Get_CR3();
        mm_st->start_code = GLB_Men_Desc.start_code;
        mm_st->end_code = GLB_Men_Desc.end_code;
        mm_st->start_data = (unsigned long)&_data;
        mm_st->end_data = GLB_Men_Desc.end_code;
        mm_st->start_rodata = (unsigned long)&_rodata;
        mm_st->end_rodata = GLB_Men_Desc.end_rodata;
        mm_st->start_bss = (unsigned long)&_bss;
        mm_st->end_bss = (unsigned long)&_ebss;
        mm_st->start_brk = PAGE_2M_ALIGN(GLB_Men_Desc.struct_end);
        mm_st->end_brk = 0xffffffffffffffff;
        mm_st->start_stack = _stack_start;
    }

    tsk_un->Task.flag = PF_KTHREAD;
    tsk_un->Task.state = TASK_RUNNING;
    tsk_un->Task.pid = global_pid ++;
    tsk_un->Task.cpu_id = current->cpu_id;
    tsk_un->Task.priority = 2;
    tsk_un->Task.vrun_time = 0;

    tsk_un->Task.thread.rip = 0;
    tsk_un->Task.thread.rsp = rsp;
    tsk_un->Task.thread.rsp0 = rsp0;
    tsk_un->Task.thread.rsp_user = 0xA00000;
    tsk_un->Task.thread.ds = KERNEL_DS;
    tsk_un->Task.thread.kernel_gs_base = 0;
    tsk_un->Task.thread.gs_base = 0;
    tsk_un->Task.parent = &tsk_un->Task;
    tsk_un->Task.next = &tsk_un->Task;
    // color_printk(GREEN,BLACK,"current:%p,RSP0:%p\n",current, current->thread.rsp0);
    for (i = 0; i < TASK_FILE_MAX; i++)
    {
        tsk_un->Task.fptr[i] = NULL;
    }

    List_Init(&tsk_un->Task.List);

    //sysenter: RIP = Sys_Cal; CS = KERNEL_CS; RSP = _stack_start; SS = KERNEL_CS + 8
    // wrmsr(0x174, KERNEL_CS);
    // wrmsr(0x175, rsp0);
    // wrmsr(0x176, (unsigned long)Sys_Call);

    //Enable SYSCALL/SYSRET 64bit mode instruction
    *((unsigned long *)&EFER) = rdmsr(0xC0000080);
    EFER.SCE = 1;
    wrmsr(0xC0000080, *((unsigned long *)&EFER));
    //Notion: fast call will reserve RPL in selector, privilge check will be performed while interrupt
    STAR.EIP = 0; //32bit Compatibility-Mode, not used
    //syscall: CS = CALL_CS; SS = CALL_CS + 8
    STAR.CALL_CS.IS = CALL_SS;
    STAR.CALL_CS.TI = 0;
    STAR.CALL_CS.RPL = 0;
    //sysret:  CS = RET_CS + 16; SS = RET_CS + 8. Already adjusted order of descriptor in head.S
    STAR.RET_CS.IS = KERNEL_CS;
    STAR.RET_CS.RPL = 3;
    STAR.RET_CS.TI = 0;
    wrmsr(0xC0000081, *((unsigned long *)&STAR));       //STAR
    wrmsr(0xC0000082, (unsigned long)Sys_Call_Kernel);  //LSTAR: RIP after syscall
    wrmsr(KERNEL_GS_BASE, (unsigned long)&tsk_un->Task);    //Initilize swapgs
}
unsigned long do_exit(unsigned long exit_code)
{
    color_printk(RED,BLACK,"exit task is running,arg:%#018lx\n",exit_code);

    // color_printk(GREEN,BLACK,"current:%p,pid:%d",current,current->pid);
    while(1){
        cli;
        current->state = TASK_ZOMBIE;
        current->exit_code = exit_code;
        sti;
        
        wakeup(&current->parent->wait_childexit,TASK_UNINTERRUPTIBLE);
        // color_printk(GREEN,BLACK,"current:%p,K_GS_BASE:%p\n",current,rdmsr(KERNEL_GS_BASE));

        schedule();
    }

    return 0;
}
unsigned long do_exec(unsigned long argc, unsigned long *argv)
{
    FS_entry *entry = NULL;
    pt_regs *reg = (pt_regs *)(current->thread.rsp0 - sizeof(pt_regs));

    entry = path_walk((char *)argv[1], 0);
    if(!entry){
        return NULL;
    }
    entry->type.file.pos = 0;
    memset((void *)0x800000,0,0xA00000 - 0x800000);
    if (entry->sp && entry->type.file.ops->read) {
        entry->type.file.ops->read(entry, (void *)(0x800000), entry->type.file.size);
    }else {
        return NULL;
    }
    current->thread.rsp_user = 0xA00000;

    reg->rcx = 0x800000;
    reg->rdi = argc - 2;
    reg->rsi = (unsigned long)&argv[2];

    return 1;
}
task_struct *do_fork(unsigned long clone_flags, unsigned char *path)
{
    task_struct *tsk = NULL;
    unsigned long i, ret = 0;
    pg_attr ATTR;
    FS_entry *entry = NULL;
    page *p = NULL;

    tsk = (task_struct *)kmalloc(STACK_SIZE, 0);
    memset(tsk, 0, STACK_SIZE);
    memset(&ATTR, 0, sizeof(pg_attr));

    List_Init(&tsk->List);
    tsk->priority = 2;
    tsk->pid = global_pid++;
    tsk->cpu_id = current->cpu_id;
    tsk->state = TASK_UNINTERRUPTIBLE;
    tsk->next = init_task_union.Task.next;
    init_task_union.Task.next = tsk;
    tsk->parent = current;
    wait_queue_init(&tsk->wait_childexit, NULL);
    //Flags
    if (clone_flags & CLONE_VM) {
        tsk->flag |= PF_VFORK;
    }
    //File pointers
    if (!(clone_flags & CLONE_FS)) {
        for (i = 0; i < TASK_FILE_MAX; i++) {
            if (current->fptr[i] != NULL) {
                tsk->fptr[i] = (FS_entry *)kmalloc(sizeof(FS_entry), 0);
                if (!tsk->fptr[i]) {
                    do_unfork(tsk);
                    return NULL;
                }
                memcpy(&current->fptr[i], tsk->fptr[i], sizeof(FS_entry));
            }
        }
    }
    //Memory & pages
    if (clone_flags & CLONE_VM) {
        tsk->mm = current->mm;
    } else {
        tsk->mm = kmalloc(sizeof(mm_struct), 0);
        if (!tsk->mm) {
            do_unfork(tsk);
            return NULL;
        }
        memcpy(current->mm, tsk->mm, sizeof(mm_struct));
        tsk->mm->PML4E = kmalloc(PAGE_4K_SIZE, 0);
        if (!tsk->mm->PML4E){
            do_unfork(tsk);
            return NULL;
        }
        memset(tsk->mm->PML4E, 0, PAGE_4K_SIZE);
        //Copy kernel pages
        memcpy((void *)((unsigned long)&current->mm->PML4E[256]+PAGE_OFFSET), &tsk->mm->PML4E[256], PAGE_4K_SIZE / 2);
        //Convert to physical
        tsk->mm->PML4E = (pml4e *)((unsigned long)tsk->mm->PML4E - PAGE_OFFSET);
    }
    //Deal with user space
    // p = alloc_pages(1, 0);
    // free_pages(p->PHY_Address,1);
    // p = NULL;
    p = alloc_pages(1, 0);
    if (!p) {
        do_unfork(tsk);
        return NULL;
    }
    ATTR.PML4E_Attr.RW = 1;
    ATTR.PML4E_Attr.P = 1;
    ATTR.PDPTE_Attr.P = 1;
    ATTR.PDPTE_Attr.RW = 1;
    ATTR.PDE_Attr.PS = 1;
    ATTR.PDE_Attr.P = 1;
    ATTR.PDE_Attr.RW = 1;
    if (!(tsk->flag & PF_KTHREAD)) {
        ATTR.PML4E_Attr.US = 1;
        ATTR.PDPTE_Attr.US = 1;
        ATTR.PDE_Attr.US = 1;
    }
    ret = pagetable_init((unsigned long *)tsk->mm->PML4E, p->PHY_Address, 0x800000, 1, &ATTR, 0, 0);
    if (!ret) {
        do_unfork(tsk);
        return NULL;
    }
    //tsk->thread structure
    tsk->thread.rsp0 = (unsigned long)tsk + STACK_SIZE;
    tsk->thread.kernel_gs_base = (unsigned long)tsk;
    tsk->thread.gs_base = NULL;
    if(path){
        //memory
        tsk->mm->start_brk = 0xA00000;
        tsk->mm->end_brk = 0xA00000;
        //thread
        tsk->thread.rip = (unsigned long)user_init;
        tsk->thread.rsp = 0xA00000;
        tsk->thread.rsp_user = 0xA00000;
        //code
        entry = path_walk(path, 0);
        if (!entry) {
            do_unfork(tsk);
            return NULL;
        }

        if (entry->sp && entry->type.file.ops->read) {
            ret = entry->type.file.ops->read(entry, (void *)(p->PHY_Address + PAGE_OFFSET), entry->type.file.size);
        } else {
            do_unfork(tsk);
            return NULL;
        }
    }else{
        //code
        memcpy((void *)0x800000, (void *)(p->PHY_Address + PAGE_OFFSET), PAGE_2M_SIZE);
        //Heap
        if(tsk->mm->end_brk - tsk->mm->start_brk != 0){ //If valid heap, add one page after
            p = NULL;
            p = alloc_pages(1, 0);
            if (!p) {
                color_printk(BLACK,WHITE,"alloc_pages fail");
                do_unfork(tsk);
                return NULL;
            }
            ret = pagetable_init((unsigned long *)tsk->mm->PML4E, p->PHY_Address, 0xA00000, 1, &ATTR, 0, 0);
            if (!ret) {
                color_printk(BLACK,WHITE,"pagetable_init fail");
                do_unfork(tsk);
                return NULL;
            }
            memcpy((void *)0xA00000, (void *)(p->PHY_Address + PAGE_OFFSET), PAGE_2M_SIZE);
        }
        //thread
        tsk->thread.rip = (unsigned long)fork_ret_entry;
        tsk->thread.rsp = (unsigned long)tsk + STACK_SIZE - 0x88;
        memcpy((void *)(current->thread.rsp0 - 0x88), (void *)tsk->thread.rsp, 0x88);
        ret = tsk->thread.rsp0 - 8;
        *((unsigned long *)ret) = 0;
        tsk->thread.rsp_user = current->thread.rsp_user;
    }

    tsk->state = TASK_RUNNING;
    insert_task_queue(tsk);
    current->flag |= NEED_SCHEDULE;

    return tsk;
}
unsigned long do_unfork(task_struct *tsk)
{
    int i, j, k;
    unsigned long *PML4E = (unsigned long *)((unsigned long)tsk->mm->PML4E + PAGE_OFFSET), *PDPTE = 0,*PDE = 0;

    color_printk(BLACK,RED,"Doing unfork\n");
    //Free file system
    if (!(tsk->flag & CLONE_FS)) {
        for (i = 0; i < TASK_FILE_MAX; i++) {
            if (current->fptr[i] != NULL) {
                kfree(tsk->fptr[i]);
            }
        }
    }
    //Free page table
    if (!(tsk->flag & CLONE_VM)) {
        for (i = 0; i < 256; i++) { //Only clean user space
            if (PML4E[i]) {
                PDPTE = (unsigned long *)((PML4E[i] & (~0xfffUL)) + PAGE_OFFSET);
                color_printk(ORANGE, BLACK, "PML4E[%d]:%p, Virt:%p\n", i, PML4E[i], PDPTE);
                for (j = 0; j < 512; j++) {
                    if (PDPTE[j]) {
                        PDE = (unsigned long *)((PDPTE[i] & (~0xfffUL)) + PAGE_OFFSET);
                        color_printk(ORANGE, BLACK, "PDPTE[%d]:%p,Virt:%p\n", j, PDPTE[i],PDE);
                        for (k = 0; k < 512; k++) {
                            if (PDE[k]) {
                                color_printk(ORANGE, BLACK, "PDE[%d]:%p\n", k, PDE[k]);
                                free_pages((unsigned long)(PDE[j] & (~0xfffUL)), 1);
                            }
                        }
                        kfree((void *)PDE);
                    }
                }
                kfree((void *)PDPTE);
            }
        }
        kfree((void *)PML4E);
    }
    kfree(tsk);
}
#pragma endregion

#pragma region User
__asm__(
    ".globl user_init\t\n"
    "user_init:\t\n"
    "movq $0x800000, %rcx \t\n" //sysret use rcx for rip
    "pushfq \t\n"
    "popq %r11 \t\n" //sysret will pop r11 as rflag
    ".byte 0x48 \t\n"
    "sysret \t\n"
);
#pragma endregion

#pragma region System enter / call
unsigned long no_system_call(unsigned long param)
{
    color_printk(RED, BLACK, "no_system_call is calling,NR:%#04x\n", param);
    return -1;
}
unsigned long sys_printf(unsigned long param)
{
    // if(!strcmp((char*)param,"exit_code: 10\n")){
    //     color_printk(YELLOW,BLACK,"current:%p,GS_BASE:%p",current, rdmsr(0xC0000101));
    // }
    return (unsigned long)color_printk(WHITE, BLACK, (char *)param);
}
unsigned long sys_open(char *filename, unsigned long flags)
{
    unsigned long i = 0;
    char *path = NULL;
    long pathlen = 0, error = 0;
    FS_entry *entry = NULL;

    path = (char *)kmalloc(MAX_FILE_DIR_LEN, 0);
    if (!path) {
        return ENOMEM;
    }
    pathlen = strnlen_user(filename, MAX_FILE_DIR_LEN);
    if (pathlen <= 0) {
        kfree(path);
        return EFAULT;
    } else if (pathlen == MAX_FILE_DIR_LEN) {
        kfree(path);
        return ENAMETOOLONG;
    }
    strncpy_from_user(filename, path, pathlen + 1);

    entry = path_walk(path, 0);
    kfree(path);

    if (!entry) {
        return ENOENT;
    }
    if (entry->attr & ATTR_DIRECTORY) {
        return EISDIR;
    }

    if (entry->sp && entry->type.file.ops->open) {
        error = entry->type.file.ops->open(entry);
    }
    if (error != 1) {
        return -EFAULT;
    }

    color_printk(BLUE, WHITE, "Find %s\nDIR_FirstCluster:%d\tDIR_FileSize:%d\terror=%d\n", entry->name, entry->first_cluster, entry->type.file.size, error);

    if (flags & O_TRUNC)
    {
        entry->type.file.size = 0;
    }
    else if (flags & O_APPEND)
    {
        entry->type.file.pos = entry->type.file.size;
    }
    entry->type.file.mode = flags;

    for (i = 0; i < TASK_FILE_MAX; i++) {
        if (!current->fptr[i]) {
            break;
        }
    }

    if (i == TASK_FILE_MAX) {
        return EMFILE;
    }
    current->fptr[i] = entry;
    return i;
}
unsigned long sys_close(unsigned long fd)
{
    FS_entry *entry = NULL;

    if (fd < 0 || fd >= TASK_FILE_MAX){
        return EBADF;
    }

    entry = current->fptr[fd];
    if (entry->sp && entry->type.file.ops->close){
        entry->type.file.ops->close(entry);
    }

    color_printk(GREEN, BLACK, "File \"%s\" closed!\n", entry->name);
    current->fptr[fd] = NULL;

    return 0;
}
unsigned long sys_read(unsigned long fd, void *buf, unsigned long count)
{
    FS_entry *entry = NULL;
    unsigned long ret = 0;

    if (fd < 0 || fd >= TASK_FILE_MAX) {
        return EBADF;
    }
    if (count < 0) {
        return EINVAL;
    }

    entry = current->fptr[fd];

    if (entry->sp && entry->type.file.ops->read) {
        ret = entry->type.file.ops->read(entry, buf, count);
    }

    return ret;
}
unsigned long sys_write(unsigned long fd, void *buf, unsigned long count)
{
    FS_entry *entry = NULL;
    unsigned long ret = 0;

    if (fd < 0 || fd >= TASK_FILE_MAX)
    {
        return EBADF;
    }
    if (count < 0)
    {
        return EINVAL;
    }

    entry = current->fptr[fd];

    if (entry->sp && entry->type.file.ops->write)
    {
        ret = entry->type.file.ops->write(entry, buf, count);
    }

    return ret;
}
unsigned long sys_lseek(unsigned long fd, long offset, unsigned long whence)
{
    FS_entry *entry = NULL;
    unsigned long ret = 0;

    if (fd < 0 || fd >= TASK_FILE_MAX)
    {
        return EBADF;
    }

    if (whence < 0 || whence >= SEEK_MAX)
    {
        return EINVAL;
    }

    entry = current->fptr[fd];

    if (entry->sp && entry->type.file.ops->lseek)
    {
        ret = entry->type.file.ops->lseek(entry, offset, whence);
    }

    return ret;
}
unsigned long sys_fork(void)
{
    task_struct *tsk = NULL;
    color_printk(GREEN, BLACK, "sys_fork\n");
    tsk = do_fork(NULL, NULL);
    // glb_ind = 1;
    if(tsk){
        return tsk->pid;
    }else{
        return NULL;
    }
}
unsigned long sys_brk(unsigned long brk)
{
    unsigned long new_brk = PAGE_2M_ALIGN(brk), i, PHY_Addr;
    page *p;
    pg_attr ATTR;

    // color_printk(GREEN,BLACK,"sys_brk:%p, new_brk:%p\n",brk,new_brk);

    if(new_brk == 0){
        return current->mm->start_brk;
    }
    if(new_brk < current->mm->end_brk){
        for(i = current->mm->end_brk - PAGE_2M_SIZE; i >= new_brk; i -= PAGE_2M_SIZE){
            PHY_Addr = NULL;
            PHY_Addr = pagetable_clear((unsigned long *)current->mm->PML4E, i, 1, 0, 1);
            if(PHY_Addr){
                if(free_pages(PHY_Addr, 1)){
                    color_printk(RED,BLACK,"Free page failed, end_brk:%p\n",current->mm->end_brk);
                    current->mm->end_brk -= PAGE_2M_SIZE;
                }else{
                    break;
                }
            }else{
                color_printk(RED,BLACK,"Page table clear failed, end_brk:%p\n",current->mm->end_brk);
                break;
            }
        }
    } else{
        memset(&ATTR, 0, sizeof(pg_attr));
        ATTR.PML4E_Attr.RW = 1;
        ATTR.PML4E_Attr.P = 1;
        ATTR.PDPTE_Attr.P = 1;
        ATTR.PDPTE_Attr.RW = 1;
        ATTR.PDE_Attr.PS = 1;
        ATTR.PDE_Attr.P = 1;
        ATTR.PDE_Attr.RW = 1;
        if (!(current->flag & PF_KTHREAD)) {
            ATTR.PML4E_Attr.US = 1;
            ATTR.PDPTE_Attr.US = 1;
            ATTR.PDE_Attr.US = 1;
        }
        for(i = current->mm->end_brk; i < new_brk; i += PAGE_2M_SIZE){
            p = alloc_pages(1,0);
            if(p){
                if(pagetable_init((unsigned long *)current->mm->PML4E, p->PHY_Address, i, 1, &ATTR, 0, 1)){
                    current->mm->end_brk += PAGE_2M_SIZE;
                }else{
                    color_printk(RED,BLACK,"Page table init failed, end_brk:%p\n",current->mm->end_brk);
                    break;
                }
            }else{
                color_printk(RED,BLACK,"Allocate page failed, end_brk:%p\n",current->mm->end_brk);
                break;
            }
        }
    }
    
    // color_printk(RED,BLACK,"start_brk:%p, end_brk:%p\n",current->mm->start_brk,current->mm->end_brk);
    
    return current->mm->end_brk;
}
unsigned long sys_clear(void)
{
    memset(Pos.FB_addr, 0, Pos.XResolution * Pos.YResolution * sizeof(int));
    Pos.XPosition = 0;
    Pos.YPosition = 0;
    // color_printk(GREEN,BLACK,"KERNEL_GS_BASE:%p\n",rdmsr(KERNEL_GS_BASE));
    // color_printk(GREEN,BLACK,"current:%p\n",current);
    return 1;
}
unsigned long sys_reboot(unsigned long cmd)
{
   	color_printk(GREEN,BLACK,"sys_reboot\n");
	switch(cmd){
		case SYSTEM_REBOOT:
			io_out8(0x64,0xFE);
			break;

		case SYSTEM_POWEROFF:
			color_printk(RED,BLACK,"sys_reboot cmd SYSTEM_POWEROFF\n");
			break;

		default:
			color_printk(RED,BLACK,"sys_reboot cmd ERROR!\n");
			break;
	}
	return 0;
}
unsigned long sys_chdir(char* filename)
{
    char *path;
    unsigned long pathlen = 0;
    FS_entry *entry = NULL;

    color_printk(GREEN,BLACK,"sys_chdir:%s\n", filename);
    
    path = (char *)kmalloc(PAGE_4K_SIZE,0);
    if(path == NULL){
        return ENOMEM;
    }
    pathlen = strlen(filename) + 1;
    if(pathlen < 0){
        kfree(path);
        return EFAULT;
    }else if(pathlen > PAGE_4K_SIZE){
        kfree(path);
        return ENAMETOOLONG;
    }

    strncpy_from_user(filename, path, pathlen);
 
    entry = path_walk(path, 0);
    if (!entry) {
        return ENOENT;
    }
    if(!(entry->attr & ATTR_DIRECTORY)) {
        return ENOTDIR;
    }

    return 0;
}
unsigned long sys_lsdir(char* filename)
{
    unsigned long i = 0;
    char *path = NULL;
    long pathlen = 0, error = 0;
    FS_entry *entry = NULL;

    path = (char *)kmalloc(MAX_FILE_DIR_LEN, 0);
    if (!path) {
        return ENOMEM;
    }
    pathlen = strnlen_user(filename, MAX_FILE_DIR_LEN);
    if (pathlen <= 0) {
        kfree(path);
        return EFAULT;
    } else if (pathlen == MAX_FILE_DIR_LEN) {
        kfree(path);
        return ENAMETOOLONG;
    }
    strncpy_from_user(filename, path, pathlen + 1);

    entry = path_walk(path, 0);

    if (!entry) {
        return ENOENT;
    }
    if (!(entry->attr & ATTR_DIRECTORY)) {
        return ENOTDIR;
    }

    if (entry->sp && entry->type.dir.ops->lsdir) {
        error = entry->type.dir.ops->lsdir(entry);
    }
    if (error != 1) {
        return -EFAULT;
    }
    return 0;
}
unsigned long sys_exec(unsigned long argc, unsigned long *argv)
{
    int i;

    return  do_exec(argc, argv);
}
unsigned long sys_exit(unsigned long exit_code)
{
    color_printk(GREEN,BLACK,"sys_exit\n");
    return do_exit(exit_code);
}
unsigned long sys_wait(unsigned long pid)
{
    task_struct *child = NULL;
    unsigned long retval = 0;
    color_printk(GREEN,BLACK,"sys_wait\n");
    child = get_task(pid);
    if(child->parent != current){
        return ECHILD;
    }

    if(child->state == TASK_ZOMBIE){
        child->parent->next = child->next;
        do_unfork(child);
        return retval;
    }

    sleep(&current->wait_childexit);

    retval = child->exit_code;
    child->parent->next = child->next;
    // do_unfork(child);
    // color_printk(YELLOW, BLACK, "pid:%d,current:%p,K_GS_BASE:%p,GS_BASE:%p\n",
    //     current->pid,current, rdmsr(KERNEL_GS_BASE),rdmsr(GS_BASE));
    return retval;
}
system_call_t system_call_table[MAX_SYSTEM_CALL_NR] = {
    //First 6 parameter: RDI, RSI, RDX, RCX, R8 and R9
    [0] = no_system_call,
    [SYS_PRINT] = (unsigned long (*)(unsigned long Param))sys_printf,
    [2] = (unsigned long (*)(unsigned long Param))DISK1_FAT32_FS_init,
    [SYS_OPEN]  = (unsigned long (*)(unsigned long Param))sys_open,
    [SYS_CLOSE] = (unsigned long (*)(unsigned long Param))sys_close,
    [SYS_READ]  = (unsigned long (*)(unsigned long Param))sys_read,
    [SYS_WRITE] = (unsigned long (*)(unsigned long Param))sys_write,
    [SYS_LSEEK] = (unsigned long (*)(unsigned long Param))sys_lseek,
    [SYS_FORK]  = (unsigned long (*)(unsigned long Param))sys_fork,
    [SYS_BRK]   = (unsigned long (*)(unsigned long Param))sys_brk,
    [SYS_CLR]   = (unsigned long (*)(unsigned long Param))sys_clear,
    [SYS_RBT]   = (unsigned long (*)(unsigned long Param))sys_reboot,
    [SYS_CD]    = (unsigned long (*)(unsigned long Param))sys_chdir,
    [SYS_LS]    = (unsigned long (*)(unsigned long Param))sys_lsdir,
    [SYS_EXEC]  = (unsigned long (*)(unsigned long Param))sys_exec,
    [SYS_EXIT]  = (unsigned long (*)(unsigned long Param))sys_exit,
    [SYS_WAIT]  = (unsigned long (*)(unsigned long Param))sys_wait,
    [17 ... MAX_SYSTEM_CALL_NR - 1] = no_system_call
};
__asm__(
    ".globl Sys_Call_Kernel\t\n"
    "Sys_Call_Kernel:\t\n" 
    "swapgs\n\t"
    "movq %rsp, %gs:(40)\n\t"
    "movq %gs:(24),%rsp\n\t"
    PUSH_ALL
    "movq %rbx,%rax\n\t"
    "leaq system_call_table(%rip), %rbx\t\n"
    "callq *(%rbx,%rax,8)\t\n"
    "movq %rax,0x80(%rsp)\t\n"
    POP_ALL
    "movq %gs:(40),%rsp\n\t"
    "swapgs\n\t"
    "orq $0x200,%r11\t\n"     //syscall will mask interrupt RFLAGS.IF, need reset to enable interrupt
                              //can modify corresponding bit in SFMASK(0xC0000084) to control the clear of RFLAGS register
    ".byte 0x48\t\n"
    "sysret\t\n"              //return to rcx, already made when calling
);
__asm__(
    ".globl fork_ret_entry\t\n"
    "fork_ret_entry:\t\n"       //Enter from timer interrupt, gs->user
    "swapgs\n\t"                //gs->current
    POP_ALL                     
    "movq %gs:(40),%rsp\n\t"    //move current->rsp_user to rsp
    "swapgs\n\t"                //gs->user
    "orq $0x200,%r11\t\n" 
    ".byte 0x48\t\n"
    "sysret\t\n"
);
#pragma endregion

#pragma region schedule
void switch_to(task_struct *prev, task_struct *next)
{
    TSS_TABLE[current->cpu_id].RSP0 = next->thread.rsp0; //User level interrupt (CPL change) will use rsp0, kernel interrupt will not.
    //Prepare for swapgs
    prev->thread.gs_base = rdmsr(GS_BASE);
    prev->thread.kernel_gs_base = rdmsr(KERNEL_GS_BASE);
    wrmsr(KERNEL_GS_BASE, next->thread.kernel_gs_base);
    wrmsr(GS_BASE, next->thread.gs_base);
    // wrmsr(0x175,next->thread.rsp0); //For user thread, it has different kernel stack for system call. Do not need using syscall
    __asm__ __volatile__(
        "pushq %%rbp\n\t"
        "pushq %%rax\n\t"
        "movq %%ds, %%rbx \n\t"
        "movq %%rbx, %0 \n\t"
        "movq %%rsp, %1 \n\t"
        "leaq 1f(%%rip), %%rbx \n\t"
        "movq %%rbx, %2 \n\t"
        "movq %%rax, %%ds \n\t"
        "movq %%rax, %%es \n\t"
        "movq %4, %%rsp\n\t"
        "movq %6, %%rax \n\t"
        "movq %%rax, %%cr3 \n\t"
        "jmpq *%5 \n\t"
        "1:\n\t"
        "popq %%rax\n\t"
        "popq %%rbp\n\t"
        : "=m"(prev->thread.ds), "=m"(prev->thread.rsp), "=m"(prev->thread.rip)
        : "a"(next->thread.ds), "m"(next->thread.rsp), "m"(next->thread.rip), "m"((unsigned long)next->mm->PML4E)
        : "memory"
    );
}
task_struct *get_next_task()
{
    task_struct *tsk;
    if (List_is_empty(&task_schedule[current->cpu_id].task_queue.List)) {
        return &init_task_union.Task;
    }
    tsk = container_of(task_schedule[current->cpu_id].task_queue.List.next, task_struct, List);
    List_Del(&tsk->List);

    task_schedule[current->cpu_id].running_task_count -= 1;

    return tsk;
}
void schedule(void)
{
    task_struct *tsk = NULL;
    unsigned int color;
    cli;
    current->flag &= ~NEED_SCHEDULE;
    tsk = get_next_task();
    if (current->cpu_id) {
        color = RED;
    } else {
        color = ORANGE;
    }
    // if(glb_ind){
    //     color_printk(color, BLACK, "#schedule:%d,pid:%ld(%ld)=>>pid:%ld(%ld)#\n", jiffies,
    //                 current->pid,current->vrun_time,tsk->pid,tsk->vrun_time);
    //     color_printk(GREEN,BLACK,"current:%p,K_GS_BASE:%p\t",current,rdmsr(KERNEL_GS_BASE));
    // }
    // color_printk(color, BLACK, "#schedule:%d,pid:%ld(%ld)=>>pid:%ld(%ld)#", jiffies,
    //              current->pid,current->vrun_time,tsk->pid,tsk->vrun_time);
    if (current->vrun_time > tsk->vrun_time || current->state != TASK_RUNNING) {
        if (current->state == TASK_RUNNING) {
            insert_task_queue(current);
        }
        if (!task_schedule[current->cpu_id].CPU_exec_task_jiffies) {
            switch (tsk->priority)
            {
            case 0:
            case 1:
                task_schedule[current->cpu_id].CPU_exec_task_jiffies = 4 / task_schedule[current->cpu_id].running_task_count;
                break;
            default:
                task_schedule[current->cpu_id].CPU_exec_task_jiffies = 4 / task_schedule[current->cpu_id].running_task_count * 3;
                break;
            }
        }

        task_schedule[current->cpu_id].switch_Time++;
        // if(glb_ind){
        //     color_printk(color, BLACK, "switch\n");
        // }
        switch_to(current, tsk);
    } else {
        insert_task_queue(tsk);

        if (!task_schedule[current->cpu_id].CPU_exec_task_jiffies) {
            switch (tsk->priority)
            {
            case 0:
            case 1:
                task_schedule[current->cpu_id].CPU_exec_task_jiffies = 4 / task_schedule[current->cpu_id].running_task_count;
                break;
            default:
                task_schedule[current->cpu_id].CPU_exec_task_jiffies = 4 / task_schedule[current->cpu_id].running_task_count * 3;
                break;
            }
        }
        // if(glb_ind){
        //     color_printk(color, BLACK, "\n");
        // }
    }
    sti;
}
void Init_schedule(void)
{
    int i;
    memset(&task_schedule[0], 0, sizeof(schedule_struct) * NR_CPUS);
    for (i = 0; i < NR_CPUS; i++) {
        List_Init(&task_schedule[i].task_queue.List);
        task_schedule[i].task_queue.vrun_time = 0x7fffffffffffffff;
        task_schedule[i].running_task_count = 1;
        task_schedule[i].CPU_exec_task_jiffies = 4;
    }
}
#pragma endregion

#pragma region trace
int lookup_kallsyms(unsigned long address,int level)
{
	int index = 0;
	int level_index = 0;
	char * string =(char *) &kallsyms_names;
	for(index = 0; index < kallsyms_syms_num; index++){
		if(address > kallsyms_addresses[index] && address <= kallsyms_addresses[index+1]){
            break;
        }
    }

	if(index < kallsyms_syms_num){
		for(level_index = 0; level_index < level; level_index++){
            color_printk(RED,BLACK,"  ");
        }
		color_printk(RED,BLACK,"+---> ");

		color_printk(RED,BLACK,"address:%p \t(+) %04d function:%s\n",
            address,
            address - kallsyms_addresses[index],
            &string[kallsyms_index[index]]
        );
		return 0;
	}else{
        return 1;
    }
}
void backtrace(unsigned long *rbp, unsigned long rip, unsigned long rsp)
{
    int i = 0;
    unsigned long ret_address = rip;
    color_printk(RED,BLACK,"&kallsyms_addresses:%p,kallsyms_addresses:%p\n", &kallsyms_addresses, kallsyms_addresses);
    color_printk(RED,BLACK,"&kallsyms_syms_num:%p,kallsyms_syms_num:%d\n", &kallsyms_syms_num, kallsyms_syms_num);
    color_printk(RED,BLACK,"&kallsyms_index:%p\n",&kallsyms_index);
    color_printk(RED,BLACK,"&kallsyms_names:%p,kallsyms_names:%s\n",&kallsyms_names, &kallsyms_names);
    color_printk(RED,BLACK,"====================== Kernel Stack Backtrace ======================\n");
    for(i = 0; i < 10; i++){
		if(lookup_kallsyms(ret_address,i)){
            break;
        }
		if((unsigned long)rbp < rsp){
            break;
        }
        //push rbp; mov rsp,rpb ====> rpb = current.rsp->caller.rpb, rpb+8 = caller.rip
		ret_address = *(rbp+1);
		rbp = (unsigned long *)*rbp;
    }
}
#pragma endregion