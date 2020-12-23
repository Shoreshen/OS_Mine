#include "syscall.h"
#include "stdio.h"
#include "printf.h"
#include "keyboard.h"

int main()
{
    unsigned long fd = 0, i = 500, key = 0,*arg_pos;
    char keyboard[] = "keyboard.dev";
    unsigned char buf[256] = {0};
    int arg_count, index;

    brk_st = 0;
    brk_ed = 0;

    // clear();
	// if(fork() == 0){
    //     putstring("child process\n");
    // }else{
    //     putstring("parent process\n");
    // }
    current_dir = (char *)malloc(3);
    strcpy(current_dir,"/");
    fd = open(&keyboard[0],0);

    while(i--){
		arg_count = 0;
        arg_pos = NULL;
        printf("[SHELL]#:_");
        memset(buf ,0 ,BUF_SIZE);
        read_line(fd, &buf[0]);
        printf("\n");
        arg_pos = parse_cmd(&buf[0], &arg_count);
        if(arg_pos){
            index = find_cmd((char *)arg_pos[0]);
            if(index<0){
                printf("Input Error,No Command Found!\n");
            }else{
                run_cmd(index, arg_count, arg_pos);
            }
            free(arg_pos);
        }
    }

    close(fd);

    while(1);
    return 0;
}