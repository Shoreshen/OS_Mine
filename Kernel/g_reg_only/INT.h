#pragma once

#include "../lib.h"

#pragma region Definition
    #define Get_IDT(IDTR) {__asm__ __volatile__ ("sidt %0"::"m"(IDTR):);} //Input constraints: "m": memory
    #define ATTR_INTR_GATE 0X8E //Interrupt gate will automatically distable interrupt: cli
    #define ATTR_TRAP_GATE 0X8F 
    #define ATTR_SYS_TRAP_GATE 0xEF
    #define ATTR_SYS_INTR_GATE 0xEE //Interrupt gate will automatically distable interrupt: cli
    #define _Set_INT(gate_selector_addr, attr, ist, code_addr)\
        {\
            __asm__ __volatile__ (\
                "movw	%%dx,	%%ax	\n\t"	\
                "andq	$0x7,	%%rcx	\n\t"	\
                "addq	%4,	    %%rcx	\n\t"	\
                "shlq	$32,	%%rcx	\n\t"	\
                "addq	%%rcx,	%%rax	\n\t"	\
                "xorq	%%rcx,	%%rcx	\n\t"	\
                "movl	%%edx,	%%ecx	\n\t"	\
                "shrq	$16,	%%rcx	\n\t"	\
                "shlq	$48,	%%rcx	\n\t"	\
                "addq	%%rcx,	%%rax	\n\t"	\
                "movq	%%rax,	%0	    \n\t"	\
                "shrq	$32,	%%rdx	\n\t"	\
                "movq	%%rdx,	%1	    \n\t"	\
                :\
                :"m"(*((unsigned long *)(gate_selector_addr))), "m"(*(1 + (unsigned long *)(gate_selector_addr))),\
                 "d"((unsigned long *)(code_addr)), "a"(0x8 << 16), "i"(attr << 8), "c"(ist)\
                :"memory"\
            );\
        };
#pragma endregion

#pragma region globalVar
unsigned long rflag = 0;
#pragma endregion

#pragma region Function declear
    void Init_IDT(void);
    void Init_8259A(void);
    void Local_APIC_init(void);
    void APIC_IOAPIC_init(void);
#pragma endregion

#pragma region Sys_INT_Func
__attribute__ ((interrupt)) void Div_Eorr(Int_Frame *STK);
__attribute__ ((interrupt)) void Debug(Int_Frame *STK);
__attribute__ ((interrupt)) void NMI(Int_Frame *STK);
__attribute__ ((interrupt)) void INT3(Int_Frame *STK);
__attribute__ ((interrupt)) void OverFlow(Int_Frame *STK);
__attribute__ ((interrupt)) void Bounds(Int_Frame *STK);
__attribute__ ((interrupt)) void Undefined_Opcode(Int_Frame *STK);
__attribute__ ((interrupt)) void Dev_Not_Available(Int_Frame *STK);
__attribute__ ((interrupt)) void Double_Fault(Int_Frame *STK, unsigned long Err_Code);
__attribute__ ((interrupt)) void Coprocessor_Segment_Overrun(Int_Frame *STK);
__attribute__ ((interrupt)) void Invalid_TSS(Int_Frame *STK, unsigned long Err_Code);
__attribute__ ((interrupt)) void Segment_Not_Present(Int_Frame *STK, unsigned long Err_Code);
__attribute__ ((interrupt)) void Stack_Segment_Fault(Int_Frame *STK, unsigned long Err_Code);
__attribute__ ((interrupt)) void General_Protection(Int_Frame *STK, unsigned long Err_Code);
__attribute__ ((interrupt)) void Page_Fault(Int_Frame *STK, unsigned long Err_Code);
__attribute__ ((interrupt)) void x87_FPU_Error(Int_Frame *STK);
__attribute__ ((interrupt)) void Alignment_Check(Int_Frame *STK, unsigned long Err_Code);
__attribute__ ((interrupt)) void Machine_Check(Int_Frame *STK);
__attribute__ ((interrupt)) void SIMD_Exception(Int_Frame *STK);
__attribute__ ((interrupt)) void Virtualization_Exception(Int_Frame *STK);
#pragma endregion

#pragma region Interrupt: IRQ0x20-IRQ0x37
__attribute__ ((interrupt)) void  IRQ0x20_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x21_interrupt(Int_Frame *STK); //Keyboard interrupt
__attribute__ ((interrupt)) void  IRQ0x22_interrupt(Int_Frame *STK); //Timer interrupt
__attribute__ ((interrupt)) void  IRQ0x23_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x24_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x25_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x26_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x27_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x28_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x29_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x2a_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x2b_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x2c_interrupt(Int_Frame *STK); //Mouse interrupt
__attribute__ ((interrupt)) void  IRQ0x2d_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x2e_interrupt(Int_Frame *STK); //Disk interrupt
__attribute__ ((interrupt)) void  IRQ0x2f_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x30_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x31_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x32_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x33_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x34_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x35_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x36_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void  IRQ0x37_interrupt(Int_Frame *STK);
void (* interrupt[24])(Int_Frame *STK)=
{
	IRQ0x20_interrupt,
	IRQ0x21_interrupt,
	IRQ0x22_interrupt,
	IRQ0x23_interrupt,
	IRQ0x24_interrupt,
	IRQ0x25_interrupt,
	IRQ0x26_interrupt,
	IRQ0x27_interrupt,
	IRQ0x28_interrupt,
	IRQ0x29_interrupt,
	IRQ0x2a_interrupt,
	IRQ0x2b_interrupt,
	IRQ0x2c_interrupt,
	IRQ0x2d_interrupt,
	IRQ0x2e_interrupt,
	IRQ0x2f_interrupt,
	IRQ0x30_interrupt,
	IRQ0x31_interrupt,
	IRQ0x32_interrupt,
	IRQ0x33_interrupt,
	IRQ0x34_interrupt,
	IRQ0x35_interrupt,
	IRQ0x36_interrupt,
	IRQ0x37_interrupt
};
#pragma endregion

#pragma region SMP-INIT
__attribute__ ((interrupt)) void IRQ0xC8_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void IRQ0xC9_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void IRQ0xCA_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void IRQ0xCB_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void IRQ0xCC_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void IRQ0xCD_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void IRQ0xCE_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void IRQ0xCF_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void IRQ0xD0_interrupt(Int_Frame *STK);
__attribute__ ((interrupt)) void IRQ0xD1_interrupt(Int_Frame *STK);
void (* interrupt_smp[10])(Int_Frame *STK)=
{
    IRQ0xC8_interrupt,
    IRQ0xC9_interrupt,
    IRQ0xCA_interrupt,
    IRQ0xCB_interrupt,
    IRQ0xCC_interrupt,
    IRQ0xCD_interrupt,
    IRQ0xCE_interrupt,
    IRQ0xCF_interrupt,
    IRQ0xD0_interrupt,
    IRQ0xD1_interrupt
};
#pragma endregion
