//=====================================================================================
//代码段
//=====================================================================================
.section .text

.globl _start

_start:
//=====  Loading GDT & IDT
	lgdt  GDT_PTR(%rip)		//GDT_PTR(%rip) = [rip + GDT_PTR]: Relative address corresponding to rip
	lidt  IDT_PTR(%rip)

	mov   $0x0,  %ax	//For AMD using null selector while CPL = 0
	mov   %ax,   %ds
	mov   %ax,   %es
	mov   %ax,   %fs
	mov   %ax,   %gs
	mov   %ax,   %ss
	
	movq  $0x7e00, %rsp
 
 //=====  Refilling CR3
	movq  $0x101000, %rax
	movq  %rax, %cr3
 //===== If APU
	movq  $0x1b, %rcx
	rdmsr				// Read IA32_APIC_BASE register
	bt    $8, %rax		// Test IA32_APIC_BASE.BSP[8] bit to tell if BSP or AP
	jnc	start_smp
 //===== If BSP
	movq  switch_reg(%rip), %rax
	pushq $0x8
	pushq %rax
	lretq
	
entry64:
	movq  _stack_start(%rip), %rsp //Using stack space allocated by kernel

	movq  go_to_Kernel(%rip), %rax
	pushq $0x8
	push %rax
	lretq

start_smp:
	movq	smp_kernel(%rip),	%rax		/* movq address */
	pushq	$0x08
	pushq	%rax
	lretq
//*****************************************************************************
//Addressing symbol
//*****************************************************************************
//Cannot put in .data seg, since .text seg first, the length could excceed relative addressing ability
switch_reg:
	.quad entry64
go_to_Kernel:
	.quad Start_Kernel
smp_kernel:
	.quad Start_SMP

.global _stack_start
_stack_start:
	.quad init_task_union + 32768
//*****************************************************************************
//Paging tables
//*****************************************************************************
//Cannot put in .data seg, since .text seg first, the length could excceed 0x1000h
.org 0x1000
.globl __PML4E
__PML4E:
.quad 0x102003 /*Entry 0x0*/
	//PML4E Entry
		//P(Presented):
			//bit 0: 1b: Page in memory
		//R/W:
			//bit 1: 1b: Writable (0 = Read only)
		//U/S:
			//bit 2: 1b: User mode (0 = Super Mode, only priviliage 0,1,2 can access)
		//PWT:
			//bit 3: 0b
		//PCD
			//bit 4: 0b
		//A(Accessed):
			//bit 5: 0b: Not accessed
		//PDPTE Base Address:
			//bit 12 ~ M-1: 102h: Base address = 102h * 4kb = 102000h
			//M represent for MAXPHYADDR, mostly 52. Reported by CPUID.80000008H:EAX[7:0]
.fill 255,8,0
.quad 0x103003 /*Entry 0x100 = 100000000b*/
.fill 255,8,0

.org 0x2000
.globl __PDPTE
__PDPTE:
.quad 0x104003 /*Supser mode*/
	//PDPTE Entry
		//P(Presented):
			//bit 0: 1b: Page in memory
		//R/W:
			//bit 1: 1b: Writable (0b = Read only)
		//U/S:
			//bit 2: 1b: User mode (0 = Super Mode, only priviliage 0,1,2 can access)
		//PWT:
			//bit 3: 0b
		//PCD
			//bit 4: 0b
		//A(Accessed):
			//bit 5: 0b: Not accessed
		//D(Dirty)
			//bit 6: 0b
		//PS:
			//bit 7: 0b: Pointer => PDE Base Address (1b = Pointer => 1GB Pysical Base Address)
		//PDE Base Address:
			//bit 12 ~ M-1: 102h: Base address = 102h * 4kb = 102000h
			//M represent for MAXPHYADDR, mostly 52. Reported by CPUID.80000008H:EAX[7:0]
		//Others: 0h: reserved or no use
.fill 511,8,0

.org 0x3000
.globl __PDPTE_KEL
__PDPTE_KEL:
.quad 0x105003 /*Supser mode*/
.fill 511,8,0

.org 0x4000
.globl __PDE
__PDE:
.quad 	0x000083
	//PDE Entry
		//P(Presented):
			//bit 0: 1b: Page in memory
		//R/W:
			//bit 1: 1b: Writable (0b = Read only)
		//U/S:
			//bit 2: 0b: Super mode (can only accessed from priviliage level 0,1,2, 1b = User mode,can accessed from any priviliage level)
		//PWT:
			//bit 3: 0b
		//PCD
			//bit 4: 0b
		//A(Accessed):
			//bit 5: 0b: Not accessed
		//D(Dirty)
			//bit 6: 0b
		//PS:
			//bit 7: 1b: Pointer => 2MB Pysical Base Address (0b = Pointer => 1GB Pysical Base Address)
		//G:
			//bit 8: 0b: PS = 1b => Global; if CR4.PGE = 1, determines whether the translation is global (see Section 4.10); ignored otherwise
		//PDE Base Address:
			//bit 21 ~ M-1: 0h (a>>1): Base address = 0h * 2MB = 0h
			//M represent for MAXPHYADDR, mostly 52. Reported by CPUID.80000008H:EAX[7:0]
		//Others: 0h: reserved or no use
.quad 	0x200083	 /*Linear Address: 0xffff800000200000*/
.quad 	0x400083
.quad 	0x600083
.quad 	0x800083
.quad	0xa00083
.quad	0xc00083
.quad	0xe00083
.quad	0x1000083	 /*Linear Address: 0xffff800001000000*/
.quad	0x1200083
.quad	0x1400083
.quad	0x1600083
.quad	0x1800083
.quad	0x1a00083
.quad	0x1c00083
.quad	0x1e00083
.quad	0x2000083
.quad	0x2200083
.quad	0x2400083
.quad	0x2600083
.quad	0x2800083
.quad	0x2a00083
.quad	0x2c00083
.quad	0x2e00083
//Screen RAM map: 1152*1024*(32/8)/(2MB)=2.26 => At least 3 of 2MB pages
.quad 	0xfd000083 	/*Linear Address: 0xffff800003000000*/
.quad 	0xfd200083 	/*Linear Address: 0xffff800003200000*/
.quad 	0xfd400083 	/*Linear Address: 0xffff800003400000*/
.quad	0xe0600087
.quad	0xe0800087
.quad	0xe0a00087
.quad	0xe0c00087
.quad	0xe0e00087
.fill	480,8,0

.org 0x5000
.globl __PDE_KEL
__PDE_KEL:
.quad 	0x000083
.quad 	0x200083	 /*Linear Address: 0xffff800000200000*/
.quad 	0x400083
.quad 	0x600083
.quad 	0x800083
.quad	0xa00083
.quad	0xc00083
.quad	0xe00083
.quad	0x1000083	 /*Linear Address: 0xffff800001000000*/
.quad	0x1200083
.quad	0x1400083
.quad	0x1600083
.quad	0x1800083
.quad	0x1a00083
.quad	0x1c00083
.quad	0x1e00083
.quad	0x2000083
.quad	0x2200083
.quad	0x2400083
.quad	0x2600083
.quad	0x2800083
.quad	0x2a00083
.quad	0x2c00083
.quad	0x2e00083
//Screen RAM map: 1152*1024*(32/8)/(2MB)=2.26 => At least 3 of 2MB pages
.quad 	0xfd000083 	/*Linear Address: 0xffff800003000000*/
.quad 	0xfd200083 	/*Linear Address: 0xffff800003200000*/
.quad 	0xfd400083 	/*Linear Address: 0xffff800003400000*/
.quad	0xe0600087
.quad	0xe0800087
.quad	0xe0a00087
.quad	0xe0c00087
.quad	0xe0e00087
.fill	480,8,0
//-------------------------------------------------------------------------------------

//=====================================================================================
//数据段
//=====================================================================================
.section .data
//*****************************************************************************
//GDT table information
//*****************************************************************************
.globl GDT_TABLE
GDT_TABLE:
	.quad	0x0000000000000000			/*0	NULL descriptor		       	00*/
	.quad	0x0020980000000000			/*1	KERNEL	Code	64-bit	Segment	08*/
		//S:
			//bit 44:    1b: Non-system segment (DATA/CODE)
		//TYPE:
			//bit 43:	1b: S=1 => Code segment
			//bit 42:	0b: S=1 & Code-Seg => Non-conforming code segment (1 = conforming)
			//bit 41:	0b: S=1 & Code-Seg => Non-Readable
			//bit 40:	0b: S=1 & Code-Seg => Not visited (CPU set, user unset)
		//DPL:
			//bit 45~46: 00b: Highest privilege
		//P(Presented):
			//bit 47:    1b: Segment in memory
		//L:
			//bit 53: 	1b: 64bit mode(0b = 32bit compatible mode)
		//D/B:
			//bit 54:	0b: 64bit mode => Operation bit length: Code 32bit & Address 64bit
		//Others: 0h: reserved
	.quad	0x0000f20000000000			/*2	USER	Data	64-bit	Segment	30*/
		//S:
			//bit 44:    1b: Non-system segment (DATA/CODE)
		//TYPE:
			//bit 43:	0b: S=1 => Data segment
			//bit 42:	0b: S=1 & Data-Seg => Expand upward (0 = downward)
			//bit 41:	1b: S=1 & Data-Seg => Writable (0 = Non-Writable)
			//bit 40:	0b: S=1 & Data-Seg => Not visited (CPU set, user unset)
		//DPL:
			//bit 45~46: 11b: Lowest privilege
		//P(Presented):
			//bit 47:    1b: Segment in memory
		//L:
			//bit 53: 	0b: Data-Seg => no use
		//D/B:
			//bit 54:	0b: Data-Seg => no use
		//Others: 0h: reserved
	.quad	0x0020f80000000000			/*3	USER	Code	64-bit	Segment	28*/
		//S:
			//bit 44:    1b: Non-system segment (DATA/CODE)
		//TYPE:
			//bit 43:	1b: S=1 => Code segment
			//bit 42:	0b: S=1 & Code-Seg => Non-conforming code segment (1 = conforming)
			//bit 41:	0b: S=1 & Code-Seg => Non-Readable
			//bit 40:	0b: S=1 & Code-Seg => Not visited (CPU set, user unset)
		//DPL:
			//bit 45~46: 11b: Lowest privilege
		//P(Presented):
			//bit 47:    1b: Segment in memory
		//L:
			//bit 53: 	1b: 64bit mode(0b = 32bit compatible mode)
		//D/B:
			//bit 54:	0b: 64bit mode => Operation bit length: Code 32bit & Address 64bit
		//Others: 0h: reserved
	.quad	0x0020980000000000			/*4	KERNEL	Code	64-bit	Segment	08*/
		//S:
			//bit 44:    1b: Non-system segment (DATA/CODE)
		//TYPE:
			//bit 43:	1b: S=1 => Code segment
			//bit 42:	0b: S=1 & Code-Seg => Non-conforming code segment (1 = conforming)
			//bit 41:	0b: S=1 & Code-Seg => Non-Readable
			//bit 40:	0b: S=1 & Code-Seg => Not visited (CPU set, user unset)
		//DPL:
			//bit 45~46: 00b: Highest privilege
		//P(Presented):
			//bit 47:    1b: Segment in memory
		//L:
			//bit 53: 	1b: 64bit mode(0b = 32bit compatible mode)
		//D/B:
			//bit 54:	0b: 64bit mode => Operation bit length: Code 32bit & Address 64bit
		//Others: 0h: reserved
	.quad	0x0000920000000000			/*5	KERNEL	Data	64-bit	Segment	10*/
		//S:
			//bit 44:    1b: Non-system segment (DATA/CODE)
		//TYPE:
			//bit 43:	0b: S=1 => Data segment
			//bit 42:	0b: S=1 & Data-Seg => Expand upward (0 = downward)
			//bit 41:	1b: S=1 & Data-Seg => Writable (0 = Non-Writable)
			//bit 40:	0b: S=1 & Data-Seg => Not visited (CPU set, user unset)
		//DPL:
			//bit 45~46: 00b: Highest privilege
		//P(Presented):
			//bit 47:    1b: Segment in memory
		//L:
			//bit 53: 	0b: Data-Seg => no use
		//D/B:
			//bit 54:	0b: Data-Seg => no use
		//Others: 0h: reserved
	.quad	0x00cf9a000000ffff			/*6	KERNEL	Code	32-bit	Segment	38*/
		//Segment Limit:
			//bit 0~15:  0xFF,0xFF
			//bit 48~51: 0xF
			//Limit = 	0xFFFFF
		//G(Granularity):
			//bit 55:	1b: 4KB(0x1000) page
		//Total segment length (0xFFFFF+1) * 0x1000 = 0x100000000 (4GB)
		//Segment Base:
			//bit 16~39: 0x0,0x0,0x0
			//bit 56~63: 0x0
			//Base address: 0x0
		//DPL:
			//bit 45~46: 00b: Highest privilege
		//S:
			//bit 44:    1b: Non-system segment (DATA/CODE)
		//AVL:
			//bit 52:    0b
		//P(Presented):
			//bit 47:    1b: Segment in memory
		//D/B:
			//bit 54:	1b: Operation bit length 32b (0 = 16b)
		//L:
			//bit 53: 	0b: Reserved for protected mode, set 0b
		//TYPE:
			//bit 43:	1b: S=1 => Code segment
			//bit 42:	0b: S=1 & Code-Seg => Non-conforming code segment (1 = conforming)
			//bit 41:	1b: S=1 & Code-Seg => Readable (0 = Non-Readable)
			//bit 40:	0b: S=1 & Code-Seg => Not visited (CPU set, user unset)
	.quad	0x00cf92000000ffff			/*7	KERNEL	Data	32-bit	Segment	40*/
		//Segment Limit:
			//bit 0~15:  0xFF,0xFF
			//bit 48~51: 0xF
			//Limit = 	0xFFFFF
		//G(Granularity):
			//bit 55:	1b: 4KB(0x1000) page
		//Total segment length (0xFFFFF+1) * 0x1000 = 0x100000000 (4GB)
		//Segment Base:
			//bit 16~39: 0x0,0x0,0x0
			//bit 56~63: 0x0
			//Base address: 0x0
		//DPL:
			//bit 45~46: 00b: Highest privilege
		//S:
			//bit 44:    1b: Non-system segment (DATA/CODE)
		//AVL:
			//bit 52:    0b
		//P(Presented):
			//bit 47:    1b: Segment in memory
		//D/B:
			//bit 54:	1b: Operation bit length 32b (0 = 16b)
		//L:
			//bit 53: 	0b: Reserved for protected mode, set 0b
		//TYPE:
			//bit 43:	0b: S=1 => Data segment
			//bit 42:	0b: S=1 & Data-Seg => Expand upward (0 = downward)
			//bit 41:	1b: S=1 & Data-Seg => Writable (0 = Non-Writable)
			//bit 40:	0b: S=1 & Data-Seg => Not visited (CPU set, user unset)
	.fill	100,8,0					    /*10 ~ 11	TSS (jmp three segment <8,9>) in long-mode 128-bit 40*/
GDT_END:

.globl GDT_PTR
GDT_PTR:
GDT_LIMIT: .word GDT_END - GDT_TABLE - 1
GDT_BASE:  .quad GDT_TABLE
//*****************************************************************************
//IDT table information
//*****************************************************************************
.globl IDT_TABLE
IDT_TABLE:
	.fill  512,8,0
IDT_END:

.globl IDT_PTR
IDT_PTR:
IDT_LIMIT: .word IDT_END - IDT_TABLE - 1
IDT_BASE:  .quad IDT_TABLE
//*****************************************************************************
//TSS table information
//*****************************************************************************
/*.globl TSS_TABLE

TSS_TABLE:
	.fill 13,8,0
TSS_END:

TSS_PTR:
TSS_LIMIT: .word TSS_END - TSS_TABLE - 1
TSS_BASE:  .quad TSS_TABLE*/
//-------------------------------------------------------------------------------------
