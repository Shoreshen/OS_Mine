#include "Time.h"
#include "Mem.h"
#include "Task.h"


#pragma region CMOS
void get_cmos_time(time *Time)
{
    int idx[7] = {0x32,0x09,0x08,0x07,0x04,0x02,0x00}, i, *t=(int*)Time;
    cli;
    do{
        for(i = 0; i<7; i++){
            io_out8(0x70, 0x80 | idx[i]);
            t[i]=io_in8(0x71);
        }

        io_out8(0x70, 0x80 | 0x00);
    }while(Time->second != io_in8(0x71));
    sti;
}
#pragma endregion

#pragma region HPET
void Init_HPET_Timer(void)
{
    pg_attr ATTR;
    gcap_id *GCAP_ID;
    rte RTE;
    int i;
    unsigned long *temp;

    //Allcate page
	memset(&ATTR, 0, sizeof(pg_attr));
	ATTR.PML4E_Attr.RW 	= 1;
	ATTR.PML4E_Attr.P	= 1;
	ATTR.PDPTE_Attr.P	= 1;
	ATTR.PDPTE_Attr.RW	= 1;
	ATTR.PDE_Attr.PS	= 1;
	ATTR.PDE_Attr.P		= 1;
	ATTR.PDE_Attr.RW	= 1;
    // PAT[3]: Uncacheable memory type
    ATTR.PDE_Attr.PCD   = 1;
    ATTR.PDE_Attr.PWT   = 1;
    pagetable_init(PML4E,
        (unsigned long)ACPI.HPET_REG & PAGE_2M_MASK - PAGE_OFFSET,
        (unsigned long)ACPI.HPET_REG & PAGE_2M_MASK,
        1, &ATTR, 0, 1);
    
    //Disable timer
    *((unsigned long *)&ACPI.HPET_REG->GEN_CONF) = 3;
    // io_mfence;
    for(i=0; i<=8; i++){
        *((unsigned long *)&ACPI.HPET_REG->TIMER_REG[i]) = *((unsigned long *)&ACPI.HPET_REG->TIMER_REG[i]) & 0xFFFFFFFF00000002;
        // io_mfence;
    }

    //Register I/O APIC
    RTE.vector                          = 0x22;
    RTE.deliver_mode                    = APIC_DEL_MODE_Fixed;
    RTE.dest_mode                       = APIC_DEST_MODE_PHYSICAL;
    RTE.deliver_status                  = APIC_DEL_STATU_Idle;
    RTE.polarity                        = APIC_POLARITY_HIGH;
    RTE.irr                             = APIC_IRR_RESET;
    RTE.trigger                         = APIC_TIG_MODE_Edge;
    RTE.mask                            = APIC_MASK_UNSET;
    RTE.reserved                        = 0;

    RTE.destination.physical.phy_dest   = 0;
    RTE.destination.physical.reserved1  = 0;
    RTE.destination.physical.reserved2  = 0;

    register_irq(0x22, &RTE , &HPET_handler, (unsigned long)0, &hpet_int_controller, "HPET");
}

void Set_HPET_Timer(int Tn, timer_reg *Timer)
{
    timer_list *tmp = NULL;
    jiffies = 0;
    Init_Timer_List(&Timer_List_Header, NULL, NULL, -1UL);
    register_softirq(0, do_timer, NULL);

    tmp = kmalloc(sizeof(timer_list), 0);
    Init_Timer_List(tmp, test_timer, NULL, 5);
    Add_Timer_List(tmp);

    memcpy(Timer, &ACPI.HPET_REG->TIMER_REG[Tn],sizeof(timer_reg));
}

void HPET_handler(unsigned long nr, unsigned long parameter)
{
	jiffies++;
    set_softirq_status(TIMER_SIRQ);
	switch(current->priority)
	{
		case 0:
		case 1:
			task_schedule[current->cpu_id].CPU_exec_task_jiffies--;
			current->vrun_time += 1;
			break;
		case 2:
		default:
			task_schedule[current->cpu_id].CPU_exec_task_jiffies -= 2;
			current->vrun_time += 2;
			break;
	}

	if(task_schedule[current->cpu_id].CPU_exec_task_jiffies <= 0){
        current->flag |= NEED_SCHEDULE;
    }
}
#pragma endregion

#pragma region APIC_TIMER
void Set_APIC_Timer(unsigned long LVT, unsigned int init)
{
    timer_list *tmp = NULL;
    jiffies = 0;
    interrupt_desc[0].controller = &apic_timer_controller;
    interrupt_desc[0].irq_name = "APIC_TIMER";
    interrupt_desc[0].flags = 0;
    interrupt_desc[0].handler = APIC_Timer_handler;

    register_softirq(0, do_timer, NULL);

    tmp = kmalloc(sizeof(timer_list), 0);
    Init_Timer_List(&Timer_List_Header, NULL, NULL, -1UL);
    Init_Timer_List(tmp, test_timer, NULL, 5);
    Add_Timer_List(tmp);
    
    wrmsr(0x832, LVT);
    wrmsr(0x83E, 0);
    wrmsr(0x838, init & 0xFFFFFFFF);
}
void APIC_Timer_handler(unsigned long nr, unsigned long parameter)
{
	jiffies++;
    set_softirq_status(TIMER_SIRQ);
	switch(current->priority)
	{
		case 0:
		case 1:
			task_schedule[current->cpu_id].CPU_exec_task_jiffies--;
			current->vrun_time += 1;
			break;
		case 2:
		default:
			task_schedule[current->cpu_id].CPU_exec_task_jiffies -= 2;
			current->vrun_time += 2;
			break;
	}
    // color_printk(RED,BLACK,"t:%d",task_schedule[current->cpu_id].CPU_exec_task_jiffies);
	if(task_schedule[current->cpu_id].CPU_exec_task_jiffies <= 0){
        current->flag |= NEED_SCHEDULE;
    }
}
#pragma endregion

#pragma region SoftIRQ_Timer
void do_timer(void *data)
{
    timer_list *tmp = container_of(Timer_List_Header.List.next, timer_list, List);
    while(!List_is_empty(&Timer_List_Header.List) && (tmp->exp_jiffies <= jiffies)){
        del_timer(tmp);
        tmp->func(tmp->data);
        tmp = container_of(Timer_List_Header.List.next, timer_list, List);
    }

    // color_printk(RED, WHITE, "(APIC:%ld)", jiffies);
}
void Init_Timer_List(timer_list *timer, void (*func)(void *data), void *data, unsigned long exp_jiffies)
{
    List_Init(&timer->List);
    timer->func = func;
    timer->data = data;
    timer->exp_jiffies = jiffies + exp_jiffies;
}

void Add_Timer_List(timer_list *timer)
{
    timer_list *tmp = container_of(Timer_List_Header.List.next, timer_list, List);
    
    if(List_is_empty(&Timer_List_Header.List)){

    }else{
        while(tmp->exp_jiffies < timer->exp_jiffies){
            tmp = container_of(Timer_List_Header.List.next, timer_list, List);
        }
    }

    List_Insert_After(&tmp->List, &timer->List);
}

void del_timer(timer_list *timer)
{
    List_Del(&timer->List);
}

void test_timer()
{
    color_printk(BLUE,WHITE,"test_timer");
}
#pragma endregion