.text
.code16

.globl _APU_boot_start
_APU_boot_start:
_APU_boot_base = .

    cli
    wbinvd  //Copy cache into the memory and invalid the cache
            //For example to save to disk using DMA, the content in cache have to write into the memory
            //For example to read from disk using DMA, the cache need to be invalidated so that cpu do not read the old data in the cache
	mov	%cs,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%ss //ss = _APU_boot_base
	mov	%ax,	%fs
	mov	%ax,	%gs
	
	movl	$(_APU_boot_tmp_Stack_end - _APU_boot_base),	%esp //ss:sp = _APU_boot_base:_APU_boot_tmp_Stack_end

	mov		%cs,	%ax
	movzx	%ax,	%esi
	shll	$4,		%esi

	leal	(_APU_Code32 - _APU_boot_base)(%esi),	%eax
	movl	%eax,	_APU_Code32_vector - _APU_boot_base
	leal	(_APU_Code64 - _APU_boot_base)(%esi),	%eax
	movl	%eax,	_APU_Code64_vector - _APU_boot_base
	leal	(_APU_tmp_GDT - _APU_boot_base)(%esi),	%eax
	movl	%eax,	(_APU_tmp_GDT + 2 - _APU_boot_base)	//Re-calculate GDT table address
	
    lidtl	_APU_tmp_IDT - _APU_boot_base //lidt+l: Indicating 32bit operand, using relative addressing resulting in lidt ds:ADDR
	lgdtl	_APU_tmp_GDT - _APU_boot_base
	
	movl 	%cr0,	%eax
	bts		$0,     %ax
	movl	%eax,    %cr0

	ljmpl	*(_APU_Code32_vector - _APU_boot_base)//Here have to minuse to express relative address, since the original address are not clear.
		//Before jmp, ds not flushed ,thus jmp [ds:ADDR]
		//Operand can only be setted number without registers
		//Real mode: ljmp: operand[16:16]
		//Prot mode: ljmp: operand[16:32],
		//Long mode: ljmp: operand[16:32], .byte 0x48, ljmp: operand[16:64]
.code32
_APU_Code32:
	mov	$0x10,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%ss //ss = _APU_boot_base
	mov	%ax,	%fs
	mov	%ax,	%gs

	movl	(_APU_boot_tmp_Stack_end - _APU_boot_base)(%esi),	%esp 

	//Open PAE
	movl	%cr4,	%eax
	bts		$5,		%eax
	movl	%eax,	%cr4
	//set page table
	movl	$0x101000,	%eax
	movl	%eax,		%cr3
	//enable long mode
	movl	$0xC0000080,	%ecx
	rdmsr
	bts	$8,	%eax 			//IA32_EFER.LME[8] = 1, Enable 64bit long mode
	wrmsr
	//enable PE & paging
	movl	%cr0,	%eax
	bts		$0,		%eax
	bts		$31,	%eax
	movl	%eax,	%cr0

	ljmp	*(_APU_Code64_vector - _APU_boot_base)(%esi)
	//Protected mode ds is selector, thus need to calculate the absolute address

.code64
.balign 4
_APU_Code64:
	mov	$0x20,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%ss //ss = _APU_boot_base
	mov	%ax,	%fs
	mov	%ax,	%gs

	movq	$0x100000,	%rax
	jmpq	*%rax

    hlt

.balign 4
_APU_tmp_IDT:
	.word	0   //2 byte
	.word	0,0 //4 byte

.balign 4
_APU_tmp_GDT:
	.short	_APU_tmp_GDT_end - _APU_tmp_GDT - 1 //2 byte
	.long	_APU_tmp_GDT - _APU_boot_base       //4 byte
	.short	0                                   //2 byte: first 8 byte not used
	.quad	0x00cf9a000000ffff
	.quad	0x00cf92000000ffff
	.quad	0x0020980000000000
	.quad	0x0000920000000000
_APU_tmp_GDT_end:

.balign 4
_APU_Code32_vector:
	.long	_APU_Code32 - _APU_boot_base
	.word	0x08,0	

.balign 4
_APU_Code64_vector:
	.long	_APU_Code64 - _APU_boot_base
	.word	0x18,0	

_APU_boot_tmp_Stack_start:
	.org	0x400   //Marking end postion at 400 byte
_APU_boot_tmp_Stack_end:
.globl _APU_boot_end
_APU_boot_end: