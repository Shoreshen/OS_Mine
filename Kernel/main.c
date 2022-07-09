//Output disasm: objdump -D system >./OSFiles/system.s
//Unmount USB: umount /media/shore/OrangeS0.02
//List Storage: sudo fdisk -l
//gcc call: left->right RDI RSI RDX RCX R8 R9
#include "./g_reg_only/INT.h"
#include "Printk.h"
#include "TSS.h"
#include "Mem.h"
#include "Task.h"
#include "cpu.h"
#include "keyboard.h"
#include "ACPI.h"
#include "PCI.h"
#include "SMP.h"
#include "Time.h"

int glb_ind = 0;

KERNEL_BOOT_PARAMETER_INFORMATION *kernel_boot_para_info = (KERNEL_BOOT_PARAMETER_INFORMATION *)(0x60000 + PAGE_OFFSET);

void Start_Kernel(void)
{
	int *addr = (int*)0xffff800003000000;
	int i, *SVGA_Base = (int *)(0x8200 + 40);
	task_union *EXMP_STACK = NULL;
	page *Page;
	void *tmp;
	slab *Slab = NULL;
	pg_attr ATTR;
	time Time;
	timer_reg Timer;
	lvt_timer LVT;
	task_struct *tsk;
	//Reset global vairables
	memset(&ACPI, 0, sizeof(ACPI));
	memset(&AHCI, 0, sizeof(AHCI));

	#pragma region Mapping SVGA
	PML4E = (unsigned long *)Get_CR3();
	memset(&ATTR, 0, sizeof(pg_attr));

	ATTR.PML4E_Attr.RW 	= 1;
	ATTR.PML4E_Attr.P	= 1;
	ATTR.PDPTE_Attr.P	= 1;
	ATTR.PDPTE_Attr.RW	= 1;
	ATTR.PDE_Attr.PS	= 1;
	ATTR.PDE_Attr.P		= 1;
	ATTR.PDE_Attr.RW	= 1;
	// Choose PAT[3]: uncacheable memory type
	ATTR.PDE_Attr.PCD	= 1;
	ATTR.PDE_Attr.PWT	= 1;
	pagetable_init(PML4E, kernel_boot_para_info->Graphics_Info.FB_addr, 0xffff800003000000, 3, &ATTR, 0, 1);
	#pragma endregion
	
	#pragma region Initialization of printk
	Pos.XResolution = kernel_boot_para_info->Graphics_Info.XResolution;
	Pos.YResolution = kernel_boot_para_info->Graphics_Info.YResolution;
	Pos.XPosition   = 0;
	Pos.YPosition   = 0;
	Pos.XCharSize   = 8;
	Pos.YCharSize   = 16;
	Pos.FB_addr     = (int*)0xffff800003000000;
	Pos.XPosMax     = Pos.XResolution/Pos.XCharSize - 1;
	Pos.YPosMax     = Pos.YResolution/Pos.YCharSize - 1;
	spin_init(&Pos.print_lock);
	#pragma endregion

	#pragma region Clear screan & Color band
	memset(Pos.FB_addr, 0, Pos.XResolution * Pos.YResolution * sizeof(int));
	for (i = 0; i < kernel_boot_para_info->Graphics_Info.XResolution * 20; i++) {
		*((char*)addr + 0) = (char)0x00;
		*((char*)addr + 1) = (char)0x00;
		*((char*)addr + 2) = (char)0xff;
		*((char*)addr + 3) = (char)0x00;
		addr++;
	}
	for (i = 0; i < kernel_boot_para_info->Graphics_Info.XResolution * 20; i++) {
		*((char*)addr + 0) = (char)0x00;
		*((char*)addr + 1) = (char)0xff;
		*((char*)addr + 2) = (char)0x00;
		*((char*)addr + 3) = (char)0x00;
		addr++;
	}
	for (i = 0; i < kernel_boot_para_info->Graphics_Info.XResolution * 20; i++) {
		*((char*)addr + 0) = (char)0xff;
		*((char*)addr + 1) = (char)0x00;
		*((char*)addr + 2) = (char)0x00;
		*((char*)addr + 3) = (char)0x00;
		addr++;
	}
	for (i = 0; i < kernel_boot_para_info->Graphics_Info.XResolution * 20; i++) {
		*((char*)addr + 0) = (char)0xff;
		*((char*)addr + 1) = (char)0xff;
		*((char*)addr + 2) = (char)0xff;
		*((char*)addr + 3) = (char)0x00;
		addr++;
	}
	#pragma endregion
	
	#pragma region CPU info
	init_cpu();
	#pragma endregion
	
	#pragma region Init memroy
	color_printk(RED,BLACK,"Memory init:\n");
	Init_Mem();
	#pragma endregion

	#pragma region Slab
	color_printk(RED,BLACK,"slab init \n");
	kernel_slab_init();
	#pragma endregion
	
	#pragma region Page table //Allocated all pages into page table
	PML4E = (unsigned long *)(Get_CR3() & (~ 0xfffUL) + PAGE_OFFSET);
	//Re-mapping VGE
	memset(&ATTR, 0, sizeof(pg_attr));

	ATTR.PML4E_Attr.RW 	= 1;
	ATTR.PML4E_Attr.P	= 1;

	ATTR.PDPTE_Attr.P	= 1;
	ATTR.PDPTE_Attr.RW	= 1;

	ATTR.PDE_Attr.PS	= 1;
	ATTR.PDE_Attr.P		= 1;
	ATTR.PDE_Attr.RW	= 1;
	ATTR.PDE_Attr.PCD	= 1;
	ATTR.PDE_Attr.PWT	= 1;

	pagetable_init(PML4E, kernel_boot_para_info->Graphics_Info.FB_addr, (GLB_Men_Desc.No_Page + 10) * PAGE_2M_SIZE + PAGE_OFFSET, 3, &ATTR, 0, 1);
	Pos.FB_addr = (int*)((GLB_Men_Desc.No_Page + 10) * PAGE_2M_SIZE + PAGE_OFFSET);
	//Mapping physical memory
	memset(&ATTR, 0, sizeof(pg_attr));

	ATTR.PML4E_Attr.RW 	= 1;
	ATTR.PML4E_Attr.P	= 1;

	ATTR.PDPTE_Attr.P	= 1;
	ATTR.PDPTE_Attr.RW	= 1;

	ATTR.PDE_Attr.PS	= 1;
	ATTR.PDE_Attr.P		= 1;
	ATTR.PDE_Attr.RW	= 1;
	//Fill kernel PML4E, upper 256 entry will be copied while using do_fork to create process
	for(i = 256; i < 512; i++){
		if(PML4E[i] == NULL){
			PML4E[i] = (unsigned long)kmalloc(PAGE_4K_SIZE,0) - PAGE_OFFSET;
			memset((void *)PML4E[i],0,PAGE_4K_SIZE);
			PML4E[i] |= *(unsigned long *)(&ATTR.PML4E_Attr);
		}
	}
	pagetable_init(PML4E, 0, PAGE_OFFSET, GLB_Men_Desc.No_Page, &ATTR, 0, 1);
	#pragma endregion
	
	#pragma region TSS
	EXMP_STACK = kmalloc(STACK_SIZE,0);
	EXMP_STACK->Task.cpu_id = 0;
	memset(TSS_TABLE,0,sizeof(TSS_TABLE));
	SET_TSS64(10,
		&TSS_TABLE[0],
		_stack_start, 
		_stack_start, 
		_stack_start, 
		((unsigned long)EXMP_STACK) + STACK_SIZE, 
		((unsigned long)EXMP_STACK) + STACK_SIZE, 
		((unsigned long)EXMP_STACK) + STACK_SIZE,
		((unsigned long)EXMP_STACK) + STACK_SIZE, 
		((unsigned long)EXMP_STACK) + STACK_SIZE,
		((unsigned long)EXMP_STACK) + STACK_SIZE,
		((unsigned long)EXMP_STACK) + STACK_SIZE
	);
	#pragma endregion

	#pragma region ACPI & PCI & PCIe
	Init_ACPI();
	Scan_PCI_Bus(0);
	#pragma endregion
	
	#pragma region Init_IDT
	color_printk(RED,BLACK,"IDT init:\n");
	Init_IDT();
	APIC_IOAPIC_init();
	#pragma endregion
	
	#pragma region Keyboard & mouse & Disk(AHCI)
	color_printk(RED,BLACK,"keyboard init \n");
	keyboard_init();
	color_printk(RED,BLACK,"mouse init \n");
	mouse_init();
	color_printk(RED,BLACK,"disk init \n");
	Disk_Init();
	AHCI.trg_port = 0; //0 = hda; 1 = hdb
	AHCI_queue.global_schedual = 0;
	DISK1_FAT32_FS_init();
	// unsigned char *a=kmalloc(512, 0);
	// int slot;
	// //Config
	// memset(a, 0, 512);
	// slot = AHCI_PostCMD(0,1,a,3);
	// while(AHCI.ABAR->ports[AHCI.trg_port].ci & 1<<slot);
	// for(i = 0; i < 512; i++){
	// 	color_printk(BLACK,YELLOW,"%02x",a[i]);
	// }
	// color_printk(BLACK,YELLOW,"\n");
	// color_printk(BLACK,YELLOW,"Total LBA: %d\n",((Disk_Identify_Info *)a)->Total_user_LBA_for_48_Address_Feature_set);
	// //Write
	// memset(a, 0x5b, 512);
	// slot = AHCI_PostCMD(0,1,a,1);
	// while(AHCI.ABAR->ports[AHCI.trg_port].ci & 1<<slot);
	// //Read
	// memset(a, 0, 512);
	// slot = AHCI_PostCMD(0,1,a,0);
	// while(AHCI.ABAR->ports[AHCI.trg_port].ci & 1<<slot);
	// for(i = 0; i < 512; i++){
	// 	color_printk(BLACK,YELLOW,"%02x",a[i]);
	// }
	// color_printk(BLACK,YELLOW,"\n");
	color_printk(RED,BLACK,"NVME init \n");
	NVME_Disk_Init();
	#pragma endregion

	#pragma region SMP
	SMP_init();
	#pragma endregion

	#pragma region Task & schedule
	current->cpu_id = 0;
	global_pid = 0;
	Init_schedule();
	Init_Task((task_union *)current, &init_mm, (unsigned long)current + STACK_SIZE, (unsigned long)current + STACK_SIZE);
	tsk = do_fork(CLONE_FS | CLONE_SIGNAL, "user.bin");
	#pragma endregion

	#pragma region Time
	memset(&Time, 0, sizeof(Time));
	get_cmos_time(&Time);
	color_printk(RED,BLACK,"year:%#04x,month:%#02x,day:%#02x,hour:%#02x,mintue:%#02x,second:%#02x\n",
		Time.cent*0x100+Time.year,Time.month,Time.day,Time.hour,Time.minute,Time.second);
	// if(ACPI.HPET_REG){
	// 	Init_HPET_Timer();
	// 	memset(&Timer, 0, sizeof(timer_reg));
	// 	Timer.Tn_INT_ENB_CNF = 1;
	// 	Timer.Tn_TYPE_CNF	 = 1;
	// 	Timer.Tn_VAL_SET_CNF = 1;
	// 	Timer.Timer			 = 14318179;
	// 	Set_HPET_Timer(0, &Timer);
	// }

	//APIC Timer Interrupt
	LVT.Vector = 0x20;
	LVT.Del_Stat = APIC_DEL_STATU_Idle;
	LVT.Mask = 0;
	LVT.T_Mode = 1; //1 = Periodic
	LVT.rsv0 = 0;
	LVT.rsv1 = 0;
	LVT.rsv2 = 0;
	AHCI_queue.global_schedual = 1; //Enable schedualing task of filing system
	Set_APIC_Timer(*((unsigned long *)&LVT), 1000000);
	#pragma endregion
	
	while(1)
	{
		// if(Key_Buffer.count){
		// 	analysis_keycode();
		// }
		if(Mouse_Buffer.count){
			analysis_mousecode();
		}
	}
}