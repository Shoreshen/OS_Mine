#include "SMP.h"
#include "TSS.h"
#include "./g_reg_only/INT.h"
#include "Task.h"
#include "Time.h"

void SMP_init()
{
	int i;
	unsigned int a,b,c,d;
    icr ICR;

	//get local APIC ID & print specs
	for(i = 0;;i++)
	{
		get_cpuid(0xb,i,&a,&b,&c,&d);
		if((c >> 8 & 0xff) == 0){
            break;
        }
		if(MACRO_PRINT){
			color_printk(WHITE,BLACK,"local APIC ID Package_../Core_2/SMT_1,type(%x) Width:%#08x,num of logical processor(%x)\n",
				c >> 8 & 0xff,a & 0x1f,b & 0xff);
		}
		SMP_CTL.Tot_Cores = b & 0xff;
	}

	color_printk(WHITE,BLACK,"x2APIC Total cores: %d\tx2APIC ID level:%#08x\tx2APIC ID the current logical processor:%#08x\n",
		SMP_CTL.Tot_Cores,c & 0xff,d);

    memcpy(_APU_boot_start, (unsigned char *)0xffff800000020000, (unsigned long)&_APU_boot_end - (unsigned long)&_APU_boot_start);

	//Set SMP interrupt
	for(i = 200; i < 210; i++){
		_Set_INT(IDT_PTR.Offset + i, ATTR_INTR_GATE, 2, interrupt_smp[i - 200]);
	}

	//Boot up APs
	memset(&ICR, 0, sizeof(ICR));
	ICR.Vector      = 0;
	ICR.Del_Mode    = ICR_DEL_MODE_INIT;
	ICR.Des_Mode    = ICR_DES_MODE_PHY;
	ICR.Del_Stat    = 0;
	ICR.Level       = ICR_LEVEL_OTHER;
	ICR.Trig_Mode   = ICR_TRIG_EDGE;
	ICR.SH          = ICR_SH_OTHERS;

	wrmsr(MSR_ICR, *((unsigned long*)&ICR));

	spin_init(&SMP_CTL.SMP_lock);
	for(SMP_CTL.global_i = 1; SMP_CTL.global_i < SMP_CTL.Tot_Cores; SMP_CTL.global_i++){
		spin_lock(&SMP_CTL.SMP_lock);
		memset(&ICR, 0, sizeof(ICR));
		SMP_CTL.SMP_TASK_PTR = kmalloc(STACK_SIZE, 0);
		SMP_CTL.EXMP_TASK_PTR = kmalloc(STACK_SIZE, 0);

		SMP_CTL.SMP_TASK_PTR->Task.cpu_id = SMP_CTL.global_i;
		SMP_CTL.EXMP_TASK_PTR->Task.cpu_id = SMP_CTL.global_i;
		
		SMP_CTL.SMP_TSS_TABLE_PTR = &TSS_TABLE[SMP_CTL.global_i];
		
		ICR.Vector      = 0x20; //AP start at: cs = 0x2000;rip = 0x01
		ICR.Del_Mode    = ICR_DEL_MODE_START;
		ICR.Des_Mode    = ICR_DES_MODE_PHY;
		ICR.Del_Stat    = 0;
		ICR.Level       = ICR_LEVEL_OTHER;
		ICR.Trig_Mode   = ICR_TRIG_EDGE;
		ICR.SH          = ICR_SH_NONE;
		ICR.dest.x2apic_dest = SMP_CTL.global_i;

		wrmsr(MSR_ICR, *((unsigned long*)&ICR));		//Intel's ISS protocol, INIT, START UP, START UP to make sure BP boots
		wrmsr(MSR_ICR, *((unsigned long*)&ICR));
	}
	//Send inter-processor interrupt
	spin_lock(&SMP_CTL.SMP_lock);
	ICR.Vector = 0xc8;
	ICR.dest.x2apic_dest = 1;
	ICR.Del_Mode = ICR_DEL_MODE_FIX;
	wrmsr(MSR_ICR, *((unsigned long*)&ICR));

	ICR.Vector = 0xc9;
	wrmsr(MSR_ICR, *((unsigned long*)&ICR));
	spin_unlock(&SMP_CTL.SMP_lock);
}

void Start_SMP()
{
    unsigned int x,y;
    unsigned long tmp;
	
    __asm__ __volatile__ (
			"movq %%rax, %%rsp	\n\t"
			:
			:"a"((unsigned long)SMP_CTL.SMP_TASK_PTR + STACK_SIZE)
			:"memory"	
    );

    //Enabling xAPIC(IA32_APIC_BASE[10]) and 2xAPIC(IA32_APIC_BASE[11])
	tmp = rdmsr(0x1b);
	tmp |= ((1UL << 10) | (1UL << 11));
	wrmsr(0x1b,tmp);

	//Enabling LAPIC(SVR[8])
	tmp = rdmsr(0x80f);
	tmp |= (1UL << 8); //No support for EOI broadcast, no need to set bit SVR[12]
	tmp |= (1UL << 12);
	wrmsr(0x80f,tmp);

	//Mask all LVT
	tmp = 0x10000;
	//wrmsr(0x82F, tmp); Virtual machine do not support
	wrmsr(0x832, tmp);
	wrmsr(0x833, tmp);
	wrmsr(0x834, tmp);
	wrmsr(0x835, tmp);
	wrmsr(0x836, tmp);
	wrmsr(0x837, tmp);

	//Set TSS
	SET_TSS64(10 + SMP_CTL.global_i * 2,
		SMP_CTL.SMP_TSS_TABLE_PTR,
		(unsigned long)SMP_CTL.SMP_TASK_PTR + STACK_SIZE, 
		(unsigned long)SMP_CTL.SMP_TASK_PTR + STACK_SIZE, 
		(unsigned long)SMP_CTL.SMP_TASK_PTR + STACK_SIZE, 
		(unsigned long)SMP_CTL.EXMP_TASK_PTR + STACK_SIZE, 
		(unsigned long)SMP_CTL.EXMP_TASK_PTR + STACK_SIZE, 
		(unsigned long)SMP_CTL.EXMP_TASK_PTR + STACK_SIZE, 
		(unsigned long)SMP_CTL.EXMP_TASK_PTR + STACK_SIZE, 
		(unsigned long)SMP_CTL.EXMP_TASK_PTR + STACK_SIZE, 
		(unsigned long)SMP_CTL.EXMP_TASK_PTR + STACK_SIZE, 
		(unsigned long)SMP_CTL.EXMP_TASK_PTR + STACK_SIZE
	);
	color_printk(RED,YELLOW,"x2APIC ID:%p\n",rdmsr(0x802));
	spin_unlock(&SMP_CTL.SMP_lock);
	sti;
	// if(3 == current->cpu_id){
	// 	task_union *tsk_un;
	// 	task_struct *tsk;
	// 	pg_attr ATTR;
	// 	lvt_timer LVT;

	// 	Init_Task((task_union *)current, &init_mm, (unsigned long)current + STACK_SIZE, (unsigned long)current + STACK_SIZE);
		
	// 	memset(&ATTR, 0, sizeof(ATTR));
	// 	ATTR.PML4E_Attr.RW 	= 1;
	// 	ATTR.PML4E_Attr.P	= 1;
	// 	ATTR.PML4E_Attr.US	= 1;
	// 	ATTR.PDPTE_Attr.P	= 1;
	// 	ATTR.PDPTE_Attr.RW	= 1;
	// 	ATTR.PDPTE_Attr.US	= 1;
	// 	ATTR.PDE_Attr.PS	= 1;
	// 	ATTR.PDE_Attr.P		= 1;
	// 	ATTR.PDE_Attr.RW	= 1;
	// 	ATTR.PDE_Attr.US	= 1;
		
	// 	tsk_un = kmalloc(sizeof(task_struct),0);
	// 	tsk = &tsk_un->Task;
	// 	memset(tsk_un, 0, sizeof(task_struct));
	// 	memcpy(&init_task_union.Task, tsk, sizeof(task_struct));
	// 	tsk->state		= TASK_RUNNING;
	// 	tsk->flag 		= 0;
	// 	tsk->priority 	= 2;
	// 	tsk->pid++;
	// 	tsk->thread.rip	 = 0x800000;
	// 	kThread((unsigned long)user_init, 0x1000, &ATTR, tsk);
	// 	kfree(tsk);

	// 	LVT.Vector = 0x20;
	// 	LVT.Del_Stat = APIC_DEL_STATU_Idle;
	// 	LVT.Mask = 0;
	// 	LVT.T_Mode = 1; //1 = Periodic
	// 	LVT.rsv0 = 0;
	// 	LVT.rsv1 = 0;
	// 	LVT.rsv2 = 0;
	// 	Set_APIC_Timer(*((unsigned long *)&LVT), 100000000);
	// }
	// if(current->cpu_id>1){
	// 	x = 1/0;
	// }
	while(1){
		hlt;
	}
}
