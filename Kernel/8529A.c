#include "./g_reg_only/INT.h"

void Init_8259A(void)
{
    int i;

	for(i = 32;i < 56;i++)
	{
		_Set_INT(IDT_PTR.Offset + i, ATTR_INTR_GATE, 2, interrupt[i - 32]);
	}
    color_printk(RED,BLACK,"8259A init \n");

    //8259A-master	ICW1-4
	io_out8(0x20,0x11);
        //Port 0x20(Master ICW1):
            //bit 0: 1b: use ICW4
            //bit 1: 0b: cascade 8529A
            //bit 4: 1b: must be 1
            //Others: 0
	io_out8(0x21,0x20);
        //Port 0x21(Master ICW2~4):
            //bit 0~2: 0b: Must be 0
            //bit 3~7: 100b: 100b << 3 = 0x20, IR0 mapping INT[0x20], IR1 mapping INT[0x21], etc
	io_out8(0x21,0x04);
        //Port 0x21(Master ICW2~4):
            //bit 0~7: 00000100b: IR2 connect to slave 8529A 
	io_out8(0x21,0x01);
        //Port 0x21(Master ICW2~4):
            //bit 0: 1b: 8086 mode(0 = MCS 80/85 mode)
            //bit 1: 0b: EOI mode(1 = AEOI mode): EOI mode clear ISR when EOI sent, AEOI mode clear ISR after second INTA sent from CPU
            //bit 2~3: 0b: No buffer
            //bit 4: 0b: SFNM mode (1b = FNM mode): 
                //FNM mode: orders priority from IR0~7, block slave when master IR is under taken
                //SFNM mode: orders priority from IR0~7, does not block slave

	//8259A-slave	ICW1-4
	io_out8(0xa0,0x11);
        //Port 0xa0(Slave ICW1):
            //bit 0: 1b: use ICW4
            //bit 1: 0b: cascade 8529A
            //bit 4: 1b: must be 1
            //Others: 0
	io_out8(0xa1,0x28);
        //Port 0x21(Slave ICW2~4):
            //bit 0~2: 0b: Must be 0
            //bit 3~7: 100b: 101b << 3 = 0x28, IR0 mapping INT[0x28], IR1 mapping INT[0x29], etc
	io_out8(0xa1,0x02);
        //Port 0x21(Slave ICW2~4):
            //bit 0~2: 10b: 10b = 2, slave 8529A is connected to IR2 in master 8529A
            //bit 3~7: 0b: Must be 0
	io_out8(0xa1,0x01);
        //Port 0x21(Slave ICW2~4):
            //bit 0: 1b: 8086 mode(0 = MCS 80/85 mode)
            //bit 1: 0b: EOI mode(1 = AEOI mode): EOI mode clear ISR when EOI sent, AEOI mode clear ISR after second INTA sent from CPU
            //bit 2~3: 0b: No buffer
            //bit 4: 0b: SFNM mode (1b = FNM mode): 
                //FNM mode: orders priority from IR0~7, block slave when master IR is under taken
                //SFNM mode: orders priority from IR0~7, does not block slave

	//8259A-M/S	OCW1
	io_out8(0x21,0xfd);
        //bit 0~7: 11111101b: Only not mask IR1: keyboard 
	io_out8(0xa1,0xff);
        //bit 0~7: 11111111b: Mask all IR 
    sti;

    return;
}