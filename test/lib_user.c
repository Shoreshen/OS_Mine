#include "syscall.h"
#include "stdio.h"

int glb = 0;

#pragma region system_call
unsigned long putstring(char *string)
{
    unsigned long ret = SYS_PRINT;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret), "D"((unsigned long)string)
        : "memory"
    );
    return ret;
}
unsigned long open(char *filename, unsigned long flags)
{
    unsigned long ret = SYS_OPEN;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret), "D"((unsigned long)filename), "S"(flags)
        : "memory"
    );
    return ret;
}
unsigned long close(unsigned long fd)
{
    unsigned long ret = SYS_CLOSE;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret), "D"(fd)
        : "memory"
    );
    return ret;
}
unsigned long read(unsigned long fd, void *buf, unsigned long count)
{
    unsigned long ret = SYS_READ;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret), "D"(fd), "S"((unsigned long)buf), "d"(count)
        : "memory"
    );
    return ret;
}
unsigned long write(unsigned long fd, void *buf, unsigned long count)
{
    unsigned long ret = SYS_WRITE;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret), "D"(fd), "S"((unsigned long)buf), "d"(count)
        : "memory"
    );
    return ret;
}
unsigned long lseek(unsigned long fd, long offset, unsigned long whence)
{
    unsigned long ret = SYS_LSEEK;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret), "D"(fd), "S"(offset), "d"(whence)
        : "memory"
    );
    return ret;
}
unsigned long fork()
{
    unsigned long ret = SYS_FORK;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret)
        : "memory"
    );
    return ret;
}
unsigned long brk(unsigned long new_brk_end)
{
    unsigned long ret = SYS_BRK;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret), "D"(new_brk_end)
        : "memory"
    );
    return ret;
}
unsigned long clear(void)
{
    unsigned long ret = SYS_CLR;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret)
        : "memory"
    );
    return ret;
}
unsigned long reboot(unsigned long cmd)
{
    unsigned long ret = SYS_RBT;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret), "D"(cmd)
        : "memory"
    );
    return ret;
}
unsigned long chdir(unsigned long filename)
{
    unsigned long ret = SYS_CD;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret), "D"(filename)
        : "memory"
    );
    return ret;
}
unsigned long lsdir(unsigned long filename)
{
    unsigned long ret = SYS_LS;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret), "D"(filename)
        : "memory"
    );
    return ret;
}
unsigned long exec(unsigned long argc, unsigned long *argv)
{
    unsigned long ret = SYS_EXEC;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret), "D"(argc), "S"(argv)
        : "memory"
    );
    return ret;
}
unsigned long exit(unsigned long exit_code)
{
    unsigned long ret = SYS_EXIT;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret), "D"(exit_code)
        : "memory"
    );
    return ret;
}
unsigned long wait(unsigned long pid)
{
    unsigned long ret = SYS_WAIT;
    __asm__ __volatile__(
        "pushq %%r11\n\t"
        "leaq 1f(%%rip), %%rcx \n\t"
        "syscall \n\t"
        "1:	\n\t"
        "popq %%r11\n\t"
        : "=a"(ret)
        : "b"(ret), "D"(pid)
        : "memory"
    );
    return ret;
}
#pragma endregion

#pragma region memory
unsigned long malloc(unsigned long size)
{
    mall_des *tmp = NULL, *tmp2 = NULL;

    size = ((size + 7) & ~((1UL << 3) - 1));
    if(brk_st == 0){
        brk_st = brk(0);
        brk_ed = brk(brk_st + sizeof(mall_des));
        tmp = (mall_des *)brk_st;
        tmp->prev = NULL;
        tmp->next = NULL;
        tmp->end  = (unsigned long)tmp + sizeof(mall_des);
    }

    if(size){
        tmp = (mall_des *)brk_st;
        //If exists space to insert
        while(tmp->next){
            if((unsigned long)tmp->next - tmp->end > (size + sizeof(mall_des))){
                if(tmp->end > (unsigned long)tmp + sizeof(mall_des)){
                    tmp2 = (mall_des *)tmp->end;
                    
                    tmp2->prev = tmp;
                    tmp2->next = tmp->next;
                    tmp2->prev->next = tmp2;
                    tmp2->next->prev = tmp2;

                    tmp = tmp2;                 
                }
                tmp->end = (unsigned long)tmp + sizeof(mall_des) + size;
                return (unsigned long)tmp2 + sizeof(mall_des);
            }
            tmp = tmp->next;
        }
        //If tmp taken
        if(tmp->end > (unsigned long)tmp + sizeof(mall_des)){
            tmp->next = (mall_des *)tmp->end;
            tmp->next->prev = tmp;
            tmp = tmp->next;                
            tmp->next = NULL;
            tmp->end = (unsigned long)tmp + sizeof(mall_des);
        }
        //Allocate memory from kernel
        brk_ed = brk((unsigned long)tmp + sizeof(mall_des) + size);
        if(brk_ed >= (unsigned long)tmp + size + sizeof(mall_des)){
            tmp->end = (unsigned long)tmp + sizeof(mall_des) + size;
            return (unsigned long)tmp + sizeof(mall_des);
        }else{
            return 0;
        }
    }else{
        return 0;
    }
}
void free(void *addr)
{
    mall_des *tmp = addr - sizeof(mall_des);

    if(brk_st == 0 || (unsigned long)addr < brk_st || (unsigned long)addr > brk_ed){
        return;
    }
    tmp->end = (unsigned long)tmp + sizeof(mall_des);
    if(tmp->next == NULL){
        if(tmp->prev != NULL){
            tmp = tmp->prev;
            tmp->next = NULL;
        }
        brk_ed = brk(tmp->end);
    }
    
    return;
}
void* memcpy(void *From, void *To, long Num)    //Can not declare as inline, compiler will recompile it and put at the start of the .text seg
{                                               //If called in init.c, the first function will me "memcpy" instead of "main“
    __asm__ __volatile__	(					//__volatile__: keep instruction as written, do not optimize while compiling.
        "cld	\n\t"							//Clear direction: direction = upward
        "rep	movsq\n\t"						//Repeat move string quad(8 byte)
        "testb	$4,%b1	\n\t"					//test byte: 100b and lower 2 byte of 2end Cons(constraint), only effect EFLAG reg
        "je	1f	\n\t"							//Jump equal to symbal 1:
        "movsl	\n\t"							//Move string long(4 byte)
        "1:\t testb	$2,%b1	\n\t"				//Same as above
        "je	2f	\n\t"
        "movsw	\n\t"
        "2:\t testb	$1,%b1	\n\t"
        "je	3f	\n\t"
        "movsb	\n\t"
        "3:	\n\t"
        :										//Output: N/A
        :"c"(Num/8),"r"(Num),"D"(To),"S"(From)	//Input:
                                                    //(Num/8) => 0th Cons: "c"(RCX)
                                                    //(Num)   => 1st Cons: "r"(gcc pick register)
                                                    //(To)    => 2nd Cons: "D"(RDI)
                                                    //(From)  => 3Rd Cons: "S"(RSI)
        :										//clobbered:N/A
    );
    return To;
}
void* memset(void *Address, unsigned char C, long Count)
{
    int d0,d1;
    unsigned long tmp = C * 0x0101010101010101UL;
    __asm__	__volatile__	(	
        "cld	\n\t"
        "rep	\n\t"
        "stosq	\n\t"
        "testb	$4, %b3	\n\t"
        "je	1f	\n\t"
        "stosl	\n\t"
        "1:\ttestb	$2, %b3	\n\t"
        "je	2f\n\t"
        "stosw	\n\t"
        "2:\ttestb	$1, %b3	\n\t"
        "je	3f	\n\t"
        "stosb	\n\t"
        "3:	\n\t"
        :
        :"a"(tmp),"b"(Count),"c"(Count/8),"D"(Address)	
        :"memory"					
    );
    return Address;
}
int memcmp(void *FirstPart, void *SecondPart, long Count)
{
    register int __res;

    __asm__	__volatile__	(	
        "cld	\n\t"		//clean direct
        "repe	cmpsb\n\t"		//repeat if equal
        "je	1f	\n\t"
        "movl	$1,	%%eax	\n\t"
        "jl	1f	\n\t"
        "negl	%%eax	\n\t"
        "1:	\n\t"
        :"=a"(__res)
        :"0"(0),"D"(FirstPart),"S"(SecondPart),"c"(Count)
        :
    );
    return __res;
}
#pragma endregion

#pragma region string
int strlen(char * String)
{
    register int __res;
    __asm__	__volatile__ (	                    //__volatile__: Complier will not optimize instructions
        "cld	\n\t"                           //Clear direction: direction = upward
        "repne	\n\t"                           //Repeat not equal: Repeat while the ECX register not zero and the ZF flag is clear
                                                //					Each time ECX - 1
        "scasb  \n\t"                           //Scan string byte: Compare byte of RDI and AL, set EFLAG(here mainly ZF)
        "notl	%0	\n\t"                       //Logical instruction 0->1, 1->0, operating on 0th Cons(constraint)
        "decl	%0	\n\t"                       //minuse 1, operating on register %0
        :"=c"(__res)                            //Output: 
                                                    //0th Cons: "c"(RCX) => __res
        :"D"(String),"a"(0),"0"(0xffffffff)     //Input:
                                                    //(String)     => 1st Cons: "D"(RDI)
                                                    //(0)          => 2nd Cons: "a"(RAX)
                                                    //(0xffffffff) => 0th Output("=c")
        :                                       //clobbered: N/A
        );
    return __res;
}
int strcmp(char * FirstPart,char * SecondPart)
{
    register int __res;
    __asm__	__volatile__	(	
        "cld	\n\t"
        "1:	\n\t"
        "lodsb	\n\t"
        "scasb	\n\t"
        "jne	2f	\n\t"
        "testb	%%al,	%%al	\n\t"
        "jne	1b	\n\t"
        "xorl	%%eax,	%%eax	\n\t"
        "jmp	3f	\n\t"
        "2:	\n\t"
        "movl	$1,	%%eax	\n\t"
        "jl	3f	\n\t"
        "negl	%%eax	\n\t"
        "3:	\n\t"
        :"=a"(__res)
        :"D"(FirstPart),"S"(SecondPart)
        :					
    );
    return __res;
}
char * strncpy(char * Dest,char * Src,long Count)
{
    __asm__	__volatile__	(	
        "cld	\n\t"
        "1:	\n\t"
        "decq	%2	\n\t"
        "js	2f	\n\t"
        "lodsb	\n\t"
        "stosb	\n\t"
        "testb	%%al,	%%al	\n\t"
        "jne	1b	\n\t"
        "rep	\n\t"
        "stosb	\n\t"
        "2:	\n\t"
        :
        :"S"(Src),"D"(Dest),"c"(Count)
        :"ax","memory"				
    );
    return Dest;
}
char * strcpy(char * Dest,char * Src)
{
	__asm__	__volatile__	(	
        "cld	\n\t"
        "1:	\n\t"
        "lodsb	\n\t"
        "stosb	\n\t"
        "testb	%%al,	%%al	\n\t"
        "jne	1b	\n\t"
        :
        :"S"(Src),"D"(Dest)
        :"ax","memory"
    );
	return 	Dest;
}
char * strcat(char * Dest,char * Src)
{
	__asm__	__volatile__	(	
        "cld	\n\t"
        "repne	\n\t"
        "scasb	\n\t"
        "decq	%1	\n\t"
        "1:	\n\t"
        "lodsb	\n\t"
        "stosb	\n\r"
        "testb	%%al,	%%al	\n\t"
        "jne	1b	\n\t"
        :
        :"S"(Src),"D"(Dest),"a"(0),"c"(0xffffffff)
        :"memory"				
    );
	return Dest;
}
#pragma endregion

#pragma region shell
int cd_command(int argc, unsigned long *argv){
    int len;
    unsigned long i;
    char *path = NULL;

    if(!strcmp(".", (char *)argv[1])){
        return 1;
    }else if(!strcmp("..", (char *)argv[1])){
        if(!strcmp("/", current_dir)){
            return 0;
        }
        for(i = strlen(current_dir) - 1; i > 1; i-- ){
            if(current_dir[i] == '/'){
                break;
            }
        }
        current_dir[i] = '\0';
        printf("pwd switch to %s\n",current_dir);
        return 1;
    }else{
        len = strlen(current_dir);
        i = len + strlen((char *)argv[1]) + 2;
        path = (char *)malloc(i);
        memset(path, 0 ,i);
        strcpy(path, current_dir);
        if(len>1){
            path[len] = '/';
        }
        strcat(path,(char *)argv[1]);
        printf("cd_command :%s\n",path);

        i = chdir((unsigned long)path);
        if(!i){
            current_dir = path;
        } else{
            printf("Can`t goto directory %s\n",argv[1]);
        }
        printf("pwd switch to %s\n",current_dir);
    }
}
int ls_command(int argc, unsigned long *argv)
{
    if(argc > 1){
        lsdir(argv[1]);
    }else{
        lsdir((unsigned long)current_dir);
    }
    return 1;
}
int pwd_command(int argc, unsigned long *argv)
{
	if(current_dir)
		printf("%s\n",current_dir);
	return 1;
}
int cat_command(int argc, unsigned long *argv)
{
    int len = 0, i = 0, fd = 0;
    char *filename = NULL, *buf = NULL;

    len = strlen(current_dir);
    i = len + strlen((char *)argv[1]);
    filename = (char *)malloc(i+2);
    memset(filename,0,i+2);
    strcpy(filename,current_dir);
    if(len > 1){
        filename[len] = '/';
    }
    strcat(filename,(char *)argv[1]);
    printf("cat_command filename:%s\n",filename);

    fd = open(filename,0);
    i = lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);
    buf = (char *)malloc(i+1);
    memset(buf,0,i+1);
    len = read(fd,buf,i);
    printf("length:%d\t%s\n",len,buf);
    close(fd);

    return 1;
}
int touch_command(int argc, unsigned long *argv){}
int rm_command(int argc, unsigned long *argv){}
int mkdir_command(int argc, unsigned long *argv){}
int rmdir_command(int argc, unsigned long *argv){}
int exec_command(int argc, unsigned long *argv)
{
    unsigned long error = NULL, exit_code = NULL;
    error = fork();
	if(error == 0){
        putstring("child process\n");
        exec(argc, argv);
        // while(1);
        exit(10);
    }else{
        printf("parent process:%d\n",error);
        exit_code = wait(error);
        printf("exit_code: %d\n", exit_code);
    }
    return 1;
}
int reboot_command(int argc, unsigned long *argv){
    reboot(SYSTEM_REBOOT);
    return 0;
}
int clear_command(int argc, unsigned long *argv){
    clear();
    return 1;
}

struct	buildincmd shell_internal_cmd[] = 
{
	{"cd",cd_command},
	{"ls",ls_command},
	{"pwd",pwd_command},
	{"cat",cat_command},
	{"touch",touch_command},
	{"rm",rm_command},
	{"mkdir",mkdir_command},
	{"rmdir",rmdir_command},
	{"exec",exec_command},
	{"reboot",reboot_command},
    {"clear",clear_command},
};

int find_cmd(char *cmd)
{
	int i = 0;
	for(i = 0;i<sizeof(shell_internal_cmd)/sizeof(struct buildincmd);i++){
		if(!strcmp(cmd,shell_internal_cmd[i].name)){
            // printf("cmd: %s",shell_internal_cmd[i].name);
            return i; 
        } 
    }
	return -1;
}
void run_cmd(int index,int argc,unsigned long* arg_pos)
{
	printf("run_command %s\n",shell_internal_cmd[index].name);
	shell_internal_cmd[index].function(argc,arg_pos);
}
#pragma endregion