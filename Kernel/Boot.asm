org 0x7c00 ;
;=====================================================================================
;编译常量
;=====================================================================================
SectorNumOfRootDirStart   equ  25		;Boot(1 sector) + 2 * FAT(12 sector) = 25 sector, Root start on 26th sector
										;1st sector numbering 0, thus Root start on sector 25
SectorNumOfFAT1Start      equ  1
RootDirSectors			  equ  14     	;Number of sectors Root Directory took.
                                      	;Since each entry takes 32 byte, thus 
									  	;RootDirSectors = RoundUp(BPB_RootEntCnt * 32 / BPB_BytsPerSec, 0)
SectorBalance             equ  SectorNumOfRootDirStart + RootDirSectors - 16
										;Fisrt cluster in data area corresponding to sector FAT[2]
										;Start sector of first cluster in data area = SectorNumOfRootDirStart + RootDirSectors
										;Targeting on n'th cluster, start sector = n*8 + (SectorNumOfRootDirStart + RootDirSectors - 2 * BPB_SecPerClus)
BaseOfStack	              equ  0x7c00
BaseOfLoader	          equ  0x1000
BytesPerCluster			  equ  512 * 8
;-------------------------------------------------------------------------------------

;=====================================================================================
;FAT12 头文件
;Simulation of USB 15.7MB:
;	Total of 32130 sectors
;	255 heads, 63 sectors per track, 2 cylinders
;	
;=====================================================================================
jmp short LABEL_START		; Start to boot.
nop				            ; 这个 nop 不可少

BS_OEMName		DB 'MINEboot'	; OEM String, 必须 8 个字节
BPB_BytsPerSec	DW 512			; 每扇区字节数
BPB_SecPerClus	DB 8			; Number of sector in cluster, used to be 1
BPB_RsvdSecCnt	DW 1			; 保留扇区数，这里只引导扇区
BPB_NumFATs		DB 2		    ; Total number of FAT tables
BPB_RootEntCnt	DW 224			; 根目录文件数最大值
BPB_TotSec16	DW 32130		; Total number of sector
BPB_Media		DB 0xF0		    ; 媒体描述符
BPB_FATSz16		DW 12		    ; Number of sector in each FAT table
BPB_SecPerTrk	DW 63			; Sector per track
BPB_NumHeads	DW 255			; Number of heads
BPB_HiddSec		DD 0		    ; 隐藏扇区数
BPB_TotSec32	DD 0		    ; wTotalSectorCount为0时这个值记录扇区数
BS_DrvNum		DB 0		    ; 中断 13 的驱动器号
BS_Reserved1	DB 0		    ; 未使用
BS_BootSig		DB 29h		    ; 扩展引导标记 (29h)
BS_VolID		DD 0		    ; 卷序列号
BS_VolLab		DB 'OrangeS0.02'; 卷标, 必须 11 个字节
BS_FileSysType	DB 'FAT12   '	; 文件系统类型, 必须 8个字节
;-------------------------------------------------------------------------------------

;=====================================================================================
;引导函数
;=====================================================================================
LABEL_START:
;===== Clear Regs
	cli     ;Disable interrupt
	xor     ax,ax
	mov     ss,ax
	mov     ds,ax
	mov     es,ax
	mov     fs,ax
	mov     gs,ax
	sti     ;Enable interrupt
;===== Allocate stack
	mov	    sp, BaseOfStack
;===== Clear Screen
	call    Clear_Screen
;===== Load kernel
	mov     si, Load_Kernel
	call    Print_Str
	mov     si, LoaderFileName
	call    Func_SearchDir
	cmp		ax, 0fffh
	jnz     .LoadFile
	mov     si, File_Not_Fund_Str
	call    Print_Str
	jmp     $
;===== Load found file
.LoadFile:
	mov 	si, File_Fund_Str
	call    Print_Str
	mov     bx, BaseOfLoader
	mov     es, bx
	xor     bx, bx
	call    Func_Load_File
	mov     si, File_Loaded_Str
	call    Print_Str
	jmp     BaseOfLoader:0h
;-------------------------------------------------------------------------------------

;=====================================================================================
;变量
;=====================================================================================
RootDirSizeForLoop	dw	RootDirSectors
SectorNo            dw  SectorNumOfRootDirStart
NamePtr             dw  0
;-------------------------------------------------------------------------------------

;=====================================================================================
;字符串
;=====================================================================================
Load_Kernel        db 'Loading Loader.bin:', 13, 10, 13, 10, 0
File_Not_Fund_Str: db 'ERROR: No file found', 13, 10, 13, 10, 0
File_Fund_Str:     db 'File found, loading...', 0
File_Loaded_Str:   db 'Loaded!', 13, 10, 13, 10, 0
LoaderFileName:	   db 'LOADER  BIN',0
;-------------------------------------------------------------------------------------


;=====================================================================================
;函数
;=====================================================================================
;*****************************************************************************
;Clear screen
;Register Operation:
;   Function begin: pusha
;   Function end:   popa
;*****************************************************************************
Clear_Screen:
	pusha
	mov     ax, 0003H
	int     10H
	popa
ret
;*****************************************************************************
;Input:
;	ES:BX: Memory buffer address
;   AX ->  First cluster of FAT entry
;Output:
;   [ES:BX] -> Starting position of loaded file
;Register Operation:
;   N\A
;External Functions:
;   Func_ReadSectors
;   Func_GetFATEntry
;*****************************************************************************
Func_Load_File:
.ContRead:
	push    ax
	xor		cx, cx
	mov		cl, Byte[BPB_SecPerClus]
	mul		cl
	add		ax, SectorBalance
	call    Func_ReadSectors
	pop     ax
	push    es
	push    bx
	call    Func_GetFATEntry
	cmp     ax, 0fffh
	jz      .Loaded
	pop     bx
	pop     es
	add     bx, BytesPerCluster
	jmp     .ContRead
.Loaded:
	pop     bx
	pop     es
	ret
;*****************************************************************************
;Input:
;   SI -> Starting address of file name
;Output:
;   AX succeed -> First cluster of FAT entry
;      fail    -> 0fffh
;Register Operation:
;   N\A
;External Functions:
;   Func_ReadSectors
;*****************************************************************************
Func_SearchDir:
	mov     word[NamePtr], si
;===== Clear DF, so SI increasing when using LODSB instruction
	cld
;===== Set buffer address
	mov		ax, 0
	mov		es, ax
	mov		bx, 8000h ;Set buffer address
;===== Total no. of sectors root directory took
	mov		word[RootDirSizeForLoop], RootDirSectors
	mov     word[SectorNo], SectorNumOfRootDirStart
;===== Search loop
.DIR_Loop:
	cmp		word[RootDirSizeForLoop], 0
	jz      .NotFound
	mov		di, 7FE0h				  ;DI = 8000h - 20h
	mov		cl, 1
	mov     ax, word[SectorNo]
	call    Func_ReadSectors
	dec		word[RootDirSizeForLoop]
	inc     word[SectorNo]
	mov     dx, 10h					  ;Each sector 512/32 = 16 Directories
.SCT_Loop:
	and     di, 0FFE0h                ;Clear last 5 bit  DI = int[DI/32] * 32
	add     di, 20h	                  ;Next directory    DI = DI + 32
	cmp     dx, 0
	jz      .DIR_Loop
	dec     dx
	mov     si, word[NamePtr]
	mov     cx, 11
.NME_Loop:
	cmp     cx, 0
	jz      .Found
	dec     cx
	lodsb 
	cmp     al, byte[es:di]
	jnz     .SCT_Loop
	inc     di
	jmp     .NME_Loop
;===== Post search operations
.NotFound:
	mov		ax, 0fffh                    ;0fffh if not found
	jmp		.END
.Found:
	and     di, 0FFE0h
	add     di, 1ah
	mov     ax, word[es:di]
.END:
	ret
;*****************************************************************************
;Reading driver function
;Input:
;	AX:    Logical start number of sector (0 to 2879)
;	CL:    Number of sectors to read
;	ES:BX: Memory buffer address
;Output:
;   [ES:BX] -> Loaded content from flop disk
;Register Operation:
;   Function begin: pusha
;   Function end:   popa
;int 13h Interruption:
;   AH:    02H = Reading sector
;   AL:    Number of sectors to read 
;   CH:    Cylinder
;   CL:    Sector pos
;	DL:    Drive No: bit7=0:floppy; bit7=1 fixed drive
;   DH:    Head
;	ES:BX: Memory buffer address
;pusha/popa: AX,CX,DX,BX,SP(初始值0x7c00),BP,SI,DI
;*****************************************************************************
Func_ReadSectors:
;===== switch stack pointer, allocate local variable space
	push	ds
	push	ax
	push    dx

	push	dword	00h
	push	word	00h
	push 	word	ax
	push	word	es
	push	word	bx
	push	word	cx
	push	word	10h
	mov	ax,	ss
	mov	ds,	ax
	mov	ah,	42h	;read
	mov	dl,	80h ;0x0:Read from floppy disk; 0x80:Read from hard disk;
	mov	si,	sp
	int	13h
	add	sp,	10h

	pop dx
	pop	ax
	pop ds
	ret
;*****************************************************************************
;LoadFAT 
;Input:
;	AX: n'th FAT12 entry, or Cluster No.
;Output:
;	AX: Content of n'th FAT12 entry
;Register Operation:
;   N/A
;External Functions:
;   Func_ReadSectors
;*****************************************************************************
Func_GetFATEntry:
;===== Set buff base address
	mov     bx, 0
	mov     es, bx
	
;===== Calculate Sector
	mov     bx, 3
	mul     bx
	mov     bx, 2
	div     bx        ;DX refilled with mul instruction
                      ;Each entry takes 1.5 byte, n * 3/2 result in the number of byte
			          ;DIV => AX = Quotient, DX = reminder
	push    dx
;===== Load Sector to Memory
	xor     dx, dx    ;Clear DX, 
	mov     bx, [BPB_BytsPerSec]
	div     bx
	mov     bx, 8000h ;Set buff offset address
	add     ax, SectorNumOfFAT1Start
	mov     cl, 2
	call    Func_ReadSectors
;===== Find value
	add     bx, dx
	mov     ax, [es:bx]
	
	pop     dx
	cmp     dx, 1
	jnz      .ODD
	shr     ax, 4
	jmp     .END
.ODD:
	and     ax, 0FFFh
.END:
	ret
;*****************************************************************************
;Print string
;Input:
;   SI -> Starting position of '\0' end string
;Output:
;   N/A
;Register Operation:
;   Function begin: pusha
;   Function end:   popa
;*****************************************************************************
Print_Str:
	pusha
	cld; si moving forward from memory(si++)
	mov ah,0x0E
	.PrintLoop:
		lodsb
		cmp al, 0
		je .EndPrint
		int 0x10
		jmp .PrintLoop
	.EndPrint:
	popa
	ret
;-------------------------------------------------------------------------------------
times 510 - ($ - $$) db 0
dw    0xaa55