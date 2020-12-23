#include "Mem.h"

void Init_Mem(void)
{
    e820* p = (e820*)&kernel_boot_para_info->E820_Info.E820_Entry[0];
    GLB_Men_Desc.No_E820 = 0;
    GLB_Men_Desc.No_Page = 0;
    GLB_Men_Desc.Used_Pages = 0;
    unsigned long TotMem = 0,MemEnd = 0, i, start, end, j;


    #pragma region Pre-Fetch Info E820
    GLB_Men_Desc.start_code = (unsigned long)&_text;
    GLB_Men_Desc.end_code   = (unsigned long)&_etext;
    GLB_Men_Desc.end_data   = (unsigned long)&_edata;
    GLB_Men_Desc.end_rodata = (unsigned long)&_erodata;
    GLB_Men_Desc.end_brk    = (unsigned long)&_end;
    GLB_Men_Desc.struct_end = GLB_Men_Desc.end_brk;
    //GLB_Men_Desc.E820 start after GLB_Men_Desc.struct_end, aligned at 4kb address
    GLB_Men_Desc.E820 = (e820*)PAGE_4K_ALIGN(GLB_Men_Desc.struct_end);
        
    do{
        GLB_Men_Desc.E820[GLB_Men_Desc.No_E820].address = p[GLB_Men_Desc.No_E820].address;
        GLB_Men_Desc.E820[GLB_Men_Desc.No_E820].length  = p[GLB_Men_Desc.No_E820].length;
        GLB_Men_Desc.E820[GLB_Men_Desc.No_E820].type    = p[GLB_Men_Desc.No_E820].type;

        if(GLB_Men_Desc.E820[GLB_Men_Desc.No_E820].type == 1){
            MemEnd += p[GLB_Men_Desc.No_E820].length;
            TotMem = p[GLB_Men_Desc.No_E820].address + p[GLB_Men_Desc.No_E820].length;
        }
		GLB_Men_Desc.No_E820++;
    }while(p[GLB_Men_Desc.No_E820].type <=4 && p[GLB_Men_Desc.No_E820].type >=1);
    //Adjust GLB_Men_Desc.struct_end
    GLB_Men_Desc.struct_end = (unsigned long)GLB_Men_Desc.E820 + GLB_Men_Desc.No_E820 * sizeof(e820);
    GLB_Men_Desc.No_Page    = MemEnd >> PAGE_2M_SHIFT;
    #pragma endregion

    #pragma region Bits_map
    GLB_Men_Desc.bits_map = (unsigned long*)PAGE_4K_ALIGN(GLB_Men_Desc.struct_end);
    GLB_Men_Desc.bits_length = (GLB_Men_Desc.No_Page + sizeof(long) * 8 -1) >> 6 << 3;
    memset(GLB_Men_Desc.bits_map, 0xff, GLB_Men_Desc.bits_length);
    GLB_Men_Desc.struct_end = PAGE_4K_ALIGN(GLB_Men_Desc.bits_map + GLB_Men_Desc.bits_length);
    #pragma endregion

    #pragma region Pages
    GLB_Men_Desc.Page = (page*)GLB_Men_Desc.struct_end;
    for(i=0; i<GLB_Men_Desc.No_Page; i++) {
        GLB_Men_Desc.Page[i].PHY_Address = PAGE_2M_SIZE * i;
        *(GLB_Men_Desc.bits_map + (i>>6)) &= ~(1UL << i % 64);
    }
    TotMem = PAGE_2M_ALIGN(GLB_Men_Desc.struct_end - PAGE_OFFSET) >> PAGE_2M_SHIFT;
    for(i=0; i<TotMem; i++){
        GLB_Men_Desc.Page[i].Attr = (PG_PTable_Maped | PG_Kernel_Init | PG_Active | PG_Kernel);
        *(GLB_Men_Desc.bits_map + (i>>6)) |= 1UL << i % 64;
        GLB_Men_Desc.Page[i].Ref_Count++;
        GLB_Men_Desc.Used_Pages++;
    }
    GLB_Men_Desc.struct_end = PAGE_4K_ALIGN((unsigned long)GLB_Men_Desc.Page + GLB_Men_Desc.No_Page * sizeof(page));
    #pragma endregion

    #pragma region Print mem Info
    if(MACRO_PRINT){
        color_printk(WHITE,BLACK,"code start: \t%p\ncode end: \t%p\ndata end: \t%p\nrodata end: \t%p\nbrk end: \t%p\nstruct end: \t%p\n",
            GLB_Men_Desc.start_code,
            GLB_Men_Desc.end_code,
            GLB_Men_Desc.end_data,
            GLB_Men_Desc.end_rodata,
            GLB_Men_Desc.end_brk,
            GLB_Men_Desc.struct_end);
        
        color_printk(BLUE,BLACK,"Display Physics Address MAP,\nType(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
        for(i=0; i<GLB_Men_Desc.No_E820; i++){
            color_printk(ORANGE,BLACK,"Address:%p, Length:%p, Type:%#x\n",
                GLB_Men_Desc.E820[i].address, 
                GLB_Men_Desc.E820[i].length, 
                GLB_Men_Desc.E820[i].type);
        }
        color_printk(ORANGE,BLACK,"OS Can Used Total RAM:%dMB\n", MemEnd/1024/1024);
        color_printk(ORANGE,BLACK,"Total pages:%d\n", GLB_Men_Desc.No_Page);
        color_printk(ORANGE,BLACK,"E820 start:%p\n", GLB_Men_Desc.E820);
        color_printk(ORANGE,BLACK,"Bits_map start: %p\tBits_length: %d\n", GLB_Men_Desc.bits_map, GLB_Men_Desc.bits_length);
        color_printk(ORANGE,BLACK,"Bits_map 1st: %p\tBits_map last: %p\n", *GLB_Men_Desc.bits_map, *(GLB_Men_Desc.bits_map + (GLB_Men_Desc.bits_length >> 3) - 1));
        color_printk(ORANGE,BLACK,"Page structure start: %p\ttotal number of 2MB page allocated: %d\tTotal page used: %d\n", 
            GLB_Men_Desc.Page, GLB_Men_Desc.No_Page, GLB_Men_Desc.Used_Pages);
        color_printk(ORANGE,BLACK,"Memory structure end:%p\n", (unsigned long)GLB_Men_Desc.Page + sizeof(page) * GLB_Men_Desc.No_Page);
        color_printk(PURPLE,BLACK,"Page[0].Attr: %p, Page[0].Addr: %p, Page[0].Ref_Count: %d\n",
            GLB_Men_Desc.Page[0].Attr,
            GLB_Men_Desc.Page[0].PHY_Address,
            GLB_Men_Desc.Page[0].Ref_Count);
        color_printk(PURPLE,BLACK,"Page[1].Attr: %p, Page[1].Addr: %p, Page[1].Ref_Count: %d\n",
            GLB_Men_Desc.Page[1].Attr,
            GLB_Men_Desc.Page[1].PHY_Address,
            GLB_Men_Desc.Page[1].Ref_Count);
    }
    #pragma endregion
}

unsigned long kernel_slab_init(void)
{
    unsigned long i, j, temp_struct_end_aligned;

    temp_struct_end_aligned = PAGE_2M_ALIGN(GLB_Men_Desc.struct_end - PAGE_OFFSET) >> PAGE_2M_SHIFT;

    #pragma region Setting values for kmalloc_cache_size[i].cache_pool
    for(i=0; i<16; i++){
        List_Init(&kmalloc_cache_size[i].cache_pool.List);
        kmalloc_cache_size[i].cache_pool.color_count    = PAGE_2M_SIZE / kmalloc_cache_size[i].size;
        kmalloc_cache_size[i].cache_pool.color_length   = ((kmalloc_cache_size[i].cache_pool.color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3;
        kmalloc_cache_size[i].cache_pool.color_map      = (unsigned long *)GLB_Men_Desc.struct_end;
        GLB_Men_Desc.struct_end = 
            (unsigned long)(GLB_Men_Desc.struct_end + kmalloc_cache_size[i].cache_pool.color_length + sizeof(long) * 10) & ( ~ (sizeof(long) - 1));
        memset(kmalloc_cache_size[i].cache_pool.color_map,0xff,kmalloc_cache_size[i].cache_pool.color_length);
		for(j = 0;j < kmalloc_cache_size[i].cache_pool.color_count;j++){
            *(kmalloc_cache_size[i].cache_pool.color_map + (j >> 6)) ^= 1UL << (j % 64);
        }
    }
    #pragma endregion

    #pragma region Marking page structure for extra memeories
    i = PAGE_2M_ALIGN(GLB_Men_Desc.struct_end - PAGE_OFFSET) >> PAGE_2M_SHIFT;
    
    for(j = temp_struct_end_aligned; j < i; j++){
        GLB_Men_Desc.Page[j].Attr = (PG_PTable_Maped | PG_Kernel_Init | PG_Kernel);
        *(GLB_Men_Desc.bits_map + (j>>6)) |= 1UL << j % 64;
        GLB_Men_Desc.Page[j].Ref_Count++;
        GLB_Men_Desc.Used_Pages++;
    }
    #pragma endregion
    
    #pragma region Allocating pages for kmalloc_cache_size
    page* Page = alloc_pages(16, PG_PTable_Maped | PG_Kernel_Init | PG_Kernel);
    for(i = 0; i < 16; i++){
        kmalloc_cache_size[i].cache_pool.Page = &Page[i];
        kmalloc_cache_size[i].cache_pool.Vaddress = (void *)((unsigned long)kmalloc_cache_size[i].cache_pool.Page->PHY_Address + PAGE_OFFSET);

        kmalloc_cache_size[i].cache_pool.free_count = kmalloc_cache_size[i].cache_pool.color_count;
        kmalloc_cache_size[i].cache_pool.using_count = 0;

        kmalloc_cache_size[i].total_free = kmalloc_cache_size[i].cache_pool.free_count;
        kmalloc_cache_size[i].total_using = 0;
    }
    #pragma endregion
}

void* kmalloc(unsigned long size, unsigned long gfp_flages)
{
    unsigned long TotalFree, i, j;
    slab *Slab;
    page *Page;

    if(size > 1048576)
	{
		color_printk(RED,BLACK,"kmalloc() ERROR: kmalloc size too long:%d\n",size);
		return NULL;
	}
    for(i=0;i<16;i++){
        if(kmalloc_cache_size[i].size >= size){
            Slab = &kmalloc_cache_size[i].cache_pool;
            break;
        }
    }

    if(kmalloc_cache_size[i].total_free != 0){
        do{
            if(Slab->free_count == 0){
                Slab = container_of(Slab->List.next, slab, List);
            }else{
                for(j = 0; j < Slab->color_count; j++){
                    if(*(Slab->color_map + (j >> 6)) == 0xffffffffffffffffUL)
                    {
                        j += 63;
                        continue;
                    }
                    if((*(Slab->color_map + (j >> 6)) & (1UL << (j % 64))) == 0){
                        *(Slab->color_map + (j >> 6)) |= 1UL << (j % 64);
                        Slab->using_count++;
                        Slab->free_count--;

                        kmalloc_cache_size[i].total_free--;
                        kmalloc_cache_size[i].total_using++;

                        return (void *)((char *)Slab->Vaddress + kmalloc_cache_size[i].size * j);
                    }
                }
            }
        } while (Slab != &kmalloc_cache_size[i].cache_pool);
    }else{
        Page = alloc_pages(1, PG_Kernel);

        if(kmalloc_cache_size[i].size <= 512){
            TotalFree = PAGE_2M_SIZE - sizeof(slab) - PAGE_2M_SIZE/kmalloc_cache_size[i].size/8;

            Slab = (slab*)((unsigned long)Page->PHY_Address + PAGE_OFFSET + TotalFree);
            Slab->color_map = (unsigned long *)((unsigned long)Slab + sizeof(slab));

            TotalFree = TotalFree / kmalloc_cache_size[i].size;
        }else{
            Slab = (slab *)kmalloc(sizeof(slab), 0);
            Slab->color_map = (unsigned long *)kmalloc(kmalloc_cache_size[i].cache_pool.color_length, 0);
            TotalFree = PAGE_2M_SIZE / kmalloc_cache_size[i].size;
        }
        
        Slab->Page          = Page;
        Slab->color_count   = TotalFree;
        Slab->color_length  = ((Slab->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3;
        Slab->free_count    = Slab->color_count - 1;
        Slab->using_count   = 1;
        Slab->Vaddress      = (void *)((unsigned long)Page->PHY_Address + PAGE_OFFSET);

        memset(Slab->color_map, 0xff, Slab->color_length);
        for(j = 1;j < Slab->color_count;j++){
            *(Slab->color_map + (j >> 6)) ^= 1UL << j % 64;
        }

        List_Init(&Slab->List);
        List_Insert_Before(&kmalloc_cache_size[i].cache_pool.List,&Slab->List);

        kmalloc_cache_size[i].total_free += Slab->free_count;
        kmalloc_cache_size[i].total_using += Slab->using_count;

        return Slab->Vaddress;
    }

	color_printk(BLUE,BLACK,"kmalloc() ERROR: no memory can alloc\n");
	return NULL;
}

unsigned long kfree(void * address)
{
    unsigned long i, index;
    slab *Slab;
    void * page_base_address = (void *)((unsigned long)address & PAGE_2M_MASK);
    for(i = 0; i < 16; i++){
        Slab = &kmalloc_cache_size[i].cache_pool;
        do{
            if(page_base_address == Slab->Vaddress){
				index = (address - Slab->Vaddress) / kmalloc_cache_size[i].size;
				*(Slab->color_map + (index >> 6)) ^= 1UL << index % 64;

                Slab->using_count--;
                Slab->free_count++;
                kmalloc_cache_size[i].total_using--;
                kmalloc_cache_size[i].total_free++;

                if((Slab->using_count == 0) && (kmalloc_cache_size[i].total_free >= Slab->color_count * 3 / 2) && (&kmalloc_cache_size[i].cache_pool != Slab)){
                    if(kmalloc_cache_size[i].size <= 512){
                        List_Del(&Slab->List);
                        kmalloc_cache_size[i].total_free -= Slab->color_count;
                        free_pages(Slab->Page->PHY_Address, 1);
                    }else{
                        List_Del(&Slab->List);
                        kmalloc_cache_size[i].total_free -= Slab->color_count;
                        kfree(Slab->color_map);
                        free_pages(Slab->Page->PHY_Address, 1);
                        kfree(Slab);
                    }
                }
                return 1;
            }else{
                Slab = container_of(Slab->List.next, slab, List);
            }
        }while(Slab != &kmalloc_cache_size[i].cache_pool);
    }

	color_printk(RED,BLACK,"kfree() ERROR: can`t free memory\n");
	
	return 0;
}

page* alloc_pages(int number, unsigned long page_flags)
{
    unsigned long i, j, k, num;

    if(number < 1 || number > 63){
		color_printk(RED,BLACK,"alloc_pages() ERROR: number is invalid\n");
		return NULL;
    }

    for(i = 0; i < GLB_Men_Desc.bits_length / 8; i++){
        num = (1UL << number) - 1;
        for(j = 0; j < 64; j++){
            if(!((j ? (GLB_Men_Desc.bits_map[i] >> j | GLB_Men_Desc.bits_map[i + 1] << (64 - j)) : GLB_Men_Desc.bits_map[i]) & num)){
                GLB_Men_Desc.Used_Pages += number;
                for(k=0; k<number; k++){
                    *(GLB_Men_Desc.bits_map + ((GLB_Men_Desc.Page[i * 64 + j + k].PHY_Address >> PAGE_2M_SHIFT)>>6)) |= 
                        1UL << (GLB_Men_Desc.Page[i * 64 + j + k].PHY_Address >> PAGE_2M_SHIFT) % 64;
                    GLB_Men_Desc.Page[i * 64 + j + k].Attr = page_flags;
                    GLB_Men_Desc.Page[i * 64 + j + k].Ref_Count++;
                }
                return &GLB_Men_Desc.Page[i * 64 + j];
            }
        }
    }

    return NULL;
}

unsigned long free_pages(unsigned long PHY_Addr,int number)
{
	unsigned long i = 0, p_no = 0;

	if(number >= 64 || number <= 0){
		color_printk(RED,BLACK,"free_pages() ERROR: number is invalid\n");
		return NULL;	
	}

	GLB_Men_Desc.Used_Pages -= number;
	for(i = 0; i < number; i++){
        p_no = PHY_Addr >> PAGE_2M_SHIFT;
        if(GLB_Men_Desc.Page[p_no].PHY_Address != PHY_Addr){
            color_printk(RED, BLACK, "Physical address not match\n");
            while(1);
        }
		*(GLB_Men_Desc.bits_map + ((PHY_Addr >> PAGE_2M_SHIFT) >> 6)) &= ~(1UL << (PHY_Addr >> PAGE_2M_SHIFT) % 64);
		GLB_Men_Desc.Page[p_no].Attr = 0;
        GLB_Men_Desc.Page[p_no].Ref_Count = 0;
	}

    return 1;
}

unsigned long pagetable_clear(unsigned long *PML4E, unsigned long Virtual_Base, unsigned long No_Pages, int Print, int refr_CR3)
{
    unsigned long i, pos, *PDPDE = NULL, *PDE = NULL, Phy_Base = NULL;

    PML4E = (unsigned long *)((unsigned long)PML4E + PAGE_OFFSET);

    if(Virtual_Base & (PAGE_2M_SIZE - 1)){
        color_printk(RED, BLACK, "pagetable_init() ERROR\n");
        return NULL;
    }
    //Clear page table content
    for(i = 0; i < No_Pages; i++){
        //PML4E
        pos = ((i * PAGE_2M_SIZE + Virtual_Base) >> PAGE_PML4E_SHIFT) & 0x1FF;
        if(PML4E[pos] == 0){
            return Phy_Base;
        }
        //PDPDE
        PDPDE = (unsigned long *)((PML4E[pos] & (~ 0xfffUL)) + PAGE_OFFSET);
        pos = ((i * PAGE_2M_SIZE + Virtual_Base) >> PAGE_PDPDE_SHIFT) & 0x1FF;
        if(PDPDE[pos] == 0){
            return Phy_Base;
        }
        //PDE
        PDE = (unsigned long *)((PDPDE[pos] & (~ 0xfffUL)) + PAGE_OFFSET);
        pos = ((i * PAGE_2M_SIZE + Virtual_Base) >> PAGE_2M_SHIFT) & 0x1FF;
        Phy_Base = PDE[pos] & (~ 0xfffUL);
        PDE[pos] = NULL;
    }
    //Refresh
    if(refr_CR3){
        Rfrsh_CR3((unsigned long *)((unsigned long)PML4E - PAGE_OFFSET));
    }

    return Phy_Base;
}

unsigned long pagetable_init(unsigned long *PML4E, unsigned long PHY_Base, unsigned long Virtual_Base, unsigned long No_Pages, pg_attr *ATTR, int Print, int refr_CR3)
{
    unsigned long i, pos, start, end, *virtual = NULL, *PDPDE = NULL, *PDE = NULL;

    PML4E = (unsigned long *)((unsigned long)PML4E + PAGE_OFFSET);

    if(Print){
        color_printk(YELLOW, BLACK, "Mapping physical address: %p~%p\nTo virtual address：%p~%p, total of: %d pages.\n",
            PHY_Base, PHY_Base + No_Pages * PAGE_2M_SIZE,
            Virtual_Base, Virtual_Base + No_Pages * PAGE_2M_SIZE,
            No_Pages
        );
    }
    if(Virtual_Base & (PAGE_2M_SIZE - 1) || PHY_Base & (PAGE_2M_SIZE - 1)){
        color_printk(RED, BLACK, "pagetable_init() ERROR\n");
        return NULL;
    }
    
    for(i = 0; i < No_Pages; i++){
        //PML4E
        pos = ((i * PAGE_2M_SIZE + Virtual_Base) >> PAGE_PML4E_SHIFT) & 0x1FF;
        if(PML4E[pos] == 0){
            virtual = (unsigned long *)((unsigned long)kmalloc(PAGE_4K_SIZE, 0) - PAGE_OFFSET);
            if(!virtual){
                return NULL;
            }
            memset((void *)((unsigned long)virtual + PAGE_OFFSET), 0, PAGE_4K_SIZE);
            if(Print){
                color_printk(YELLOW, BLACK, "New PDPDE, Phy_Addr: %p\n",virtual);
            }
            PML4E[pos] = (unsigned long)virtual;
        }
        PML4E[pos] |= *(unsigned long *)(&ATTR->PML4E_Attr);
        //PDPDE
        PDPDE = (unsigned long *)((PML4E[pos] & (~ 0xfffUL)) + PAGE_OFFSET);
        pos = ((i * PAGE_2M_SIZE + Virtual_Base) >> PAGE_PDPDE_SHIFT) & 0x1FF;
        if(PDPDE[pos] == 0){
            virtual = (unsigned long *)((unsigned long)kmalloc(PAGE_4K_SIZE, 0) - PAGE_OFFSET);
            if(!virtual){
                return NULL;
            }
            memset((void *)((unsigned long)virtual + PAGE_OFFSET), 0, PAGE_4K_SIZE);
            if(Print){
                color_printk(YELLOW, BLACK, "New PDE, Phy_Addr: %p\n",virtual);
            }
            PDPDE[pos] = (unsigned long)virtual;
        }
        PDPDE[pos] |= *(unsigned long *)(&ATTR->PDPTE_Attr);
        //PDE
        PDE = (unsigned long *)((PDPDE[pos] & (~ 0xfffUL)) + PAGE_OFFSET);
        pos = ((i * PAGE_2M_SIZE + Virtual_Base) >> PAGE_2M_SHIFT) & 0x1FF;
        PDE[pos] = (i * PAGE_2M_SIZE + PHY_Base) | *(unsigned long *)(&ATTR->PDE_Attr);
    }
    if(refr_CR3){
        Rfrsh_CR3((unsigned long *)((unsigned long)PML4E - PAGE_OFFSET));
    }
    return No_Pages;
}

unsigned long Virt_To_Phy(unsigned long Virtual_Addr)
{
    unsigned long *PML4E = (unsigned long *)(Get_CR3() + PAGE_OFFSET), *PDPDE = NULL, *PDE = NULL, Vir_Base, Offset, pos;

    Offset = Virtual_Addr & (PAGE_2M_SIZE - 1);
    Vir_Base = Virtual_Addr - Offset;
    
    //PML4E
    pos = (Vir_Base >> PAGE_PML4E_SHIFT) & 0x1FF;
    //PDPDE
    PDPDE = (unsigned long *)((PML4E[pos] & (~ 0xfffUL)) + PAGE_OFFSET);
    pos = (Vir_Base >> PAGE_PDPDE_SHIFT) & 0x1FF;
    //PDE
    PDE = (unsigned long *)((PDPDE[pos] & (~ 0xfffUL)) + PAGE_OFFSET);
    pos = (Vir_Base >> PAGE_2M_SHIFT) & 0x1FF;

    return (unsigned long)(PDE[pos] & PAGE_2M_MASK) + Offset;
}

void Display_PageTable(unsigned long *PML4E, unsigned long Virtual_Addr)
{
    unsigned long i, pos, start, end, *virtual = NULL, *PDPDE = NULL, *PDE = NULL;
    pos = (Virtual_Addr >> 39) & 0x1FF;
    color_printk(YELLOW, BLACK,"PML4E:%p,%p\t\t\n", &PML4E[pos],PML4E[pos]);

    PDPDE = (unsigned long *)(PML4E[pos] & (~ 0xfffUL));
    pos = (Virtual_Addr >> PAGE_PDPDE_SHIFT) & 0x1FF;
    color_printk(YELLOW, BLACK,"PDPDE:%p,%p\t\t\n", &PDPDE[pos],PDPDE[pos]);

    PDE = (unsigned long *)(PDPDE[pos] & (~ 0xfffUL));
    pos = (Virtual_Addr >> PAGE_2M_SHIFT) & 0x1FF;
    color_printk(YELLOW, BLACK,"PDE:%p,%p\t\t\n", &PDE[pos],PDE[pos]);
}