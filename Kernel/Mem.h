#pragma once

#include "lib.h"

#pragma region Definition
    #define PAGE_PML4E_SHIFT	39
    #define PAGE_PDPDE_SHIFT	30

    #define PAGE_2M_SHIFT	     21
    #define PAGE_2M_SIZE	    (1UL << PAGE_2M_SHIFT)
    #define PAGE_2M_MASK	    (~ (PAGE_2M_SIZE - 1))
    #define PAGE_2M_ALIGN(addr)	(((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)

    #define PAGE_4K_SHIFT	    12
    #define PAGE_4K_SIZE	    (1UL << PAGE_4K_SHIFT)
    #define PAGE_4K_MASK	    (~ (PAGE_4K_SIZE - 1))
    #define PAGE_4K_ALIGN(addr)	(((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)
    #define PG_PTable_Maped     (1 << 0)
    #define PG_Kernel_Init      (1 << 1)
    #define PG_Referenced	    (1 << 2)
    #define PG_Active           (1 << 4)
    #define PG_Kernel           (1 << 7)
    #define PG_K_Share_To_U	    (1 << 8)
#pragma endregion

#pragma region Glb_Var
    glb_mem_desc GLB_Men_Desc;
    unsigned long CR3;
    slab_cache kmalloc_cache_size[16] = 
    {
        {32	    ,0	,0	,NULL	,NULL   ,{NULL,NULL,NULL,0,0,0,0,NULL}},
        {64	    ,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},
        {128	,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},
        {256	,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},
        {512	,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},
        {1024	,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},			//1KB
        {2048	,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},
        {4096	,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},			//4KB
        {8192	,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},
        {16384	,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},
        {32768	,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},
        {65536	,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},			//64KB
        {131072	,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},			//128KB
        {262144	,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},
        {524288	,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},
        {1048576,0	,0	,NULL	,NULL	,{NULL,NULL,NULL,0,0,0,0,NULL}},			//1MB
    };
#pragma endregion

void Init_Mem(void);

static inline unsigned long Get_CR3(void)
{
	unsigned long CR3;
    __asm__ __volatile__	(
        "movq	%%cr3,	%0\n\t"
        :"=r"(CR3)
        :
        :"memory"
    );
    return CR3;
}

static inline void Rfrsh_CR3(unsigned long *PML4E)
{
	__asm__ __volatile__ 	(
        "movq	%%rax,	%%cr3\n\t"	
        :			
        :"a"(PML4E)
        :"memory"			
    );
}

unsigned long *PML4E;

void Init_Mem(void);
page* alloc_pages(int number, unsigned long page_flags);
unsigned long kernel_slab_init(void);
unsigned long free_pages(unsigned long PHY_Addr,int number);
unsigned long pagetable_init(unsigned long *PML4E, unsigned long PHY_Base, unsigned long Virtual_Base, unsigned long No_Pages, pg_attr *ATTR, int Print, int refr_CR3);
unsigned long pagetable_clear(unsigned long *PML4E, unsigned long Virtual_Base, unsigned long No_Pages, int Print, int refr_CR3);
void Display_PageTable(unsigned long *PML4E, unsigned long Virtual_Addr);
unsigned long Virt_To_Phy(unsigned long Virtual_Addr);