#include "INT.h"
#include "../Task.h"

void Init_IDT(void)
{
    _Set_INT(IDT_PTR.Offset,      ATTR_TRAP_GATE,     1, Div_Eorr);
    _Set_INT(IDT_PTR.Offset + 1,  ATTR_TRAP_GATE,     1, Debug);
    _Set_INT(IDT_PTR.Offset + 2,  ATTR_INTR_GATE,     1, NMI);
    _Set_INT(IDT_PTR.Offset + 3,  ATTR_SYS_TRAP_GATE, 1, INT3);
    _Set_INT(IDT_PTR.Offset + 4,  ATTR_SYS_TRAP_GATE, 1, OverFlow);
    _Set_INT(IDT_PTR.Offset + 5,  ATTR_SYS_TRAP_GATE, 1, Bounds);
    _Set_INT(IDT_PTR.Offset + 6,  ATTR_TRAP_GATE,     1, Undefined_Opcode);
	_Set_INT(IDT_PTR.Offset + 7,  ATTR_TRAP_GATE,     1, Dev_Not_Available);
	_Set_INT(IDT_PTR.Offset + 8,  ATTR_TRAP_GATE,     1, Double_Fault);
	_Set_INT(IDT_PTR.Offset + 9,  ATTR_TRAP_GATE,     1, Coprocessor_Segment_Overrun);
	_Set_INT(IDT_PTR.Offset + 10, ATTR_TRAP_GATE,     1, Invalid_TSS);
	_Set_INT(IDT_PTR.Offset + 11, ATTR_TRAP_GATE,     1, Segment_Not_Present);
	_Set_INT(IDT_PTR.Offset + 12, ATTR_TRAP_GATE,     1, Stack_Segment_Fault);
	_Set_INT(IDT_PTR.Offset + 13, ATTR_TRAP_GATE,     1, General_Protection);
    _Set_INT(IDT_PTR.Offset + 14, ATTR_TRAP_GATE,     1, Page_Fault);
    _Set_INT(IDT_PTR.Offset + 16, ATTR_TRAP_GATE,     1, x87_FPU_Error);
	_Set_INT(IDT_PTR.Offset + 17, ATTR_TRAP_GATE,     1, Alignment_Check);
	_Set_INT(IDT_PTR.Offset + 18, ATTR_TRAP_GATE,     1, Machine_Check);
	_Set_INT(IDT_PTR.Offset + 19, ATTR_TRAP_GATE,     1, SIMD_Exception);
	_Set_INT(IDT_PTR.Offset + 20, ATTR_TRAP_GATE,     1, Virtualization_Exception);

    return;
}

#pragma region Sys_INT_Func
__attribute__ ((interrupt)) void Div_Eorr(Int_Frame *STK)
{
    unsigned long *rbp;
    get_rbp(rbp);
    rbp = (unsigned long *)*rbp;
    color_printk(RED,BLACK,"do_divide_error(0),ERROR_CODE:%p,RSP:%p,RIP:%p,CPU:%p\n", 0, STK->RSP, STK->RIP, current->cpu_id);
    backtrace(rbp, STK->RIP, STK->RSP);
    while(1);
}
__attribute__ ((interrupt)) void Debug(Int_Frame *STK)
{
    color_printk(RED,BLACK,"do_debug(1),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", 0, STK->RSP, STK->RIP, current->cpu_id);
    while(1);
}

__attribute__ ((interrupt)) void NMI(Int_Frame *STK)
{
    color_printk(RED,BLACK,"do_nmi(2),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", 0, STK->RSP, STK->RIP, current->cpu_id);
    while(1);
}

__attribute__ ((interrupt)) void INT3(Int_Frame *STK)
{
    color_printk(RED,BLACK,"do_int3(3),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", 0, STK->RSP, STK->RIP, current->cpu_id);
    while(1);
}

__attribute__ ((interrupt)) void OverFlow(Int_Frame *STK)
{
    color_printk(RED,BLACK,"do_overflow(4),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", 0, STK->RSP, STK->RIP, current->cpu_id);
    while(1);
}

__attribute__ ((interrupt)) void Bounds(Int_Frame *STK)
{
    color_printk(RED,BLACK,"do_bounds(5),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", 0, STK->RSP, STK->RIP, current->cpu_id);
    while(1);
}

__attribute__ ((interrupt)) void Undefined_Opcode(Int_Frame *STK)
{
    color_printk(RED,BLACK,"do_undefined_opcode(6),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", 0, STK->RSP, STK->RIP, current->cpu_id);
    while(1);
}

__attribute__ ((interrupt)) void Dev_Not_Available(Int_Frame *STK)
{
    color_printk(RED,BLACK,"do_dev_not_available(7),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", 0, STK->RSP, STK->RIP, current->cpu_id);
    while(1);
}

__attribute__ ((interrupt)) void Double_Fault(Int_Frame *STK, unsigned long Err_Code)
{
    color_printk(RED, BLACK, "STK: %p\n", STK);
    color_printk(RED,BLACK,"do_double_fault(8),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", Err_Code, STK->RSP, STK->RIP, current->cpu_id);
    while(1);
}

__attribute__ ((interrupt)) void Coprocessor_Segment_Overrun(Int_Frame *STK)
{
    color_printk(RED,BLACK,"do_coprocessor_segment_overrun(9),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", 0, STK->RSP, STK->RIP, current->cpu_id);
    while(1);
}

__attribute__ ((interrupt)) void Invalid_TSS(Int_Frame *STK, unsigned long Err_Code)
{
    color_printk(RED,BLACK,"do_invalid_TSS(10),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", Err_Code, STK->RSP, STK->RIP, current->cpu_id);

    if (Err_Code & 0x01) {
        color_printk(RED,BLACK,"The exception occurred during delivery of an event external to the program,"
            "such as an interrupt or an earlier exception.\n");
    }
    if (Err_Code & 0x02){
        color_printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
    }else{
        if(Err_Code & 0x04){
            color_printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
        }else{
            color_printk(RED,BLACK,"Refers to a descriptor in the current GDT;\n");
        }
    }

    color_printk(RED,BLACK,"Segment Selector Index:%#010x\n",Err_Code & 0xfff8);

    while(1);
}

__attribute__ ((interrupt)) void Segment_Not_Present(Int_Frame *STK, unsigned long Err_Code)
{
    color_printk(RED,BLACK,"do_segment_not_present(11),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", Err_Code, STK->RSP, STK->RIP, current->cpu_id);

    if (Err_Code & 0x01) {
        color_printk(RED,BLACK,"The exception occurred during delivery of an event external to the program,"
            "such as an interrupt or an earlier exception.\n");
    }
    
    if (Err_Code & 0x02){
        color_printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
    }else{
        if(Err_Code & 0x04){
            color_printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
        }else{
            color_printk(RED,BLACK,"Refers to a descriptor in the current GDT;\n");
        }
    }

    color_printk(RED,BLACK,"Segment Selector Index:%#010x\n",Err_Code & 0xfff8);

    while(1);
}

__attribute__ ((interrupt)) void Stack_Segment_Fault(Int_Frame *STK, unsigned long Err_Code)
{
    color_printk(RED,BLACK,"do_stack_segment_fault(12),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", Err_Code, STK->RSP, STK->RIP, current->cpu_id);

    if (Err_Code & 0x01) {
        color_printk(RED,BLACK,"The exception occurred during delivery of an event external to the program,"
            "such as an interrupt or an earlier exception.\n");
    }
    
    if (Err_Code & 0x02){
        color_printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
    }else{
        if(Err_Code & 0x04){
            color_printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
        }else{
            color_printk(RED,BLACK,"Refers to a descriptor in the current GDT;\n");
        }
    }

    color_printk(RED,BLACK,"Segment Selector Index:%#010x\n",Err_Code & 0xfff8);

    while(1);
}

__attribute__ ((interrupt)) void General_Protection(Int_Frame *STK, unsigned long Err_Code)
{
    color_printk(RED,BLACK,"do_general_ppppprotection(13),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", Err_Code, STK->RSP, STK->RIP, current->cpu_id);

    if (Err_Code & 0x01) {
        color_printk(RED,BLACK,"The exception occurred during delivery of an event external to the program,"
            "such as an interrupt or an earlier exception.\n");
    }
    
    if (Err_Code & 0x02){
        color_printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
    }else{
        if(Err_Code & 0x04){
            color_printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
        }else{
            color_printk(RED,BLACK,"Refers to a descriptor in the current GDT;\n");
        }
    }

    color_printk(RED,BLACK,"Segment Selector Index:%#010x\n",Err_Code & 0xfff8);

    while(1);
}

__attribute__ ((interrupt)) void Page_Fault(Int_Frame *STK, unsigned long Err_Code)
{
    unsigned long CR2 = 0;

    __asm__	__volatile__("movq %%cr2, %0":"=r"(CR2)::"memory");

    color_printk(RED,BLACK,"do_page_fault(14),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", Err_Code, STK->RSP, STK->RIP, current->cpu_id);

    if(!(Err_Code & 0x01)){
        color_printk(RED,BLACK,"Page Not-Present,\t");
    }
    
    if(Err_Code & 0x02){
        color_printk(RED,BLACK,"Write Cause Fault,\t");
    }
	else{
        color_printk(RED,BLACK,"Read Cause Fault,\t");
    }
        
    if(Err_Code & 0x04){
        color_printk(RED,BLACK,"Fault in user(3)\t");
    }
	else{
        color_printk(RED,BLACK,"Fault in supervisor(0,1,2)\t");
    }

    if(Err_Code & 0x08){
        color_printk(RED,BLACK,",Reserved Bit Cause Fault\t");
    }

    if(Err_Code & 0x10){
        color_printk(RED,BLACK,",Instruction fetch Cause Fault");
    }
    color_printk(RED,BLACK,"\n");
    color_printk(RED,BLACK,"CR2:%#016lx\n",CR2);

    while(1);
}

__attribute__ ((interrupt)) void x87_FPU_Error(Int_Frame *STK)
{
    color_printk(RED,BLACK,"do_x87_FPU_error(16),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", 0, STK->RSP, STK->RIP, current->cpu_id);
    while(1);   
}

__attribute__ ((interrupt)) void Alignment_Check(Int_Frame *STK, unsigned long Err_Code)
{
    color_printk(RED,BLACK,"do_alignment_check(17),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", Err_Code, STK->RSP, STK->RIP, current->cpu_id);
    while(1);
}

__attribute__ ((interrupt)) void Machine_Check(Int_Frame *STK)
{
    color_printk(RED,BLACK,"do_machine_check(18),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", 0, STK->RSP, STK->RIP, current->cpu_id);
    while(1);
}

__attribute__ ((interrupt)) void SIMD_Exception(Int_Frame *STK)
{
    color_printk(RED,BLACK,"do_SIMD_exception(19),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", 0, STK->RSP, STK->RIP, current->cpu_id);
    while(1);
}

__attribute__ ((interrupt)) void Virtualization_Exception(Int_Frame *STK)
{
    color_printk(RED,BLACK,"do_virtualization_exception(20),ERROR_CODE:%#016lx,RSP:%#016lx,RIP:%#016lx,CPU:%#016lx\n", 0, STK->RSP, STK->RIP, current->cpu_id);
    while(1);
}
#pragma endregion

#pragma region Interrupt: IRQ0x20-IRQ0x37
__attribute__ ((interrupt)) void IRQ0x20_interrupt(Int_Frame *STK)//APIC timer
{
	// color_printk(BLACK,GREEN,"RIP:%p,CS:%p,RSP:%p,SS:%p\n",STK->RIP,STK->CS,STK->RSP,STK->SS);
    // while(1);
    if(interrupt_desc[0].handler != NULL){
        interrupt_desc[0].handler(0x00, interrupt_desc[0].parameter);
    }
	if(interrupt_desc[0].controller->ack != NULL){
        interrupt_desc[0].controller->ack(0x20);
    }
	if(SoftIRQ_status && interrupt_desc[0].controller->do_soft != NULL){
        interrupt_desc[0].controller->do_soft();
    }
    if(current->flag & NEED_SCHEDULE){
        schedule();
    }
}
__attribute__ ((interrupt)) void IRQ0x21_interrupt(Int_Frame *STK) //Keyboard interrupt
{
    if(interrupt_desc[1].handler != NULL){
        interrupt_desc[1].handler(0x01, interrupt_desc[1].parameter);
    }
	if(interrupt_desc[1].controller->ack != NULL){
        interrupt_desc[1].controller->ack(0x21);
    }
}
__attribute__ ((interrupt)) void IRQ0x22_interrupt(Int_Frame *STK)//HPET Timer interrupt
{
	if(interrupt_desc[2].handler != NULL){
        interrupt_desc[2].handler(0x02, interrupt_desc[2].parameter);
    }
	if(interrupt_desc[2].controller->ack != NULL){
        interrupt_desc[2].controller->ack(0x22);
    }
	if(SoftIRQ_status && interrupt_desc[2].controller->do_soft != NULL){
        interrupt_desc[2].controller->do_soft();
    }
    if(current->flag & NEED_SCHEDULE){
        schedule();
    }
}
__attribute__ ((interrupt)) void IRQ0x23_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x23\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x24_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x24\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x25_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x25\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x26_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x26\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x27_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x27\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x28_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x28\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x29_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x29\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x2a_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x2a\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x2b_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x2b\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x2c_interrupt(Int_Frame *STK) //Mouse interrupt
{
	if(interrupt_desc[0x0c].handler != NULL){
        interrupt_desc[0x0c].handler(0x0c, interrupt_desc[0xc].parameter);
    }
	if(interrupt_desc[0x0c].controller->ack != NULL){
        interrupt_desc[0x0c].controller->ack(0x2c);
    }
}
__attribute__ ((interrupt)) void IRQ0x2d_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x2d\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x2e_interrupt(Int_Frame *STK) //Disk interrupt
{
	if(interrupt_desc[0x0e].handler != NULL){
        interrupt_desc[0x0e].handler(0x0e, interrupt_desc[0xe].parameter);
    }
	if(interrupt_desc[0x0e].controller->ack != NULL){
        interrupt_desc[0x0e].controller->ack(0x2e);
    }
}
__attribute__ ((interrupt)) void IRQ0x2f_interrupt(Int_Frame *STK)
{
	if(interrupt_desc[0x0f].handler != NULL){
        interrupt_desc[0x0f].handler(0x0f, interrupt_desc[0xf].parameter);
    }
	if(interrupt_desc[0x0f].controller->ack != NULL){
        interrupt_desc[0x0f].controller->ack(0x2f);
    }
}
__attribute__ ((interrupt)) void IRQ0x30_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x30\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x31_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x31\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x32_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x32\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x33_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x33\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x34_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x34\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x35_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x35\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x36_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x36\t");
	io_out8(0x20,0x20);
}
__attribute__ ((interrupt)) void IRQ0x37_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"do_IRQ: 0x37\t");
	io_out8(0x20,0x20);
}
#pragma endregion

#pragma region SMP-INIT
__attribute__ ((interrupt)) void IRQ0xC8_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"SMP IPI: 0xC8, APIC_ID: %#08x\n", Get_APIC_ID());
	//io_out8(0x20,0x20);//Sent EOI to port 0x20 (8529A master)
    wrmsr(0x80b, 0UL);
}
__attribute__ ((interrupt)) void IRQ0xC9_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"SMP IPI: 0xC9, APIC_ID: %#08x\n", Get_APIC_ID());
	//io_out8(0x20,0x20);//Sent EOI to port 0x20 (8529A master)
    wrmsr(0x80b, 0UL);
}
__attribute__ ((interrupt)) void IRQ0xCA_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"SMP IPI: 0xCA, APIC_ID: %#08x\n", Get_APIC_ID());
	//io_out8(0x20,0x20);//Sent EOI to port 0x20 (8529A master)
    wrmsr(0x80b, 0UL);
}
__attribute__ ((interrupt)) void IRQ0xCB_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"SMP IPI: 0xCB, APIC_ID: %#08x\n", Get_APIC_ID());
	//io_out8(0x20,0x20);//Sent EOI to port 0x20 (8529A master)
    wrmsr(0x80b, 0UL);
}
__attribute__ ((interrupt)) void IRQ0xCC_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"SMP IPI: 0xCC, APIC_ID: %#08x\n", Get_APIC_ID());
	//io_out8(0x20,0x20);//Sent EOI to port 0x20 (8529A master)
    wrmsr(0x80b, 0UL);
}
__attribute__ ((interrupt)) void IRQ0xCD_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"SMP IPI: 0xCD, APIC_ID: %#08x\n", Get_APIC_ID());
	//io_out8(0x20,0x20);//Sent EOI to port 0x20 (8529A master)
    wrmsr(0x80b, 0UL);
}
__attribute__ ((interrupt)) void IRQ0xCE_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"SMP IPI: 0xCE, APIC_ID: %#08x\n", Get_APIC_ID());
	//io_out8(0x20,0x20);//Sent EOI to port 0x20 (8529A master)
    wrmsr(0x80b, 0UL);
}
__attribute__ ((interrupt)) void IRQ0xCF_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"SMP IPI: 0xCF, APIC_ID: %#08x\n", Get_APIC_ID());
	//io_out8(0x20,0x20);//Sent EOI to port 0x20 (8529A master)
    wrmsr(0x80b, 0UL);
}
__attribute__ ((interrupt)) void IRQ0xD0_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"SMP IPI: 0xD0, APIC_ID: %#08x\n", Get_APIC_ID());
	//io_out8(0x20,0x20);//Sent EOI to port 0x20 (8529A master)
    wrmsr(0x80b, 0UL);
}
__attribute__ ((interrupt)) void IRQ0xD1_interrupt(Int_Frame *STK)
{
	color_printk(RED,BLACK,"SMP IPI: 0xD1, APIC_ID: %#08x\n", Get_APIC_ID());
	//io_out8(0x20,0x20);//Sent EOI to port 0x20 (8529A master)
    wrmsr(0x80b, 0UL);
}
#pragma endregion