#include "TSS.h"



void SET_TSS64(unsigned long n,TSS_Struct *TSS_TABLE_PTR,unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,unsigned long ist2,unsigned long ist3,
    unsigned long ist4,unsigned long ist5,unsigned long ist6,unsigned long ist7)
{    
    Set_TSS_Desc((GDT_TSS_Desc *)(GDT_PTR.Offset + n), 1001, TSS_TABLE_PTR);

    load_TR(n);

    _set_tss64(rsp0, rsp1, rsp2, ist1, ist2, ist3, ist4, ist5, ist6, ist7);
}

void PrintTSS(unsigned int FRcolor, unsigned int BKcolor)
{
    TSS_Struct * TSS = get_TSS();

    color_printk(FRcolor, BKcolor, "TSS->RSP0: %p\n", TSS->RSP0);
    color_printk(FRcolor, BKcolor, "TSS->RSP1: %p\n", TSS->RSP1);
    color_printk(FRcolor, BKcolor, "TSS->RSP2: %p\n", TSS->RSP2);
    color_printk(FRcolor, BKcolor, "TSS->IST1: %p\n", TSS->IST1);
    color_printk(FRcolor, BKcolor, "TSS->IST2: %p\n", TSS->IST2);
}

void _set_tss64(unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,unsigned long ist2,unsigned long ist3,
    unsigned long ist4,unsigned long ist5,unsigned long ist6,unsigned long ist7)
{
    TSS_Struct * TSS = get_TSS();

    TSS->RSP0 = rsp0;
    TSS->RSP1 = rsp1;
    TSS->RSP2 = rsp2;
    TSS->IST1 = ist1;
    TSS->IST2 = ist2;
    TSS->IST3 = ist4;
    TSS->IST4 = ist4;
    TSS->IST5 = ist5;
    TSS->IST6 = ist6;
    TSS->IST7 = ist7;
}