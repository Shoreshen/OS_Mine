#include "keyboard.h"
#include "Mem.h"
#include "Task.h"

wait_queue_T keyboard_queue_head;

#pragma region keyboard
void keyboard_init(void)
{
    rte RTE;
    unsigned long i;
	
	wait_queue_init(&keyboard_queue_head, NULL);

    //Reset keyboard buffer
    Key_Buffer.p_head = &Key_Buffer.buf[0];
    Key_Buffer.p_tail = &Key_Buffer.buf[0];
    Key_Buffer.count = 0;
    memset(&Key_Buffer.buf[0], 0, KB_BUF_SIZE);

    //Setting RTE entry
    RTE.vector                          = 0x21;
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

    //Setting 8042 controller for keyboard
    wait_KB_write();
	io_out8(PORT_KB_CMD, KBCMD_WRITE_CMD);
	wait_KB_write();
	io_out8(PORT_KB_DATA, KB_INIT_MODE);
    for(i=0;i<1000000;i++){
        nop;
    }

    //Register IRQ
    register_irq(0x21, &RTE , &keyboard_handler, (unsigned long)0, &keyboard_int_controller, "ps/2 keyboard");
}
// void wakeup(wait_queue_T *queue_head, unsigned long state)
// {
// 	wait_queue_T *wait = NULL;

// 	if(List_is_empty(&queue_head->List)){
// 		color_printk(RED,BLACK,"list is empty\n");
// 		return;
// 	}

// 	wait = er_of(queue_head->List.next, wait_queue_T, List);
// 	if(wait->tsk->state & state){
// 		List_Del(&wait->List);
// 		wait->tsk->state = TASK_RUNNING;
// 		insert_task_queue(wait->tsk);
// 		current->flag |= NEED_SCHEDULE;
// 	}
// }
void keyboard_handler(unsigned long nr, unsigned long parameter)
{
	unsigned char x;
	x = io_in8(0x60);
	// color_printk(WHITE,BLACK,"(K:%02x)",x);
	if(Key_Buffer.p_head == &Key_Buffer.buf[0] + KB_BUF_SIZE){
        Key_Buffer.p_head = &Key_Buffer.buf[0];
    }

	*Key_Buffer.p_head = x;
	Key_Buffer.count++;
	Key_Buffer.p_head++;

	wakeup(&keyboard_queue_head, TASK_UNINTERRUPTIBLE);
}

unsigned char get_scancode(void)
{
	unsigned char ret  = 0;

	if(Key_Buffer.count == 0){
        while(!Key_Buffer.count){
            nop;
        }
    }
	if(Key_Buffer.p_tail == &Key_Buffer.buf[0] + KB_BUF_SIZE){
        Key_Buffer.p_tail = &Key_Buffer.buf[0];
    }

	ret = *Key_Buffer.p_tail;
	Key_Buffer.count--;
	Key_Buffer.p_tail++;

	return ret;
}

void analysis_keycode(void)
{
    unsigned char x = 0;
    int i, key = 0, make = 0;
    x = get_scancode();
    if(x == 0xE1){
		key = PAUSEBREAK;
		for(i = 1;i<6;i++){
			if(get_scancode() != pausebreak_scode[i])
			{
				key = 0;
				break;
			}
        }
    }
    else if(x == 0xE0){
        x = get_scancode();
		switch(x)
		{
			case 0x2A: // press printscreen
				if(get_scancode() == 0xE0)
					if(get_scancode() == 0x37)
					{
						key = PRINTSCREEN;
						make = 1;
					}
				break;
			case 0xB7: // UNpress printscreen
				if(get_scancode() == 0xE0)
					if(get_scancode() == 0xAA)
					{
						key = PRINTSCREEN;
						make = 0;
					}
				break;
			case 0x1d: // press right ctrl
				Key_Buffer.ctrl_r = 1;
				key = OTHERKEY;
				break;
			case 0x9d: // UNpress right ctrl
				Key_Buffer.ctrl_r = 0;
				key = OTHERKEY;
				break;
			case 0x38: // press right alt
				Key_Buffer.alt_r = 1;
				key = OTHERKEY;
				break;
			case 0xb8: // UNpress right alt
				Key_Buffer.alt_r = 0;
				key = OTHERKEY;
				break;		
			default:
				key = OTHERKEY;
				break;
		}
    }
    if(key == 0){
		if(x & 0x80){//Bit 7 of all scan code of break is 1b
			make = 0;
		}else{
			make = 1;	
		}
			
		if(Key_Buffer.shift_l || Key_Buffer.shift_r){
			key = keycode_map_normal[(x & 0x7f)][1];
		}else{
			key = keycode_map_normal[(x & 0x7f)][0];
		}
		switch(x & 0x7F)
		{
			case 0x2a:	//SHIFT_L:
				Key_Buffer.shift_l = make;
				key = 0;
				break;

			case 0x36:	//SHIFT_R:
				Key_Buffer.shift_r = make;
				key = 0;
				break;

			case 0x1d:	//CTRL_L:
				Key_Buffer.ctrl_l = make;
				key = 0;
				break;

			case 0x38:	//ALT_L:
				Key_Buffer.alt_l = make;
				key = 0;
				break;

			default:
				if(!make){
					key = 0;
				}
				break;
		}
		
		if(key){
			color_printk(RED,BLACK,"(K:%c)",key);
		}
    }
}
#pragma endregion

#pragma region keyboard_ops
long keyboard_open(FS_entry* entry)
{
	entry->type.file.Data = Key_Buffer.buf;
	Key_Buffer.p_head = Key_Buffer.buf;
	Key_Buffer.p_tail = Key_Buffer.buf;
	Key_Buffer.count = 0;
	memset(Key_Buffer.buf, 0, KB_BUF_SIZE);
	return 1;
}
long keyboard_close(FS_entry* entry)
{
	entry->type.file.Data = NULL;
	Key_Buffer.p_head = Key_Buffer.buf;
	Key_Buffer.p_tail = Key_Buffer.buf;
	Key_Buffer.count = 0;
	memset(Key_Buffer.buf, 0, KB_BUF_SIZE);
	return 1;
}
long keyboard_ioctl(unsigned long cmd)
{
	switch(cmd){
		case KEY_CMD_RESET_BUFFER:
			Key_Buffer.p_head = Key_Buffer.buf;
			Key_Buffer.p_tail = Key_Buffer.buf;
			Key_Buffer.count = 0;
			memset(Key_Buffer.buf, 0, KB_BUF_SIZE);
			break;
		default:
			break;
	}
}
long keyboard_read(FS_entry *entry, void *buf, unsigned long count)
{
	unsigned long buf_res = 0, counter = 0;

	if(glb_ind){
		color_printk(GREEN, BLACK,"here\n");
	}

	if(Key_Buffer.count == 0){
		sleep(&keyboard_queue_head);
	}

	buf_res = (&Key_Buffer.buf[0]+KB_BUF_SIZE-Key_Buffer.p_tail);
	counter = Key_Buffer.count;
	if(counter>count){
		counter = count;
	}

	if(counter <= buf_res) {
		memcpy(Key_Buffer.p_tail, buf, counter);
		Key_Buffer.p_tail += counter;
	}else{
		memcpy(Key_Buffer.p_tail, buf, buf_res);
		buf += buf_res;
		memcpy(&Key_Buffer.buf[0], buf, counter - buf_res);
		Key_Buffer.p_tail = &Key_Buffer.buf[0] + counter - buf_res;
	}
	Key_Buffer.count -= counter;
}
long keyboard_write(FS_entry *entry, void *buf, unsigned long count)
{
	color_printk(INDIGO,BLACK,"keyboard_write here\n");
	return 0;
}
#pragma endregion

#pragma region mouse
void mouse_init(void)
{
	rte RTE;
	unsigned long i;
	Mouse.mouse_count = 0;

	Mouse_Buffer.p_head = &Mouse_Buffer.buf[0];
	Mouse_Buffer.p_tail = &Mouse_Buffer.buf[0];
	Mouse_Buffer.count  = 0;
	memset(&Mouse_Buffer.buf[0], 0, KB_BUF_SIZE);
	//Setting RTEs
	RTE.vector 							= 0x2c;
	RTE.deliver_mode 					= APIC_DEL_MODE_Fixed;
	RTE.dest_mode 						= APIC_DEST_MODE_PHYSICAL;
	RTE.deliver_status 					= APIC_DEL_STATU_Idle;
	RTE.polarity 						= APIC_POLARITY_HIGH;
	RTE.irr 							= APIC_IRR_RESET;
	RTE.trigger 						= APIC_TIG_MODE_Edge;
	RTE.mask 							= APIC_MASK_UNSET;
	RTE.reserved 						= 0;
	RTE.destination.physical.phy_dest 	= 0;
	RTE.destination.physical.reserved1 	= 0;
	RTE.destination.physical.reserved2 	= 0;
	//Register interrupt
	register_irq(0x2c, &RTE , &mouse_handler, (unsigned long)0, &mouse_int_controller, "ps/2 mouse");
	//Setting 8042 controller for mouse
	wait_KB_write();
	io_out8(PORT_KB_CMD,KBCMD_EN_MOUSE_INTFACE);
    for(i=0;i<1000000;i++){
        nop;
    }
	wait_KB_write();
	io_out8(PORT_KB_CMD,KBCMD_SENDTO_MOUSE);
	wait_KB_write();
	io_out8(PORT_KB_DATA,MOUSE_ENABLE);
    for(i=0;i<1000000;i++){
        nop;
    }
	wait_KB_write();
	io_out8(PORT_KB_CMD,KBCMD_WRITE_CMD);
	wait_KB_write();
	io_out8(PORT_KB_DATA,KB_INIT_MODE);
}

void mouse_handler(unsigned long nr, unsigned long parameter)
{
	unsigned char x;
	x = io_in8(PORT_KB_DATA);
	//color_printk(GREEN,WHITE,"(M:%02x)",x);

	if(Mouse_Buffer.p_head == &Mouse_Buffer.buf[0] + KB_BUF_SIZE){
		Mouse_Buffer.p_head = &Mouse_Buffer.buf[0];
	}

	*Mouse_Buffer.p_head = x;
	Mouse_Buffer.count++;
	Mouse_Buffer.p_head ++;
}

unsigned char get_mousecode(void)
{
	unsigned char ret  = 0;

	if(Mouse_Buffer.count == 0){
		while(!Mouse_Buffer.count){
			nop;
		}
	}
	
	if(Mouse_Buffer.p_tail == &Mouse_Buffer.buf[0] + KB_BUF_SIZE){
		Mouse_Buffer.p_tail = &Mouse_Buffer.buf[0];
	}

	ret = *Mouse_Buffer.p_tail;
	Mouse_Buffer.count--;
	Mouse_Buffer.p_tail++;

	return ret;
}
void analysis_mousecode(void)
{
	unsigned char x = get_mousecode();

	switch(Mouse.mouse_count)
	{
		case 0:
			Mouse.mouse_count++;
			break;

		case 1:
			Mouse.Byte0 = x;
			Mouse.mouse_count++;
			break;
		
		case 2:
			Mouse.Byte1 = (char)x;
			Mouse.mouse_count++;
			break;

		case 3:
			Mouse.Byte2 = (char)x;
			Mouse.mouse_count = 1;
			color_printk(RED,GREEN,"(M:%02x,X:%3d,Y:%3d)\n",Mouse.Byte0,Mouse.Byte1,Mouse.Byte2);
			break;

		default:			
			break;
	}
}
#pragma endregion

void Disk_Init(void)
{
	if (AHCI.ABAR) {
		color_printk(WHITE, ORANGE, "Initialize AHCI mode.\n");
		AHCI_Disk_Init();
		memset(&AHCI_queue, 0, sizeof(AHCI_queue));
		List_Init(&AHCI_queue.Queue_List);
	} else {
		color_printk(WHITE, ORANGE, "Initialize IDE mode.\n");
		IDE_Disk_Init();
	}
}

#pragma region AHCI
void AHCI_Disk_Init(void)
{
	int i,j;
	pg_attr ATTR;
	HBA_CMD_TBL * ctbalist;
	HBA_CMD_HEADER * clblist;

    memset(&ATTR, 0, sizeof(pg_attr));
    ATTR.PML4E_Attr.RW 	= 1;
    ATTR.PML4E_Attr.P	= 1;
    ATTR.PDPTE_Attr.P	= 1;
    ATTR.PDPTE_Attr.RW	= 1;
    ATTR.PDE_Attr.PS	= 1;
    ATTR.PDE_Attr.P		= 1;
    ATTR.PDE_Attr.RW	= 1;
    //Choose PAT[3]: Uncacheable memory type 
    ATTR.PDE_Attr.PWT   = 1;
    ATTR.PDE_Attr.PCD   = 1;

	pagetable_init(PML4E, 
		((unsigned long)AHCI.ABAR - PAGE_OFFSET) & PAGE_2M_MASK, 
		((unsigned long)AHCI.ABAR & PAGE_2M_MASK), 1, 
	&ATTR, 0, 1);

	AHCI.ABAR->ghc = AHCI.ABAR->ghc | SATA_GHC_AE;//Set ACHI Enable
	AHCI.No_slot = (AHCI.ABAR->cap >> 8) & 0x1f;

	for(i=0;i<32;i++){
		if(AHCI.ABAR->pi & 1<<i){
			if(
				(AHCI.ABAR->ports[i].ssts & 0xf) == 3 && 			//Device presence detected and Phy communication established
				((AHCI.ABAR->ports[i].ssts >> 8) & 0x0F) == 1 &&	//Interface in active state
				AHCI.ABAR->ports[i].sig == SATA_SIG_ATA				//Is SATA drive
			){
				AHCI.ABAR->ports[i].cmd &= ~(HBA_PxCMD_ST | HBA_PxCMD_FRE);
				while(1)
				{
					if (AHCI.ABAR->ports[i].cmd & HBA_PxCMD_FR){
						continue;
					}
					if (AHCI.ABAR->ports[i].cmd & HBA_PxCMD_CR){
						continue;
					}
					break;
				}
				ctbalist = kmalloc(sizeof(HBA_CMD_TBL) * AHCI.No_slot, 0);
				clblist  = kmalloc(sizeof(HBA_CMD_HEADER)*AHCI.No_slot, 0);
				AHCI.ABAR->ports[i].fb  = kmalloc(sizeof(HBA_FIS),0) - PAGE_OFFSET;
				
				memset(clblist, 0, sizeof(HBA_CMD_HEADER)*AHCI.No_slot);
				memset(ctbalist, 0, sizeof(HBA_CMD_TBL) * AHCI.No_slot);
				memset((void *)((unsigned long)AHCI.ABAR->ports[i].fb  + PAGE_OFFSET), 0, sizeof(HBA_FIS));
				
				AHCI.ABAR->ports[i].clb = (HBA_CMD_HEADER *)((unsigned long)clblist - PAGE_OFFSET);
				for(j = 0;j < AHCI.No_slot;j++){
					clblist[j].ctba	 = (HBA_CMD_TBL *)((unsigned long)&ctbalist[j] - PAGE_OFFSET);
					clblist[j].cfl 	 = sizeof(FIS_REG_H2D)/sizeof(int);
					clblist[j].prdtl = 1;
					clblist[j].p	 = 1;
				}
				AHCI.ABAR->ports[i].cmd |= HBA_PxCMD_FRE;
				AHCI.ABAR->ports[i].cmd |= HBA_PxCMD_ST;
				AHCI.ABAR->ports[i].ie	|= 1;
				// color_printk(WHITE, ORANGE, "AHCI.ABAR->ports[%d].cmd:  0x%08X\n", i, AHCI.ABAR->ports[i].cmd);
				// color_printk(WHITE, ORANGE, "AHCI.ABAR->ports[%d].stss: 0x%08X\n", i, AHCI.ABAR->ports[i].ssts);
				// color_printk(WHITE, ORANGE, "AHCI.ABAR->ports[%d].sig:  0x%08X\n", i, AHCI.ABAR->ports[i].sig);
				// color_printk(WHITE, ORANGE, "AHCI.ABAR->ports[%d].tfd:  0x%08X\n", i, AHCI.ABAR->ports[i].tfd);
			}
		}
	}
	AHCI.ABAR->ghc |= SATA_GHC_IE;
	AHCI.MSI._64->MAR.Pre_fix  = 0xFEE;	// Fixed head address
	AHCI.MSI._64->MAR.Dest_ID  = 0;		// Send to APIC ID = 0
	AHCI.MSI._64->MAR.zero     = 0;		
	AHCI.MSI._64->MAR.DM       = 0;		// Pysical destination mode
	AHCI.MSI._64->MAR.RH       = 0;		// Interrupt direcet to processor in MAR.Dest_ID field
	AHCI.MSI._64->MDR.Del_Mode = 0; 	// Fixed mode
	AHCI.MSI._64->MDR.Vector   = 0x2e;	// IDT[0x2E]
	AHCI.MSI._64->MDR.Trg_Mode = 0;		// Edge trigger
	AHCI.MSI._64->MDR.Level	   = 0; 	// No use for edge triggered interrupt
	AHCI.MSI._64->MDR.zero	   = 0;
	AHCI.MSI._64->MCR.MSI_Enable = 1; 	// Enable MSI

	interrupt_desc[AHCI.MSI._64->MDR.Vector - 32].controller = &disk_int_controller;
	interrupt_desc[AHCI.MSI._64->MDR.Vector - 32].handler = AHCI_handler;
}

void AHCI_REQ(int lba, int NoSec, void *Buf, char cmd, unsigned long schedual)
{
	req_node *node;

	if(NoSec > 2048){
		color_printk(WHITE,RED,"Maximum 2048 sector but required %d sectors!\n", node->COUNT);
	}else{
		node = kmalloc(sizeof(req_node), 0);
		memset(node,0,sizeof(req_node));
		node->CMD = cmd;
		node->COUNT = NoSec;
		node->LBA = lba;
		node->phy_buffer = (unsigned char*)Virt_To_Phy((unsigned long)Buf);
		node->tsk = current;
		node->schedual = schedual;
		List_Init(&node->List);

		List_Insert_Before(&AHCI_queue.Queue_List,&node->List);
		AHCI_queue.node_count++;
		if(AHCI_queue.in_using == NULL){
			node->slot = AHCI_PostCMD();
		}
		if(AHCI_queue.in_using->schedual){
			current->state = TASK_UNINTERRUPTIBLE;
			schedule();			
		}else{
			while(AHCI.ABAR->ports[AHCI.trg_port].ci & 1<<AHCI_queue.in_using->slot);
		}
	}
}

int AHCI_PostCMD(void)
{
	int i, j, slot = 0;
	req_node 		*node;
	HBA_CMD_TBL 	*cmdtbl;
	HBA_CMD_HEADER 	*cmdheader;
	FIS_REG_H2D 	*cmdfis;

	node = container_of(AHCI_queue.Queue_List.next, req_node, List);
	List_Del(&node->List);
	AHCI_queue.in_using = node;
	AHCI_queue.node_count--;

	AHCI.ABAR->ports[AHCI.trg_port].is = (unsigned int) - 1;
	slot = AHCI.ABAR->ports[AHCI.trg_port].sact | AHCI.ABAR->ports[AHCI.trg_port].ci;
	for(i =0; i<AHCI.No_slot; i++){
		if(!(slot&1)){
			break;
		}
		slot>>=1;
	}
	if(i==AHCI.No_slot){
		color_printk(WHITE,RED,"All slot for port[%d] are busy!\n", AHCI.trg_port);
		return -1;
	}
	cmdheader 	= (HBA_CMD_HEADER *)((unsigned long)AHCI.ABAR->ports[AHCI.trg_port].clb + PAGE_OFFSET);
	cmdtbl 		= (HBA_CMD_TBL *)((unsigned long)cmdheader[i].ctba + PAGE_OFFSET);
	cmdfis 		= (FIS_REG_H2D *)(&cmdtbl->cfis);

	cmdheader->prdbc = 0;
	memset(cmdtbl, 0, sizeof(HBA_CMD_TBL));

	cmdtbl->prdt_entry[0].dba = (unsigned long)node->phy_buffer;
	cmdtbl->prdt_entry[0].dbc = 512 * node->COUNT - 1;
	// cmdtbl->prdt_entry[0].i	  = 1;

	cmdfis->fis_type 	= FIS_TYPE_REG_H2D;
	cmdfis->c			= 1;	// Command
	cmdfis->lbal 		= node->LBA & 0xFFFFFF;
	cmdfis->lbah 		= node->LBA >> 24;
	cmdfis->device 		= 1<<6;	// LBA mode
	cmdfis->count 		= node->COUNT;

	cmdfis->command = node->CMD;

	if(cmdfis->command == ATA_CMD_WRITE_DMA_EX){
		cmdheader[i].w  = 1;
	} else{
		cmdheader[i].w  = 0;
	}

	while(AHCI.ABAR->ports[AHCI.trg_port].tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ));
	AHCI.ABAR->ports[AHCI.trg_port].ci |= 1<<i;
	return i;
}

void AHCI_handler(unsigned long nr, unsigned long parameter)
{
	if(AHCI_queue.in_using->schedual){
		AHCI_queue.in_using->tsk->state = TASK_RUNNING;
		insert_task_queue(AHCI_queue.in_using->tsk);
		current->flag |= NEED_SCHEDULE; //INT.c->IRQ0x20: Only will trigger schedual when NEED_SCHEDULE flag is set for current process
										//This indicate that disk has already passed data into mem, 
										//need to trigger schedual and switch into relavent thread to deal with the data
		// color_printk(BLACK,RED,"Disk operate done:%d\n",current->pid);
	}
	// else{
	// 	color_printk(BLACK,RED,"Disk operate done\n");
	// }
	kfree(AHCI_queue.in_using);
	AHCI_queue.in_using = NULL;	

	AHCI.ABAR->ports[AHCI.trg_port].is = (unsigned int) - 1;
	AHCI.ABAR->is |= 1<<AHCI.trg_port;
	if(AHCI_queue.node_count){
		AHCI_PostCMD();
	}
}
#pragma endregion

#pragma region IDE
void IDE_Disk_Init(void)
{
	rte RTE;
	unsigned char a[512];

	RTE.vector 							= 0x2e;
    RTE.deliver_mode                    = APIC_DEL_MODE_Fixed;
    RTE.dest_mode                       = APIC_DEST_MODE_PHYSICAL;
    RTE.deliver_status                  = APIC_DEL_STATU_Idle;
    RTE.polarity                        = APIC_POLARITY_HIGH;
    RTE.irr                             = APIC_IRR_RESET;
    RTE.trigger                         = APIC_TIG_MODE_Edge;
    RTE.mask                            = APIC_MASK_SET;
    RTE.reserved                        = 0;

    RTE.destination.physical.phy_dest   = 0;
    RTE.destination.physical.reserved1  = 0;
    RTE.destination.physical.reserved2  = 0;

    //Register IRQ
    register_irq(0x2e, &RTE , &disk_handler, (unsigned long)&Disk_request, &disk_int_controller, "disk0");
	
	io_out8(PORT_DISK0_ALT_STA_CTL,0);

	Disk_request.block_request_count = 0;
	Disk_request.in_using = NULL;
	List_Init(&Disk_request.Queue_List);
}

void disk_handler(unsigned long nr, unsigned long parameter)
{
	Disk_request.in_using->end_handler(nr, parameter);
}

void Disk_End_REQ(void)
{
	kfree(Disk_request.in_using);
	if(Disk_request.in_using->Ind){
		*(Disk_request.in_using->Ind) = 0;
	}

	Disk_request.in_using = NULL;

	if(Disk_request.block_request_count){
		Disk_Exec_REQ();
	}
}

void Disk_Post_REQ(unsigned char cmd, unsigned long LBA, unsigned int count, unsigned char *buffer, unsigned long *Ind)
{
	block_buffer_node *node = kmalloc(sizeof(block_buffer_node), 0);
 
	node->cmd 		= cmd;
	node->LBA 		= LBA;
	node->count 	= count;
	node->buffer 	= buffer;
	node->Ind		= Ind;

	switch (node->cmd)
	{
	case ATA_READ_CMD:
		node->end_handler = Disk_Read_handler;
		break;
	case ATA_WRITE_CMD:
		node->end_handler = Disk_Write_handler;
		break;
	case GET_IDENTIFY_DISK_CMD:
		node->end_handler = Disk_ConfInfo_handler;
		break;
	default:
		color_printk(BLACK,WHITE,"ATA CMD Error\n");
		break;
	}

	List_Init(&node->List);
	List_Insert_Before(&Disk_request.Queue_List, &node->List);
	Disk_request.block_request_count++;

	if(Disk_request.in_using == NULL){
		Disk_Exec_REQ();
	}
}

void Disk_Exec_REQ(void)
{
	Disk_request.in_using = container_of(Disk_request.Queue_List.next, block_buffer_node, List);
	List_Del(&Disk_request.in_using->List);
	Disk_request.block_request_count--;
	int i;

	while(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_BUSY);

	switch (Disk_request.in_using->cmd)
	{
	case ATA_READ_CMD:
		io_out8(PORT_DISK0_DEVICE,0x40);
		//LBA[24:47], Count[8:15];
		io_out8(PORT_DISK0_ERR_FEATURE,0);
		io_out8(PORT_DISK0_SECTOR_CNT,	(Disk_request.in_using->count >> 8) & 0xff);
		io_out8(PORT_DISK0_SECTOR_LOW,	(Disk_request.in_using->LBA >> 24) & 0xff);
		io_out8(PORT_DISK0_SECTOR_MID,	(Disk_request.in_using->LBA >> 32) & 0xff);
		io_out8(PORT_DISK0_SECTOR_HIGH,	(Disk_request.in_using->LBA >> 40) & 0xff);
		//LBA[0:23], Count[0:7];
		io_out8(PORT_DISK0_ERR_FEATURE,0);
		io_out8(PORT_DISK0_SECTOR_CNT,	Disk_request.in_using->count & 0xff);
		io_out8(PORT_DISK0_SECTOR_LOW,	Disk_request.in_using->LBA & 0xff);
		io_out8(PORT_DISK0_SECTOR_MID,	(Disk_request.in_using->LBA >> 8) & 0xff);
		io_out8(PORT_DISK0_SECTOR_HIGH,	(Disk_request.in_using->LBA >> 16) & 0xff);

		while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_READY));

		io_out8(PORT_DISK0_STATUS_CMD,Disk_request.in_using->cmd);
		break;
	case ATA_WRITE_CMD:
		io_out8(PORT_DISK0_DEVICE,0x40);

		io_out8(PORT_DISK0_ERR_FEATURE,	0);
		io_out8(PORT_DISK0_SECTOR_CNT,	(Disk_request.in_using->count >> 8) & 0xff);
		io_out8(PORT_DISK0_SECTOR_LOW,	(Disk_request.in_using->LBA >> 24) & 0xff);
		io_out8(PORT_DISK0_SECTOR_MID,	(Disk_request.in_using->LBA >> 32) & 0xff);
		io_out8(PORT_DISK0_SECTOR_HIGH,	(Disk_request.in_using->LBA >> 40) & 0xff);

		io_out8(PORT_DISK0_ERR_FEATURE,	0);
		io_out8(PORT_DISK0_SECTOR_CNT,	Disk_request.in_using->count & 0xff);
		io_out8(PORT_DISK0_SECTOR_LOW,	Disk_request.in_using->LBA & 0xff);
		io_out8(PORT_DISK0_SECTOR_MID,	(Disk_request.in_using->LBA >> 8) & 0xff);
		io_out8(PORT_DISK0_SECTOR_HIGH,	(Disk_request.in_using->LBA >> 16) & 0xff);

		while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_READY));

		io_out8(PORT_DISK0_STATUS_CMD,Disk_request.in_using->cmd);

		while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_REQ));

		port_outsw(PORT_DISK0_DATA, Disk_request.in_using->buffer, 256 * Disk_request.in_using->count);

		break;
	case GET_IDENTIFY_DISK_CMD:
		io_out8(PORT_DISK0_DEVICE, 0xe0);
		
		io_out8(PORT_DISK0_ERR_FEATURE,0);
		io_out8(PORT_DISK0_SECTOR_CNT,	Disk_request.in_using->count & 0xff);
		io_out8(PORT_DISK0_SECTOR_LOW,	Disk_request.in_using->LBA & 0xff);
		io_out8(PORT_DISK0_SECTOR_MID,	(Disk_request.in_using->LBA >> 8) & 0xff);
		io_out8(PORT_DISK0_SECTOR_HIGH,	(Disk_request.in_using->LBA >> 16) & 0xff);

		while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_READY));

		io_out8(PORT_DISK0_STATUS_CMD,Disk_request.in_using->cmd);
		break;
	default:
		color_printk(BLACK,WHITE,"ATA CMD Error\n");
		break;
	}
}
void Disk_Read_handler(unsigned long nr, unsigned long parameter)
{
	if(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_ERROR){
		color_printk(RED,BLACK,"read_handler:%#010x\n",io_in8(PORT_DISK0_ERR_FEATURE));
	}
	else{
		port_insw(PORT_DISK0_DATA, Disk_request.in_using->buffer, 256 * Disk_request.in_using->count);
	}

	Disk_End_REQ();
}
void Disk_Write_handler(unsigned long nr, unsigned long parameter)
{
	if(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_ERROR){
		color_printk(RED,BLACK,"write_handler:%#010x\n",io_in8(PORT_DISK0_ERR_FEATURE));
	}

	Disk_End_REQ();
}
void Disk_ConfInfo_handler(unsigned long nr, unsigned long parameter)
{
	if(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_ERROR){
		color_printk(RED,BLACK,"other_handler:%#010x\n",io_in8(PORT_DISK0_ERR_FEATURE));
	}
	else{
		port_insw(PORT_DISK0_DATA, Disk_request.in_using->buffer, 256);
	}

	Disk_End_REQ();
}
#pragma endregion

#pragma region NVME
nvme NVME;
void NVME_handler(unsigned long nr, unsigned long parameter)
{
	color_printk(YELLOW,BLACK,"NVME Int\n");
}
void NVME_MAP(void)
{
	pg_attr ATTR;
	mbar tmp; //Have to write in dword
    
	memset(&ATTR, 0, sizeof(pg_attr));
    ATTR.PML4E_Attr.RW 	= 1;
    ATTR.PML4E_Attr.P	= 1;
    ATTR.PDPTE_Attr.P	= 1;
    ATTR.PDPTE_Attr.RW	= 1;
    ATTR.PDE_Attr.PS	= 1;
    ATTR.PDE_Attr.P		= 1;
    ATTR.PDE_Attr.RW	= 1;
    //Choose PAT[3]: Uncacheable memory type 
    ATTR.PDE_Attr.PWT   = 1;
    ATTR.PDE_Attr.PCD   = 1;

	pagetable_init(PML4E, 
		((unsigned long)NVME.MBAR - PAGE_OFFSET) & PAGE_2M_MASK, 
		((unsigned long)NVME.MBAR & PAGE_2M_MASK), 1, 
	&ATTR, 0, 1);
	tmp = *NVME.MBAR;
	color_printk(PURPLE,BLACK,"en:%d, cfs:%d, rdy:%d, iocqes:%d, iosqes:%d,meqs:%d,mps:%d,mpsmax:%d,mpsmin;%d\n",
		NVME.MBAR->cc.en,
		NVME.MBAR->csts.cfs,
		NVME.MBAR->csts.rdy,
		NVME.MBAR->cc.iocqes,
		NVME.MBAR->cc.iosqes,
		NVME.MBAR->cap.mqes,
		NVME.MBAR->cc.mps,
		NVME.MBAR->cap.mpsmax,
		NVME.MBAR->cap.mpsmin
	);

	tmp.cc.en = 0;
	NVME.MBAR->cc = tmp.cc; //Reset NVME controller
	while(NVME.MBAR->csts.rdy); //Wait until finish resetting

	tmp.aqa.acqs = 16;
	tmp.aqa.asqs = 16;
	tmp.asq = Virt_To_Phy((unsigned long)kmalloc(0x1000,0));
	memset((void *)tmp.asq,0,0x1000);
	tmp.acq = Virt_To_Phy((unsigned long)kmalloc(0x1000,0));
	memset((void *)tmp.acq,0,0x1000);
	tmp.cc.ams = 0;    //Set Round Robin arbitration mechanism (仲裁机制-优先级)
	tmp.cc.mps = 0;    //Set mem page size as 2^(12+0)=4KB, only value passes reset
	tmp.cc.css = 0;	   //Select NVM Command Set
	tmp.cc.iocqes = 4; //Only value pass qemu reset check
	tmp.cc.iosqes = 6; //Only value pass qemu reset check
	tmp.cc.en = 1;

	NVME.MBAR->aqa = tmp.aqa;
	NVME.MBAR->asq = tmp.asq;
	NVME.MBAR->acq = tmp.acq;
	NVME.MBAR->cc = tmp.cc;
	while(!NVME.MBAR->csts.rdy); //Wait until finish starting
}
void NVME_MSIX(void)
{
	int i;
	mcrx t_mcr;
	mdr t_mdr;
	mar t_mar; 

	for(i = 0; i < min(NR_CPUS,NVME.MSIX->Tbl_Size); i++){
		//Admin queue correspond to vector 0
		t_mcr.Mask = 0;
		t_mcr.Rsvd = 0;

		t_mar.Pre_fix  = 0xFEE;		// Fixed head address
		t_mar.Dest_ID  = i;			// Send to APIC ID = i, different cpu core
		t_mar.zero     = 0;		
		t_mar.DM       = 0;			// Pysical destination mode
		t_mar.RH       = 0;			// Interrupt direcet to processor in MAR.Dest_ID field
		t_mdr.Del_Mode = 0; 		// Fixed mode
		t_mdr.Vector   = 0x2f + i;	// IDT[0x2F]
		t_mdr.Trg_Mode = 0;			// Edge trigger
		t_mdr.Level	 = 0; 			// No use for edge triggered interrupt
		t_mdr.zero	 = 0;

		interrupt_desc[t_mdr.Vector - 32].controller = &disk_int_controller;
		interrupt_desc[t_mdr.Vector - 32].handler = NVME_handler;
		// All MSI-X table support DWORD write only
		NVME.MSIXTBL[i].MAR_up = 0;
		NVME.MSIXTBL[i].MAR = *(unsigned int*)&t_mar;
		NVME.MSIXTBL[i].MDR = *(unsigned int*)&t_mdr;
		if(i==0){
			NVME.MSIXTBL[i].MCRX = *(unsigned int*)&t_mcr;
		}

		// color_printk(WHITE,BLACK,"MCR[%d]: 0x%08X, MAR[%d]: 0x%08X,MDR[%d]: 0x%08X\n",
		// 	i, *(unsigned int *)&NVME.MSIXTBL[i].MCRX,
		// 	i, *(unsigned int *)&NVME.MSIXTBL[i].MAR,
		// 	i, *(unsigned int *)&NVME.MSIXTBL[i].MDR
		// );
	}
	NVME.MSIX->Enable = 1;
}
void NVME_Disk_Init(void)
{
	int i;
	
	NVME_MAP();
	NVME_MSIX();
	//Wait for previous reset is complete
	color_printk(PURPLE,BLACK,"asqs:%d,acqs:%d,asq:0x%X,acq:0x%X,ams:%d,mps:%d,css:%d,en:%d,rdy:%d\n",
		NVME.MBAR->aqa.asqs,
		NVME.MBAR->aqa.acqs,
		NVME.MBAR->asq,
		NVME.MBAR->acq,
		NVME.MBAR->cc.ams,
		NVME.MBAR->cc.mps,
		NVME.MBAR->cc.css,
		NVME.MBAR->cc.en,
		NVME.MBAR->csts.rdy
	);
	color_printk(PURPLE,BLACK,"Cap ID: 0x%02X, MSIX BAR: %p, MSIX OFF: %p, MSIX SIZE: %p, MSI Enable: %d, MSIX TBL: %p\n",
		NVME.MSIX->Cap_ID,
		NVME.MSIX->TlbBIR,
		NVME.MSIX->TblOff,
		NVME.MSIX->Tbl_Size,
		NVME.MSIX->Enable,
		NVME.MSIXTBL
	);	
	
	

}
#pragma endregion