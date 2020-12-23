#pragma once
#include <stdarg.h>
#include "stdio.h"

#pragma region Define: Style
    #define ZEROPAD	1		/* pad with zero "%02d": print 2 or more digits, if less than 2 digits, fill up 0 instead, not valid for LEFT set*/
    #define SIGN	2		/* unsigned/signed long */
    #define PLUS	4		/* show plus */
    #define SPACE	8		/* space if plus */
    #define LEFT	16		/* left justified */
    #define SPECIAL	32		/* 0x for HEX, 0 for OCT*/
    #define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */
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

#pragma region function
int vsprintf(char * buf, char *fmt, va_list args);
char* number(char *str, long num, int base, int field_width, int precision, int type);
int skip_atoi(char **s);
#pragma endregion