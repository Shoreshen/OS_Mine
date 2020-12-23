#pragma once

#include "sys_Struct.h"
#include "Printk.h"

#pragma region List functions
	#define container_of(ptr,type,member)\
		({\
			typeof(((type *)0)->member) *p;\
			p = ptr;\
			(type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));\
		})
	//typeof(arg) p/*p retrive the type of a variable, and create p of the same variable or pointor of the same variable
	//(type *) since it's define, this is force transfer to pointer type of "type"
	static inline void List_Init(list *List)
	{
		List->prev = List;
		List->next = List;
	}
	static inline void List_Insert_After(list *entry, list *newer)
	{
		newer->next = entry->next;
		newer->prev = entry;
		newer->next->prev = newer;
		entry->next = newer;
	}
	static inline void List_Insert_Before(list *entry, list *newer)
	{
		newer->next = entry;
		entry->prev->next = newer;
		newer->prev = entry->prev;
		entry->prev = newer;
	}
	static inline void List_Del(list *entry)
	{
		entry->prev->next = entry->next;
		entry->next->prev = entry->prev;
	}
	static inline long List_is_empty(list *entry)
	{
		if(entry == entry->next && entry->prev == entry){
			return 1;
		}else {
			return 0;
		}
	}
#pragma endregion

#pragma region Memory functions
	/**
	 @param 	From 	Pointer to address to copy from
	 @param 	To		Pointer to address to copy to
	 @param 	Num		Number of byte need to be copied

	 @return 	To param
	**/
	static inline void* memcpy(void *From, void *To, long Num)
	{
		__asm__ __volatile__	(					//__volatile__: keep instruction as written, do not optimize while compiling.
			"cld	\n\t"							//Clear direction: direction = upward
			"rep	movsq\n\t"						//Repeat move string quad(8 byte)
			"testb	$4,%b1	\n\t"					//test byte: 100b and lower 2 byte of 2end Cons(constraint), only effect EFLAG reg
			"je	1f	\n\t"							//Jump equal to symbal 1:
			"movsl	\n\t"							//Move string long(4 byte)
			"1:\t testb	$2,%b1	\n\t"				//Same as above
			"je	2f	\n\t"
			"movsw	\n\t"
			"2:\t testb	$1,%b1	\n\t"
			"je	3f	\n\t"
			"movsb	\n\t"
			"3:	\n\t"
			:										//Output: N/A
			:"c"(Num/8),"r"(Num),"D"(To),"S"(From)	//Input:
														//(Num/8) => 0th Cons: "c"(RCX)
														//(Num)   => 1st Cons: "r"(gcc pick register)
														//(To)    => 2nd Cons: "D"(RDI)
														//(From)  => 3Rd Cons: "S"(RSI)
			:										//clobbered:N/A
		);
		return To;
	}
	static inline void* memset(void *Address, unsigned char C, long Count)
	{
		int d0,d1;
		unsigned long tmp = C * 0x0101010101010101UL;
		__asm__	__volatile__	(	
			"cld	\n\t"
			"rep	\n\t"
			"stosq	\n\t"
			"testb	$4, %b3	\n\t"
			"je	1f	\n\t"
			"stosl	\n\t"
			"1:\ttestb	$2, %b3	\n\t"
			"je	2f\n\t"
			"stosw	\n\t"
			"2:\ttestb	$1, %b3	\n\t"
			"je	3f	\n\t"
			"stosb	\n\t"
			"3:	\n\t"
			:
			:"a"(tmp),"b"(Count),"c"(Count/8),"D"(Address)	
			:"memory"					
		);
		return Address;
	}
	static inline int memcmp(void *FirstPart, void *SecondPart, long Count)
	{
		register int __res;

		__asm__	__volatile__	(	
			"cld	\n\t"		//clean direct
			"repe	cmpsb\n\t"		//repeat if equal
			"je	1f	\n\t"
			"movl	$1,	%%eax	\n\t"
			"jl	1f	\n\t"
			"negl	%%eax	\n\t"
			"1:	\n\t"
			:"=a"(__res)
			:"0"(0),"D"(FirstPart),"S"(SecondPart),"c"(Count)
			:
		);
		return __res;
	}

	void* kmalloc(unsigned long size,unsigned long gfp_flages);
	unsigned long kfree(void * address);
#pragma endregion

#pragma region String functions
	static inline int strlen(char * String)
	{
		register int __res;
		__asm__	__volatile__ (	                    //"__asm__()"" same as "asm()", just another name to use
													//__volatile__: Complier will not optimize instructions
			"cld	\n\t"                           //Clear direction: direction = upward
			"repne	\n\t"                           //Repeat not equal: Repeat while the ECX register not zero and the ZF flag is clear
													//					Each time ECX - 1
			"scasb  \n\t"                           //Scan string byte: Compare byte of RDI and AL, set EFLAG(here mainly ZF)
			"notl	%0	\n\t"                       //Logical instruction 0->1, 1->0, operating on 0th Cons(constraint)
			"decl	%0	\n\t"                       //minuse 1, operating on register %0
			:"=c"(__res)                            //Output: 
														//0th Cons: "c"(RCX) => __res
			:"D"(String),"a"(0),"0"(0xffffffff)     //Input:
														//(String)     => 1st Cons: "D"(RDI)
														//(0)          => 2nd Cons: "a"(RAX)
														//(0xffffffff) => 0th Output("=c")
			:                                       //clobbered: N/A
		);
		return __res;
	}
	static inline int strcmp(char * FirstPart,char * SecondPart)
	{
		register int __res;
		__asm__	__volatile__	(	
			"cld	\n\t"
			"1:	\n\t"
			"lodsb	\n\t"
			"scasb	\n\t"
			"jne	2f	\n\t"
			"testb	%%al,	%%al	\n\t"
			"jne	1b	\n\t"
			"xorl	%%eax,	%%eax	\n\t"
			"jmp	3f	\n\t"
			"2:	\n\t"
			"movl	$1,	%%eax	\n\t"
			"jl	3f	\n\t"
			"negl	%%eax	\n\t"
			"3:	\n\t"
			:"=a"(__res)
			:"D"(FirstPart),"S"(SecondPart)
			:					
		);
		return __res;
	}
	static inline char * strncpy(char * Dest,char * Src,long Count)
	{
		__asm__	__volatile__	(	
			"cld	\n\t"
			"1:	\n\t"
			"decq	%2	\n\t"
			"js	2f	\n\t"
			"lodsb	\n\t"
			"stosb	\n\t"
			"testb	%%al,	%%al	\n\t"
			"jne	1b	\n\t"
			"rep	\n\t"
			"stosb	\n\t"
			"2:	\n\t"
			:
			:"S"(Src),"D"(Dest),"c"(Count)
			:"ax","memory"				
		);
		return Dest;
	}
	static inline char * strcpy(char * Dest,char * Src)
	{
		__asm__	__volatile__	(	
			"cld	\n\t"
			"1:	\n\t"
			"lodsb	\n\t"
			"stosb	\n\t"
			"testb	%%al,	%%al	\n\t"
			"jne	1b	\n\t"
			:
			:"S"(Src),"D"(Dest)
			:"ax","memory"
		);
		return 	Dest;
	}
	static inline int stricmp(char *string1, char *string2)
	{
		char c1;
		char c2;

		do {
			c1 = *string1;
			if(c1>='a' && c1<='z'){
				c1 -= 32;
			}
			c2 = *string2;
			if(c2>='a' && c2<='z'){
				c2 -= 32;
			}
			string1++;
			string2++;
		}
		while ((c1 == c2) && (c1));

		return (c1 - c2);
	}
#pragma endregion

#pragma region System functions
	static inline TSS_Struct * get_TSS(void)
	{
        unsigned long n;
		__asm__ __volatile__(
            "str %0"
            :"=a"(n)
            :
            :"memory"
        );
		n = n + (unsigned long)GDT_PTR.Offset;
        n = ((unsigned long)((GDT_TSS_Desc *)n)->Base2) << 32 | ((unsigned long)((GDT_TSS_Desc *)n)->Base1) << 24 | (unsigned long)((GDT_TSS_Desc *)n)->Base0;
		
		return (TSS_Struct *)n;
	}
	void PrintTSS(unsigned int FRcolor, unsigned int BKcolor);
	static inline unsigned long readrflag()
	{
		unsigned long rflag;
		__asm__ __volatile__(	
			"pushfq	\n\t"
			"popq	%0\n\t"
			:"=a"(rflag)
			:
			:"memory"
		);
		return rflag;
	}
	static inline void io_out8(unsigned short port,unsigned char value)
	{
		__asm__ __volatile__(	
			"outb	%0,	%%dx\n\t"
			"mfence	\n\t"				//Block the following memory operating instruction until the previous instruction finished
										//This is due to out of order construction.
			:
			:"a"(value),"d"(port)
			:"memory"
		);
	}
	static inline unsigned char io_in8(unsigned short port)
	{
		unsigned char ret = 0;
		__asm__ __volatile__(	
			"inb	%%dx,	%0	\n\t"
			"mfence			\n\t"
			:"=a"(ret)
			:"d"(port)
			:"memory"
		);
		return ret;
	}
	static inline unsigned int io_in32(unsigned short port)
	{
		unsigned int ret = 0;
		__asm__ __volatile__(	
			"inl	%%dx,	%0	\n\t"
			"mfence			\n\t"
			:"=a"(ret)
			:"d"(port)
			:"memory"
		);
		return ret;
	}
	static inline void io_out32(unsigned short port,unsigned int value)
	{
		__asm__ __volatile__(	
			"outl	%0,	%%dx	\n\t"
			"mfence			\n\t"
			:
			:"a"(value),"d"(port)
			:"memory"
		);
	}
	static inline unsigned long rdmsr(unsigned long address)
	{
		unsigned int tmp0 = 0;
		unsigned int tmp1 = 0;
		__asm__ __volatile__(
			"rdmsr	\n\t"
			:"=d"(tmp0),"=a"(tmp1)
			:"c"(address)
			:"memory"
		);	
		return (unsigned long)tmp0<<32 | tmp1;
	}
	static inline void wrmsr(unsigned long address,unsigned long value)
	{
		__asm__ __volatile__(
			"wrmsr	\n\t"
			:
			:"d"(value >> 32),			
			 "a"(value & 0xffffffff),	//EDX:EAX = Value to be written into MSR
			 "c"(address)				//ECX = Address of MSR registers
			:"memory"
		);
	}
	static inline long get_cr0(void)
	{
		long a;
		__asm__ __volatile__	(	
			"movq %%cr0, %0"
			:"=a"(a)
		);
		return a;
	}
	#define get_rsp(rsp)\
		__asm__ __volatile__	(\
			"movq %%rsp, %0"\
			:"=a"(rsp)\
		);
	#define get_rbp(rbp)\
		__asm__ __volatile__	(\
			"movq %%rbp, %0"\
			:"=a"(rbp)\
		);
	static inline long get_ss(void)
	{
		long a;
		__asm__ __volatile__	(	
			"movq %%ss, %0"
			:"=a"(a)
		);
		return a;
	}
	static inline void get_cpuid(unsigned int Mop,unsigned int Sop,unsigned int *a,unsigned int *b,unsigned int *c,unsigned int *d)
	{
		__asm__ __volatile__	(	
			"cpuid	\n\t"
			:"=a"(*a),"=b"(*b),"=c"(*c),"=d"(*d)    //cpuid output: rax, rbx, rcx, rdx
			:"0"(Mop),"2"(Sop)                      //cpuid input:  rax, rdx 
		);
	}

	#define port_insw(port,buffer,nr) __asm__ __volatile__(\
		"cld	\n\t"\
		"rep	\n\t"\
		"insw	\n\t"\
		"mfence	\n\t"\
		:\
		:"d"(port),"D"(buffer),"c"(nr)\
		:"memory"\
	) //in string word (2 byte)

	#define port_outsw(port,buffer,nr) __asm__ __volatile__(\
		"cld	\n\t"\
		"rep	\n\t"\
		"outsw	\n\t"\
		"mfence	\n\t"\
		:\
		:"d"(port),"S"(buffer),"c"(nr)\
		:"memory"\
	) //out string word (2 byte)
	static inline unsigned long get_rflags()
	{
		unsigned long tmp = 0;
		__asm__ __volatile__ (
			"pushfq	\n\t"
			"movq	(%%rsp), %0	\n\t"
			"popfq	\n\t"
			:"=r"(tmp)
			:
			:"memory"
		);
		return tmp;
	}
#pragma endregion

#pragma region APIC
	static inline int Get_APIC_ID(void)
	{
		int x,y;
		__asm__ __volatile__(	
			"movq $0x802,	%%rcx	\n\t"
			"rdmsr	\n\t"
			:"=a"(x),"=d"(y)
			:
			:"memory"
    	);
		return x;
	}
	static inline unsigned long ioapic_rte_read(unsigned char index)
	{
		unsigned long ret;

		*IOAPIC_MAP.virtual_index_address = index + 1;
		// io_mfence;
		ret = *IOAPIC_MAP.virtual_data_address;
		ret <<= 32;
		// io_mfence;

		*IOAPIC_MAP.virtual_index_address = index;		
		// io_mfence;
		ret |= *IOAPIC_MAP.virtual_data_address;
		// io_mfence;

		return ret;
	}

	static inline void ioapic_rte_write(unsigned char index,unsigned long value)
	{
		//RTE[0] start with index = 0x10;
		*IOAPIC_MAP.virtual_index_address = index;
		// io_mfence;
		*IOAPIC_MAP.virtual_data_address = value & 0xffffffff;
		value >>= 32;
		// io_mfence;
		
		*IOAPIC_MAP.virtual_index_address = index + 1;
		// io_mfence;
		*IOAPIC_MAP.virtual_data_address = value & 0xffffffff;
		// io_mfence;
	}

	static inline unsigned long register_irq(unsigned long irq,
			void * arg,
			void (*handler)(unsigned long nr, unsigned long parameter),
			unsigned long parameter,
			hw_int_controller *controller,
			char *irq_name)
	{	
		irq_desc_T * p = &interrupt_desc[irq - 32];
		
		p->controller 	= controller;
		p->irq_name 	= irq_name;
		p->parameter 	= parameter;
		p->flags 		= 0;
		p->handler 		= handler;

		p->controller->install(irq,arg);
		p->controller->enable(irq);
		
		return 1;
	}

	static inline unsigned long unregister_irq(unsigned long irq)
	{
		irq_desc_T * p = &interrupt_desc[irq - 32];

		p->controller->disable(irq);
		p->controller->uninstall(irq);

		p->controller 	= NULL;
		p->irq_name 	= NULL;
		p->parameter 	= NULL;
		p->flags 		= 0;
		p->handler 		= NULL;

		return 1; 
	}

	static inline void IOAPIC_enable(unsigned long irq)
	{
		unsigned long value = 0;
		value = ioapic_rte_read((irq - 32) * 2 + 0x10);
		value = value & (~0x10000UL); 
		ioapic_rte_write((irq - 32) * 2 + 0x10, value);
	}

	static inline void IOAPIC_disable(unsigned long irq)
	{
		unsigned long value = 0;
		value = ioapic_rte_read((irq - 32) * 2 + 0x10);
		value = value | 0x10000UL; 
		ioapic_rte_write((irq - 32) * 2 + 0x10,value);
	}

	static inline unsigned long IOAPIC_install(unsigned long irq, void *arg)
	{
		ioapic_rte_write((irq - 32) * 2 + 0x10, *((unsigned long *)arg));
		return 1;
	}

	static inline void IOAPIC_uninstall(unsigned long irq)
	{
		ioapic_rte_write((irq - 32) * 2 + 0x10,0x10000UL);
	}

	static inline void IOAPIC_level_ack(unsigned long irq)
	{
		wrmsr(0x80b, 0UL);
		*IOAPIC_MAP.virtual_EOI_address = 0;
	}

	static inline void IOAPIC_edge_ack(unsigned long irq)
	{
		wrmsr(0x80b, 0UL);
	}
#pragma endregion

#pragma region Spin lock
	static inline void spin_init(spinlock_T *lock)
	{
		lock->lock = 1;
	}

	static inline void spin_lock(spinlock_T *lock)
	{
		__asm__ __volatile__(
			"1:	\n\t"
			"lock	decq	%0	\n\t"
			"jns	3f	\n\t"			//3f indicate label "3", seeking forward
			"2:	\n\t"
			"pause	\n\t"				//Wait for about 30 clock cycles
			"cmpq	$0,	%0\n\t"
			"jle	2b	\n\t"			//2b indicate label "2", seeking backward
			"jmp	1b	\n\t"
			"3:	\n\t"
			:"=m"(lock->lock)
			:
			:"memory"
		);
	}

	static inline void spin_unlock(spinlock_T *lock)
	{
		__asm__ __volatile__(
			"movq	$1,	%0	\n\t"
			:"=m"(lock->lock)
			:
			:"memory"
		);
	}
	
	#define spin_lock_irqsave(lock,flags) __asm__ __volatile__("pushfq ; popq %0 ; cli":"=g"(flags)::"memory");spin_lock(lock)
	#define spin_unlock_irqrestore(lock,flags) spin_unlock(lock);__asm__ __volatile__("pushq %0 ; popfq"::"g"(flags):"memory")
#pragma endregion

#pragma region SoftIRQ
	static inline void set_softirq_status(unsigned long status)
	{
		SoftIRQ_status |= status;
	}
	static inline unsigned long get_softirq_status()
	{
		return SoftIRQ_status;
	}
	static inline void register_softirq(int nr,void (*action)(void * data),void * data)
	{
		SoftIRQ_vector[nr].action = action;
		SoftIRQ_vector[nr].data = data;
	}
	static inline void unregister_softirq(int nr)
	{
		SoftIRQ_vector[nr].action = NULL;
		SoftIRQ_vector[nr].data = NULL;
	}
	static inline void softirq_init()
	{
		SoftIRQ_status = 0;
		memset(SoftIRQ_vector,0,sizeof(softirq) * 64);
	}
	static inline void do_softirq()
	{
		int i;
		sti;
		for(i = 0;i < 64 && SoftIRQ_status;i++)
		{
			if(SoftIRQ_status & (1 << i))
			{
				SoftIRQ_vector[i].action(SoftIRQ_vector[i].data);
				SoftIRQ_status &= ~(1 << i);
			}
		}
		cli;
	}
#pragma endregion

#pragma region task
static inline task_struct* get_current()
{
	task_struct * current = NULL;
	__asm__ __volatile__ ("andq %%rsp,%0 \n\t":"=r"(current):"0"(~32767UL));
	return current;
}

#define current get_current()
schedule_struct task_schedule[NR_CPUS];
task_union __attribute__((__section__ (".data.init_task"))) init_task_union;

static inline void insert_task_queue(task_struct *tsk)
{
    task_struct *tmp = container_of(task_schedule[current->cpu_id].task_queue.List.next, task_struct, List);

    if (tsk == &init_task_union.Task || tsk->pid < task_schedule[current->cpu_id].running_task_count) {
        // color_printk(ORANGE,BLACK," ;not inserted; ");
        return;
    }

    if (!List_is_empty(&task_schedule[current->cpu_id].task_queue.List)) {
        while (tmp->vrun_time < tsk->vrun_time) {
            tmp = container_of(tmp->List.next, task_struct, List);
        }
    }
    List_Insert_Before(&tmp->List, &tsk->List);

    task_schedule[current->cpu_id].running_task_count += 1;
}
void schedule(void);
#pragma endregion

#pragma region User_API
static inline long verify_area(unsigned char* addr,unsigned long size)
{
	if(((unsigned long)addr + size) <= (unsigned long)0x00007fffffffffff ){
		return 1;
	}
	else{
		return 0;
	}
}
static inline long strnlen_user(void *src, unsigned long maxlen)
{
	unsigned long size = strlen(src);
	if(!verify_area(src,size)){
		return 0;
	}
	if(size < maxlen){
		return size;
	}else{
		return maxlen;
	}
}
static inline long strncpy_from_user(void * from,void * to,unsigned long size)
{
	if(!verify_area(from,size)){
		return 0;
	}

	strncpy(to,from,size);
	return	size;
}
#pragma endregion

#pragma region wait_queue
static inline void wait_queue_init(wait_queue_T *wait_queue, task_struct *tsk)
{
	List_Init(&wait_queue->List);
	wait_queue->tsk = tsk;
}
static inline void sleep(wait_queue_T *queue_head)
{
	wait_queue_T wait;
	wait_queue_init(&wait,current);
	current->state = TASK_UNINTERRUPTIBLE;
	List_Insert_Before(&queue_head->List, &wait.List);
	
	schedule();
}
static inline void wakeup(wait_queue_T *queue_head, unsigned long state)
{
	wait_queue_T *wait = NULL;

	if(List_is_empty(&queue_head->List)){
		return;
	}

	wait = container_of(queue_head->List.next, wait_queue_T, List);
	if(wait->tsk->state & state){
		List_Del(&wait->List);
		wait->tsk->state = TASK_RUNNING;
		insert_task_queue(wait->tsk);
		current->flag |= NEED_SCHEDULE;
	}
}
#pragma endregion

#pragma region keyboard_ops
long keyboard_open(FS_entry* entry);
long keyboard_close(FS_entry* entry);
long keyboard_ioctl(unsigned long cmd);
long keyboard_read(FS_entry *entry, void *buf, unsigned long count);
long keyboard_write(FS_entry *entry, void *buf, unsigned long count);
#pragma endregion