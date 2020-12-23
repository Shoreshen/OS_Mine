#include "./g_reg_only/INT.h"
#include "Mem.h"

void Local_APIC_init(void)
{
	unsigned int a,b,c,d;
	unsigned long tmp;

	//Detecting APIC, xAPIC, 2xAPIC
	get_cpuid(1,0,&a,&b,&c,&d);
	if(MACRO_PRINT){
		color_printk(WHITE,BLACK,"CPUID\t01,eax:%#08X,ebx:%#08X,ecx:%#08X,edx:%#08X\n",a,b,c,d);
		if((1<<9) & d){
			color_printk(WHITE,BLACK,"HW support APIC&xAPIC\t");
		}
		else{
			color_printk(WHITE,BLACK,"HW NO support APIC&xAPIC\t");
		}
		
		if((1<<21) & c){
			color_printk(WHITE,BLACK,"HW support x2APIC\n");
		}
		else{
			color_printk(WHITE,BLACK,"HW NO support x2APIC\n");
		}
	}
	
	//Enabling xAPIC(IA32_APIC_BASE[10]) and 2xAPIC(IA32_APIC_BASE[11])
	tmp = rdmsr(0x1b);
	tmp |= ((1UL << 10) | (1UL << 11));
	wrmsr(0x1b,tmp);
	tmp = 0;
	tmp = rdmsr(0x1b);
	if(MACRO_PRINT){
		color_printk(WHITE,BLACK,"IA32_APIC_BASE: %#p\n",tmp);
		if(tmp & 0xc00){
			color_printk(WHITE,BLACK,"xAPIC & x2APIC enabled\n");
		}
	}

	//Enabling LAPIC(SVR[8])
	tmp = rdmsr(0x80f);
	tmp |= (1UL << 8); //No support for EOI broadcast, no need to set bit SVR[12]
	tmp |= (1UL << 12);
	wrmsr(0x80f,tmp);
	tmp = 0;
	tmp = rdmsr(0x80f);
	if(MACRO_PRINT){
		color_printk(WHITE,BLACK,"SVR: %#p\n",tmp);
		if(tmp & 0x100){
			color_printk(WHITE,BLACK,"SVR[8] enabled\n");
		}
		if(tmp & 0x1000){
			color_printk(WHITE,BLACK,"SVR[12] enabled\n");
		}
	}

	//Get LAPIC ID, Max LVT entry, detect LAPIC mode 
	tmp = rdmsr(0x802);
	if(MACRO_PRINT){
		color_printk(WHITE,BLACK,"local_APIC_ID: %#p\n",tmp);
	}

	tmp = rdmsr(0x803);
	if(MACRO_PRINT){
		color_printk(WHITE,BLACK,"local APIC Version:%#p,Max LVT Entry:%p,SVR(Suppress EOI Broadcast):%p\t",tmp & 0xff,(tmp >> 16 & 0xff) + 1,tmp >> 24 & 0x1);
		if((tmp & 0xff) < 0x10){
			color_printk(WHITE,BLACK,"82489DX discrete APIC\n");
		}
		else if(((tmp & 0xff) >= 0x10) && ((tmp & 0xff) <= 0x15)){
			color_printk(WHITE,BLACK,"Integrated APIC\n");
		}
	}

	//Mask all LVT
	tmp = 0x10000;
	//wrmsr(0x82F, tmp); Virtual machine do not support
	wrmsr(0x832, tmp);
	wrmsr(0x833, tmp);
	wrmsr(0x834, tmp);
	wrmsr(0x835, tmp);
	wrmsr(0x836, tmp);
	wrmsr(0x837, tmp);
	if(MACRO_PRINT){
		color_printk(GREEN,BLACK,"Mask ALL LVT\n");
	}
	//Read TPR, PPR
	tmp = rdmsr(0x808);
	if(MACRO_PRINT){
		color_printk(GREEN,BLACK,"Set LVT TPR:%p\t",tmp);
	}
	tmp = rdmsr(0x80a);
	if(MACRO_PRINT){
		color_printk(GREEN,BLACK,"Set LVT PPR:%p\n",tmp);
	}
}

void IOAPIC_init(void)
{
	unsigned long i;
	pg_attr ATTR;
	//Mapping physical address OxFEC00000 to virtual address
	memset(&ATTR, 0, sizeof(pg_attr));
	ATTR.PML4E_Attr.RW 	= 1;
	ATTR.PML4E_Attr.P	= 1;

	ATTR.PDPTE_Attr.P	= 1;
	ATTR.PDPTE_Attr.RW	= 1;

	ATTR.PDE_Attr.PS	= 1;
	ATTR.PDE_Attr.P		= 1;
	ATTR.PDE_Attr.RW	= 1;
	// Choose PAT[3]: uncacheable memory type
	ATTR.PDE_Attr.PWT	= 1;
	ATTR.PDE_Attr.PCD	= 1;
	if(MACRO_PRINT){
		color_printk(BLUE,BLACK,"Start create page\n");
	}
	pagetable_init((unsigned long *)(Get_CR3() & (~ 0xfffUL) + PAGE_OFFSET), *ACPI.IOAPIC_BASE, *ACPI.IOAPIC_BASE + PAGE_OFFSET, 1, &ATTR, 0, 1);
	if(MACRO_PRINT){
		color_printk(BLUE,BLACK,"Successfully create page\n");
	}
	//Setting up IOAPIC_MAP
	IOAPIC_MAP.physical_address = *ACPI.IOAPIC_BASE;
	IOAPIC_MAP.virtual_index_address = (unsigned char*)(*ACPI.IOAPIC_BASE + PAGE_OFFSET);
	IOAPIC_MAP.virtual_data_address = (unsigned int*)(*ACPI.IOAPIC_BASE + PAGE_OFFSET + 0x10);
	IOAPIC_MAP.virtual_EOI_address = (unsigned int*)(*ACPI.IOAPIC_BASE + PAGE_OFFSET + 0x40);
	if(MACRO_PRINT){
		color_printk(BLUE,BLACK,"IOAPIC_MAP.physical_address: %p\n",IOAPIC_MAP.physical_address);
		color_printk(BLUE,BLACK,"IOAPIC_MAP.virtual_address: %p\n",(unsigned long)IOAPIC_MAP.virtual_index_address);
	}

	*IOAPIC_MAP.virtual_index_address = 0x00;
	// io_mfence;
	*IOAPIC_MAP.virtual_data_address = 0x0f000000;
	// io_mfence;
	if(MACRO_PRINT){
		color_printk(GREEN,BLACK,"Get IOAPIC ID REG:%#08x,ID:%#08x\n",*IOAPIC_MAP.virtual_data_address, *IOAPIC_MAP.virtual_data_address >> 24 & 0xf);
	}
	// io_mfence;
	*IOAPIC_MAP.virtual_index_address = 0x01;
	// io_mfence;
	if(MACRO_PRINT){
		color_printk(GREEN,BLACK,"Get IOAPIC Version REG:%#08x,MAX redirection enties:%#08d\n",
			*IOAPIC_MAP.virtual_data_address ,
			((*IOAPIC_MAP.virtual_data_address >> 16) & 0xff) + 1
		);
	}
	for(i = 0x10;i < 0x40;i += 2){
		ioapic_rte_write(i, 0x10020 + ((i - 0x10) >> 1));
	}

	//ioapic_rte_write(0x12,0x21);
	if(MACRO_PRINT){
		color_printk(GREEN,BLACK,"I/O APIC Redirection Table Entries Set Finished.\n");	
	}
}

void APIC_IOAPIC_init(void)
{
    int i;

	for(i = 32;i < 56;i++){
		_Set_INT(IDT_PTR.Offset + i, ATTR_INTR_GATE, 0, interrupt[i - 32]);
	}
	if(MACRO_PRINT){
		color_printk(GREEN,BLACK,"MASK 8259A\n");
	}

    //8259A-master	ICW1-4
	io_out8(0x20,0x11);
	io_out8(0x21,0x20);
	io_out8(0x21,0x04);
	io_out8(0x21,0x01);

	//8259A-slave	ICW1-4
	io_out8(0xa0,0x11);
	io_out8(0xa1,0x28);
	io_out8(0xa1,0x02);
	io_out8(0xa1,0x01);

	//8259A-M/S	OCW1
	io_out8(0x21,0xff);
	io_out8(0xa1,0xff);
	//enable IMCR
	io_out8(0x22,0x70);
	io_out8(0x23,0x01);
	
	Local_APIC_init();
	IOAPIC_init();

    sti;

    return;
}