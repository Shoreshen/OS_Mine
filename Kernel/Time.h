#include "lib.h"

#define CMOS_READ(addr) ({io_out8(0x70,0x80 | addr);io_in8(0x71);})

hw_int_controller hpet_int_controller = 
{
	.enable     = IOAPIC_enable,
	.disable    = IOAPIC_disable,
	.install    = IOAPIC_install,
	.uninstall  = IOAPIC_uninstall,
	.ack        = IOAPIC_edge_ack,
	.do_soft    = do_softirq,
};

hw_int_controller apic_timer_controller = 
{
	.enable     = IOAPIC_enable,
	.disable    = IOAPIC_disable,
	.install    = IOAPIC_install,
	.uninstall  = IOAPIC_uninstall,
	.ack        = IOAPIC_edge_ack,
	.do_soft    = do_softirq,
};

typedef struct{
	unsigned long Vector:8;
	unsigned long rsv0:4;
	unsigned long Del_Stat:1;
	unsigned long rsv1:3;
	unsigned long Mask:1;
	unsigned long T_Mode:2;
	unsigned long rsv2:45;
} lvt_timer;

void get_cmos_time(time *Time);
void Init_HPET_Timer(void);
void HPET_handler(unsigned long nr, unsigned long parameter);
void Set_HPET_Timer(int Tn, timer_reg *Timer);
void do_timer(void *data);
void Init_Timer_List(timer_list *timer, void (*func)(void *data), void *data, unsigned long exp_jiffies);
void Add_Timer_List(timer_list *timer);
void del_timer(timer_list *timer);
void test_timer();
void Set_APIC_Timer(unsigned long LVT, unsigned int init);
void APIC_Timer_handler(unsigned long nr, unsigned long parameter);