#pragma once
#include "lib.h"

#pragma region Define: Style
    #define ZEROPAD	1		/* pad with zero "%02d": print 2 or more digits, if less than 2 digits, fill up 0 instead, not valid for LEFT set*/
    #define SIGN	2		/* unsigned/signed long */
    #define PLUS	4		/* show plus */
    #define SPACE	8		/* space if plus */
    #define LEFT	16		/* left justified */
    #define SPECIAL	32		/* 0x for HEX, 0 for OCT*/
    #define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */
#pragma endregion

#pragma region Define: Colors
    #define WHITE 	0x00ffffff		//白
    #define BLACK 	0x00000000		//黑
    #define RED	    0x00ff0000		//红
    #define ORANGE	0x00ff8000		//橙
    #define YELLOW	0x00ffff00		//黄
    #define GREEN	0x0000ff00		//绿
    #define BLUE	0x000000ff		//蓝
    #define INDIGO	0x0000ffff		//靛
    #define PURPLE	0x008000ff		//紫
#pragma endregion

#pragma region Define: Functions
    #define is_digit(c)	((c) >= '0' && (c) <= '9')
    #define do_div(n,base)\
            ({\
                int __res; \
                __asm__(\
                    "divq %%rcx"\
                    :"=a" (n),"=d" (__res)\
                    :"0" (n),"1" (0),"c" (base)\
                );\
                __res;\
            })
    //Can not apply function, due to n need to be outputed
    //{"divq %%rcx"} = DIV instruction: divide RDX:RAX by RCX; Results in: RAX ← Quotient, RDX ← Remainder
    //{:"=a" (n),"=d" (__res)} = output_registers:
        //RAX => n;
        //RDX => __res;
    //{:"0" (n),"1" (0),"c" (base))} = Input_registers:
        //0th Output_Reg = base
        //1st Output_Reg = n
        //"d"(RDX) = 0
#pragma endregion

#pragma region Struct: Buffer, Screen cursor
    char buf[4096]={0};
    position Pos;
#pragma endregion

#pragma region Functions
int color_printk(unsigned int FRcolor, unsigned int BKcolor, char * fmt, ...);
void putchar(unsigned int FRcolor,unsigned int BKcolor,unsigned char font);
char* number(char *str, long num, int base, int field_width, int precision, int type);
#pragma endregion