#include "lib.h"

#pragma region keyboard & Mouse
#define PAUSEBREAK	1
#define PRINTSCREEN	2
#define OTHERKEY	4
#define FLAG_BREAK	0x80

#define PORT_KB_CMD	    0x64
#define PORT_KB_DATA	0x60
#define PORT_KB_STATUS	0x64
    //Read port 64:
        //bit 0: 1b = Input buffer full
        //bit 1: 1b = Output buffer full

#define KBCMD_WRITE_CMD	0x60

#define KBSTATUS_IBF	0x02
#define KBSTATUS_OBF	0x01
#define KB_INIT_MODE	0x47
    //bit 0: 1b = enable keyboard interrupt
    //bit 1: 1b = enable mouse interrupt
    //bit 2: 1b = inform system finish starting process and initialization
    //bit 3: 0b, reserved
    //bit 4: 0b = enable keyboard equipment
    //bit 5: 0b = enable mouse equipment
    //bit 6: 1b = transform keyboard scancode (to first set)
    //bit 7: 0b, reserved
#define KBCMD_EN_MOUSE_INTFACE	0xa8 //Enable mouse port
#define KBCMD_SENDTO_MOUSE	    0xd4 //Sending data to mouse
#define MOUSE_ENABLE		    0xf4 //Allowing mouse sending data packadge

#define  wait_KB_write()	while(io_in8(PORT_KB_STATUS) & KBSTATUS_IBF)
#define  wait_KB_read()		while(io_in8(PORT_KB_STATUS) & KBSTATUS_OBF)

unsigned char pausebreak_scode[]={0xE1,0x1D,0x45,0xE1,0x9D,0xC5};
key_buffer Key_Buffer;
key_buffer Mouse_Buffer;
mouse_packet Mouse;

hw_int_controller keyboard_int_controller = 
{
	.enable     = IOAPIC_enable,
	.disable    = IOAPIC_disable,
	.install    = IOAPIC_install,
	.uninstall  = IOAPIC_uninstall,
	.ack        = IOAPIC_edge_ack,
	.do_soft	= NULL,
};

hw_int_controller mouse_int_controller = 
{
	.enable     = IOAPIC_enable,
	.disable    = IOAPIC_disable,
	.install    = IOAPIC_install,
	.uninstall  = IOAPIC_uninstall,
	.ack        = IOAPIC_edge_ack,
	.do_soft	= NULL,
};
void keyboard_init(void);
void keyboard_handler(unsigned long nr, unsigned long parameter);
void analysis_keycode(void);
unsigned char get_scancode(void);

void mouse_init(void);
void mouse_handler(unsigned long nr, unsigned long parameter);
unsigned char get_mousecode(void);
void analysis_mousecode(void);
#pragma endregion

#pragma region keyboard_ops
#define	KEY_CMD_RESET_BUFFER	0
#pragma endregion

#pragma region Keyboard map
    #define NR_SCAN_CODES 	0x80
    #define MAP_COLS		2
    unsigned int keycode_map_normal[NR_SCAN_CODES][MAP_COLS] = {
		/*scan-code	unShift		Shift		*/
		/*--------------------------------------------------------------*/
		/*0x00*/	0,		0,
		/*0x01*/	0,		0,		//ESC
		/*0x02*/	'1',		'!',
		/*0x03*/	'2',		'@',
		/*0x04*/	'3',		'#',
		/*0x05*/	'4',		'$',
		/*0x06*/	'5',		'%',
		/*0x07*/	'6',		'^',
		/*0x08*/	'7',		'&',
		/*0x09*/	'8',		'*',
		/*0x0a*/	'9',		'(',
		/*0x0b*/	'0',		')',
		/*0x0c*/	'-',		'_',
		/*0x0d*/	'=',		'+',
		/*0x0e*/	0,		0,		//BACKSPACE	
		/*0x0f*/	0,		0,		//TAB

		/*0x10*/	'q',		'Q',
		/*0x11*/	'w',		'W',
		/*0x12*/	'e',		'E',
		/*0x13*/	'r',		'R',
		/*0x14*/	't',		'T',
		/*0x15*/	'y',		'Y',
		/*0x16*/	'u',		'U',
		/*0x17*/	'i',		'I',
		/*0x18*/	'o',		'O',
		/*0x19*/	'p',		'P',
		/*0x1a*/	'[',		'{',
		/*0x1b*/	']',		'}',
		/*0x1c*/	0,		0,		//ENTER
		/*0x1d*/	0x1d,		0x1d,		//CTRL Left
		/*0x1e*/	'a',		'A',
		/*0x1f*/	's',		'S',

		/*0x20*/	'd',		'D',
		/*0x21*/	'f',		'F',
		/*0x22*/	'g',		'G',
		/*0x23*/	'h',		'H',
		/*0x24*/	'j',		'J',
		/*0x25*/	'k',		'K',
		/*0x26*/	'l',		'L',
		/*0x27*/	';',		':',
		/*0x28*/	'\'',		'"',
		/*0x29*/	'`',		'~',
		/*0x2a*/	0x2a,		0x2a,		//SHIFT Left
		/*0x2b*/	'\\',		'|',
		/*0x2c*/	'z',		'Z',
		/*0x2d*/	'x',		'X',
		/*0x2e*/	'c',		'C',
		/*0x2f*/	'v',		'V',

		/*0x30*/	'b',		'B',
		/*0x31*/	'n',		'N',
		/*0x32*/	'm',		'M',
		/*0x33*/	',',		'<',
		/*0x34*/	'.',		'>',
		/*0x35*/	'/',		'?',
		/*0x36*/	0x36,		0x36,		//SHIFT Right
		/*0x37*/	'*',		'*',
		/*0x38*/	0x38,		0x38,		//ALT Left
		/*0x39*/	' ',		' ',
		/*0x3a*/	0,		0,		//CAPS LOCK
		/*0x3b*/	0,		0,		//F1
		/*0x3c*/	0,		0,		//F2
		/*0x3d*/	0,		0,		//F3
		/*0x3e*/	0,		0,		//F4
		/*0x3f*/	0,		0,		//F5

		/*0x40*/	0,		0,		//F6
		/*0x41*/	0,		0,		//F7
		/*0x42*/	0,		0,		//F8
		/*0x43*/	0,		0,		//F9
		/*0x44*/	0,		0,		//F10
		/*0x45*/	0,		0,		//NUM LOCK
		/*0x46*/	0,		0,		//SCROLL LOCK
		/*0x47*/	'7',		0,		/*PAD HONE*/
		/*0x48*/	'8',		0,		/*PAD UP*/
		/*0x49*/	'9',		0,		/*PAD PAGEUP*/
		/*0x4a*/	'-',		0,		/*PAD MINUS*/
		/*0x4b*/	'4',		0,		/*PAD LEFT*/
		/*0x4c*/	'5',		0,		/*PAD MID*/
		/*0x4d*/	'6',		0,		/*PAD RIGHT*/
		/*0x4e*/	'+',		0,		/*PAD PLUS*/
		/*0x4f*/	'1',		0,		/*PAD END*/

		/*0x50*/	'2',		0,		/*PAD DOWN*/
		/*0x51*/	'3',		0,		/*PAD PAGEDOWN*/
		/*0x52*/	'0',		0,		/*PAD INS*/
		/*0x53*/	'.',		0,		/*PAD DOT*/
		/*0x54*/	0,		0,
		/*0x55*/	0,		0,
		/*0x56*/	0,		0,
		/*0x57*/	0,		0,		//F11
		/*0x58*/	0,		0,		//F12
		/*0x59*/	0,		0,		
		/*0x5a*/	0,		0,
		/*0x5b*/	0,		0,
		/*0x5c*/	0,		0,
		/*0x5d*/	0,		0,
		/*0x5e*/	0,		0,
		/*0x5f*/	0,		0,

		/*0x60*/	0,		0,
		/*0x61*/	0,		0,
		/*0x62*/	0,		0,
		/*0x63*/	0,		0,
		/*0x64*/	0,		0,
		/*0x65*/	0,		0,
		/*0x66*/	0,		0,
		/*0x67*/	0,		0,
		/*0x68*/	0,		0,
		/*0x69*/	0,		0,
		/*0x6a*/	0,		0,
		/*0x6b*/	0,		0,
		/*0x6c*/	0,		0,
		/*0x6d*/	0,		0,
		/*0x6e*/	0,		0,
		/*0x6f*/	0,		0,

		/*0x70*/	0,		0,
		/*0x71*/	0,		0,
		/*0x72*/	0,		0,
		/*0x73*/	0,		0,
		/*0x74*/	0,		0,
		/*0x75*/	0,		0,
		/*0x76*/	0,		0,
		/*0x77*/	0,		0,
		/*0x78*/	0,		0,
		/*0x79*/	0,		0,
		/*0x7a*/	0,		0,
		/*0x7b*/	0,		0,
		/*0x7c*/	0,		0,
		/*0x7d*/	0,		0,
		/*0x7e*/	0,		0,
		/*0x7f*/	0,		0,
    };
#pragma endregion

#pragma region IDE
#define	PORT_DISK0_DATA			0x1f0
#define	PORT_DISK0_ERR_FEATURE	0x1f1
#define	PORT_DISK0_SECTOR_CNT	0x1f2
#define	PORT_DISK0_SECTOR_LOW	0x1f3   //Sector No. or LBA[0:7]
#define	PORT_DISK0_SECTOR_MID	0x1f4   //Cylinder[0:7] or LBA[8:15]
#define	PORT_DISK0_SECTOR_HIGH	0x1f5   //Cylinder[0:7] or LBA[16:23]
#define	PORT_DISK0_DEVICE		0x1f6
    //Bit           CHS mode:   LBA mode:
    //bit 7:        1b          1b
    //bit 6:        0b          1b
    //bit 5:        1b          1b
    //bit 4:        0b/1b       0b/1b           0b = Master drive      1b = Slave drive
    //bit 0~3:      Header      LBA[24:27]
#define	PORT_DISK0_STATUS_CMD	0x1f7
	//Write:
		//0xec: Read device configuration info.
		//0x20: Read sector (LBA[0:27])
		//0x24: Extend read sector (LBA[0:48])
		//0x30: Write sector (LBA[0:27])
		//0x34: Extend write sector (LBA[0:48])
	//Read:
		//Same as PORT_DISK0_ALT_STA_CTL
#define	PORT_DISK0_ALT_STA_CTL	0x3f6
    //Write:
        //bit 2: 0b = Normal operation; 1b = Restart controller
        //bit 1: 0b = Disable interrupt; 1b = Enable interrupt (IRQ 14)
    //Read:
        //bit 7: 1b = Controller busy
        //bit 6: 1b = Device ready
        //bit 3: 1b = Data request (Waiting for data input)
        //bit 0: 1b = Execution error
#define	DISK_STATUS_BUSY		(1 << 7)
#define	DISK_STATUS_READY		(1 << 6)
#define	DISK_STATUS_REQ			(1 << 3)
#define	DISK_STATUS_ERROR		(1 << 0)
#define ATA_READ_CMD			0x24
#define ATA_WRITE_CMD			0x34
#define GET_IDENTIFY_DISK_CMD	0xEC

hw_int_controller disk_int_controller = 
{
	.enable     = IOAPIC_enable,
	.disable    = IOAPIC_disable,
	.install    = IOAPIC_install,
	.uninstall  = IOAPIC_uninstall,
	.ack        = IOAPIC_edge_ack,
	.do_soft	= NULL,
};

typedef struct __attribute__((__packed__)) {
	unsigned short General_Config;      //  0	General configuration bit-significant information
	unsigned short Obsolete0;           //	1	Obsolete
	unsigned short Specific_Coinfig;    //	2	Specific configuration
	unsigned short Obsolete1;           //	3	Obsolete
	unsigned short Retired0[2];         //	4-5	Retired
	unsigned short Obsolete2;           //	6	Obsolete
	unsigned short CompactFlash[2];     //	7-8	Reserved for the CompactFlash Association
	unsigned short Retired1;            //	9	Retired
	unsigned short Serial_Number[10];   //	10-19	Serial number (20 ASCII characters)
	unsigned short Retired2[2];         //	20-21	Retired
	unsigned short Obsolete3;           //	22	Obsolete
	unsigned short Firmware_Version[4]; //	23-26	Firmware revision(8 ASCII characters)
	unsigned short Model_Number[20];    //	27-46	Model number (40 ASCII characters)

	//	47	15:8 	80h 
	//		7:0  	00h=Reserved 
	//			01h-FFh = Maximumnumber of logical sectors that shall be transferred per DRQ data block on READ/WRITE MULTIPLE commands
	unsigned short Max_logical_transferred_per_DRQ;

	//	48	Trusted Computing feature set options
	unsigned short Trusted_Computing_feature_set_options;

	//	49	Capabilities
	unsigned short Capabilities0;

	//	50	Capabilities
	unsigned short Capabilities1;

	//	51-52	Obsolete
	unsigned short Obsolete4[2];

	//	53	15:8	Free-fall Control Sensitivity
	//		7:3 	Reserved
	//		2 	the fields reported in word 88 are valid
	//		1 	the fields reported in words (70:64) are valid
	unsigned short Report_88_70to64_valid;

	//	54-58	Obsolete
	unsigned short Obsolete5[5];

	//	59	15:9	Reserved
	//		8	Multiple sector setting is valid	
	//		7:0	xxh current setting for number of logical sectors that shall be transferred per DRQ data block on READ/WRITE Multiple commands
	unsigned short Mul_Sec_Setting_Valid;

	//	60-61	Total number of user addresssable logical sectors for 28bit CMD
	unsigned short Addressable_Logical_Sectors_for_28[2];

	//	62	Obsolete
	unsigned short Obsolete6;

	//	63	15:11	Reserved
	//		10:8=1 	Multiword DMA mode 210 is selected
	//		7:3 	Reserved
	//		2:0=1 	Multiword DMA mode 210 and below are supported
	unsigned short MultWord_DMA_Select;

	//	64	15:8	Reserved
	//		7:0	PIO mdoes supported
	unsigned short PIO_mode_supported;

	//	65	Minimum Multiword DMA transfer cycle time per word
	unsigned short Min_MulWord_DMA_cycle_time_per_word;

	//	66	Manufacturer`s recommended Multiword DMA transfer cycle time
	unsigned short Manufacture_Recommend_MulWord_DMA_cycle_time;

	//	67	Minimum PIO transfer cycle time without flow control
	unsigned short Min_PIO_cycle_time_Flow_Control;

	//	68	Minimum PIO transfer cycle time with IORDY flow control
	unsigned short Min_PIO_cycle_time_IOREDY_Flow_Control;

	//	69-70	Reserved
	unsigned short Reserved1[2];

	//	71-74	Reserved for the IDENTIFY PACKET DEVICE command
	unsigned short Reserved2[4];

	//	75	Queue depth
	unsigned short Queue_depth;

	//	76	Serial ATA Capabilities 
	unsigned short SATA_Capabilities;

	//	77	Reserved for Serial ATA 
	unsigned short Reserved3;

	//	78	Serial ATA features Supported 
	unsigned short SATA_features_Supported;

	//	79	Serial ATA features enabled
	unsigned short SATA_features_enabled;

	//	80	Major Version number
	unsigned short Major_Version;

	//	81	Minor version number
	unsigned short Minor_Version;

	//	82	Commands and feature sets supported
	unsigned short Cmd_feature_sets_supported0;

	//	83	Commands and feature sets supported	
	unsigned short Cmd_feature_sets_supported1;

	//	84	Commands and feature sets supported
	unsigned short Cmd_feature_sets_supported2;

	//	85	Commands and feature sets supported or enabled
	unsigned short Cmd_feature_sets_supported3;

	//	86	Commands and feature sets supported or enabled
	unsigned short Cmd_feature_sets_supported4;

	//	87	Commands and feature sets supported or enabled
	unsigned short Cmd_feature_sets_supported5;

	//	88	15 	Reserved 
	//		14:8=1 	Ultra DMA mode 6543210 is selected 
	//		7 	Reserved 
	//		6:0=1 	Ultra DMA mode 6543210 and below are suported
	unsigned short Ultra_DMA_modes;

	//	89	Time required for Normal Erase mode SECURITY ERASE UNIT command
	unsigned short Time_required_Erase_CMD;

	//	90	Time required for an Enhanced Erase mode SECURITY ERASE UNIT command
	unsigned short Time_required_Enhanced_CMD;

	//	91	Current APM level value
	unsigned short Current_APM_level_Value;

	//	92	Master Password Identifier
	unsigned short Master_Password_Identifier;

	//	93	Hardware resset result.The contents of bits (12:0) of this word shall change only during the execution of a hardware reset.
	unsigned short HardWare_Reset_Result;

	//	94	Current AAM value 
	//		15:8 	Vendor’s recommended AAM value 
	//		7:0 	Current AAM value
	unsigned short Current_AAM_value;

	//	95	Stream Minimum Request Size
	unsigned short Stream_Min_Request_Size;

	//	96	Streaming Transger Time-DMA 
	unsigned short Streaming_Transger_time_DMA;

	//	97	Streaming Access Latency-DMA and PIO
	unsigned short Streaming_Access_Latency_DMA_PIO;

	//	98-99	Streaming Performance Granularity (DWord)
	unsigned short Streaming_Performance_Granularity[2];

	//	100-103	Total Number of User Addressable Logical Sectors for 48-bit commands (QWord)
	unsigned int Total_user_LBA_for_48_Address_Feature_set;

	//	104	Streaming Transger Time-PIO
	unsigned short Streaming_Transfer_Time_PIO;

	//	105	Reserved
	unsigned short Reserved4;

	//	106	Physical Sector size/Logical Sector Size
	unsigned short Physical_Logical_Sector_Size;

	//	107	Inter-seek delay for ISO-7779 acoustic testing in microseconds
	unsigned short Inter_seek_delay;

	//	108-111	World wide name	
	unsigned short World_wide_name[4];

	//	112-115	Reserved
	unsigned short Reserved5[4];

	//	116	Reserved for TLC
	unsigned short Reserved6;

	//	117-118	Logical sector size (DWord)
	unsigned short Words_per_Logical_Sector[2];

	//	119	Commands and feature sets supported (Continued from words 84:82)
	unsigned short CMD_feature_Supported;

	//	120	Commands and feature sets supported or enabled (Continued from words 87:85)
	unsigned short CMD_feature_Supported_enabled;

	//	121-126	Reserved for expanded supported and enabled settings
	unsigned short Reserved7[6];

	//	127	Obsolete
	unsigned short Obsolete7;

	//	128	Security status
	unsigned short Security_Status;

	//	129-159	Vendor specific
	unsigned short Vendor_Specific[31];

	//	160	CFA power mode
	unsigned short CFA_Power_mode;

	//	161-167	Reserved for the CompactFlash Association
	unsigned short Reserved8[7];

	//	168	Device Nominal Form Factor
	unsigned short Dev_from_Factor;

	//	169-175	Reserved
	unsigned short Reserved9[7];

	//	176-205	Current media serial number (ATA string)
	unsigned short Current_Media_Serial_Number[30];

	//	206	SCT Command Transport
	unsigned short SCT_Cmd_Transport;

	//	207-208	Reserved for CE-ATA
	unsigned short Reserved10[2];

	//	209	Alignment of logical blocks within a physical block
	unsigned short Alignment_Logical_blocks_within_a_physical_block;

	//	210-211	Write-Read-Verify Sector Count Mode 3 (DWord)
	unsigned short Write_Read_Verify_Sector_Count_Mode_3[2];

	//	212-213	Write-Read-Verify Sector Count Mode 2 (DWord)
	unsigned short Write_Read_Verify_Sector_Count_Mode_2[2];

	//	214	NV Cache Capabilities
	unsigned short NV_Cache_Capabilities;

	//	215-216	NV Cache Size in Logical Blocks (DWord)
	unsigned short NV_Cache_Size[2];

	//	217	Nominal media rotation rate
	unsigned short Nominal_media_rotation_rate;

	//	218	Reserved
	unsigned short Reserved11;

	//	219	NV Cache Options
	unsigned short NV_Cache_Options;

	//	220	Write-Read-Verify feature set current mode
	unsigned short Write_Read_Verify_feature_set_current_mode;

	//	221	Reserved
	unsigned short Reserved12;

	//	222	Transport major version number. 
	//		0000h or ffffh = device does not report version
	unsigned short Transport_Major_Version_Number;

	//	223	Transport Minor version number
	unsigned short Transport_Minor_Version_Number;

	//	224-233	Reserved for CE-ATA
	unsigned short Reserved13[10];

	//	234	Minimum number of 512-byte data blocks per DOWNLOAD MICROCODE command for mode 03h
	unsigned short Mini_blocks_per_CMD;

	//	235	Maximum number of 512-byte data blocks per DOWNLOAD MICROCODE command for mode 03h
	unsigned short Max_blocks_per_CMD;

	//	236-254	Reserved
	unsigned short Reserved14[19];

	//	255	Integrity word
	//		15:8	Checksum
	//		7:0	Checksum Validity Indicator
	unsigned short Integrity_word;
} Disk_Identify_Info;

typedef struct {
	unsigned char 	status;
	unsigned int 	count;
	unsigned char 	cmd;
	unsigned long 	LBA;
	unsigned char 	*buffer;
	void(* end_handler)(unsigned long nr, unsigned long parameter);
	unsigned long *Ind;

	list List;
} block_buffer_node;

typedef struct {
	list Queue_List;
	block_buffer_node *in_using;
	unsigned long block_request_count;
} request_queue;
request_queue Disk_request;

void disk_handler(unsigned long nr, unsigned long parameter);
void Disk_Init(void);
void Disk_End_REQ(void);
void Disk_Post_REQ(unsigned char cmd, unsigned long LBA, unsigned int count, unsigned char *buffer, unsigned long *Ind);
void Disk_Exec_REQ(void);
void Disk_Read_handler(unsigned long nr, unsigned long parameter);
void Disk_Write_handler(unsigned long nr, unsigned long parameter);
void Disk_ConfInfo_handler(unsigned long nr, unsigned long parameter);
void Print_Disk_Config(void);
void IDE_Disk_Init(void);
#pragma endregion

#pragma region AHCI
#define	SATA_SIG_ATA			0x00000101	// SATA drive
#define	SATA_SIG_ATAPI			0xEB140101	// SATAPI drive
#define SATA_ACT_PRES			0X103		// Device present, communication established and active
#define SATA_GHC_AE    			0x80000000	// GHC.AE is 1, indicating system software is AHCI aware
#define SATA_GHC_IE    			1<<1		// Enable interrupte
#define HBA_PxCMD_ST    		0x0001
#define HBA_PxCMD_FRE   		0x0010
#define HBA_PxCMD_FR    		0x4000
#define HBA_PxCMD_CR    		0x8000
#define FIS_TYPE_REG_H2D		0x27
#define ATA_CMD_READ_DMA_EX		0x25
#define ATA_CMD_WRITE_DMA_EX	0x35
#define IDENTIFY_DEVICE			0xEC
#define ATA_DEV_BUSY 			0x80
#define ATA_DEV_DRQ 			0x08

typedef struct{
	unsigned int 	COUNT;
	unsigned char 	CMD;
	unsigned long 	LBA;
	unsigned char 	*phy_buffer;
	unsigned long 	schedual;
	unsigned long	slot;
	list			List;
	task_struct 	*tsk;
}req_node;

typedef struct{
	list Queue_List;
	req_node *in_using;
	unsigned long global_schedual;
	unsigned long node_count;
}req_queue;

req_queue AHCI_queue;

void AHCI_Disk_Init(void);
void NVME_Disk_Init(void);
int  AHCI_PostCMD(void);
void AHCI_handler(unsigned long nr, unsigned long parameter);
void AHCI_REQ(int lba, int NoSec, void *Buf, char cmd, unsigned long schedual);
#pragma endregion

#pragma NVME
#define write_SQT(id,val) (*(controller_dbl*)((unsigned long)NVME.MBAR + 0x1000 + (2*id)*(4<<NVME.MBAR->cap.dstrd))).SQTdbl = val;
#define write_CQH(id,val) (*(controller_dbl*)((unsigned long)NVME.MBAR + 0x1000 + (2*id)*(4<<NVME.MBAR->cap.dstrd))).CQHdbl = val;
#pragma endregion