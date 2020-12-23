#pragma once
#include "lib.h"


#define load_TR(n)\
    {\
        __asm__ __volatile__(\
            "ltr	%%ax"\
            :\
            :"a"(n << 3)\
            :"memory"\
        );\
    }

static inline void Set_TSS_Desc(GDT_TSS_Desc *TSS_Desc, unsigned int TYPE, TSS_Struct *TSS_Table)
{
	unsigned long p = (unsigned long)TSS_Table;
    
    *((unsigned long*)TSS_Desc) = 0UL;
    *((unsigned long*)TSS_Desc + 1) = 0UL;

    TSS_Desc->TYPE = TYPE;
    TSS_Desc->Limit0 = 103;
    TSS_Desc->p = 1;

    TSS_Desc->Base0 = p;
    TSS_Desc->Base1 = p >> 24;
    TSS_Desc->Base2 = p >> 32;
}

void SET_TSS64(unsigned long n,TSS_Struct *TSS_TABLE_PTR,
    unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,
    unsigned long ist1,unsigned long ist2,unsigned long ist3,
    unsigned long ist4,unsigned long ist5,unsigned long ist6,
    unsigned long ist7);
void _set_tss64(unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,unsigned long ist2,unsigned long ist3,
    unsigned long ist4,unsigned long ist5,unsigned long ist6,unsigned long ist7);