OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
	. = 0xffff800000000000 + 0x100000;
	.text :
	{
		_text = .;
		*(.text)

		_etext = .;
	}
	.data :
	{
		_data = .;
		*(.data)
		_edata = .;
	}
	.rodata :
	{
		_rodata = .;
		*(.rodata)
		_erodata = .;
	}
	.data.init_task ALIGN(32768): 
	{ 
		*(.data.init_task)
	}
	.bss :
	{
		_bss = .;
		*(.bss)
		_ebss = .;
	}
	_end = .;
}