#include "stdio.h"
#include "syscall.h"
#include "printf.h"

int main(int argc,char *argv[])
{
	int i = 0;
	printf("Hello World!\n");
	printf("argc:%d,argv:%p\n",argc,argv);
	for(i = 0;i<argc;i++){
		printf("argv[%d]:%s\n",i,argv[i]);
	}
	exit(i);
	return 0;
}
