#include <Uefi.h>
#include <Library/UefiLib.h>                    //Structure: EFI_GRAPHICS_OUTPUT_PROTOCOL and others
#include <Library/UefiBootServicesTableLib.h>   //gBS: global boot service
#include <Protocol/DiskIo.h>                    //gEfiDiskIoProtocolGuid
#include <Protocol/DevicePathToText.h>          //EFI_DEVICE_PATH_TO_TEXT_PROTOCOL
#include <Guid/FileInfo.h>                      //EFI_FILE_INFO*
#include <Guid/Acpi.h>                          //ACPI GUID
#include <Library/BaseMemoryLib.h>              //CompareGuid

#define ERROR_BREAK(Status,info) if(EFI_ERROR(Status)){ Print(info, Status); while(1); }
#define KERNEL_DISK_NO 3
typedef struct __attribute__((__packed__)) {
    char 			Signature[8];
    char 			Checksum;
    char 			OEMID[6];
    char 			Revision;
    unsigned int 	RSDT;		//Version 1.0 Structure end here
    unsigned int 	Length;
    unsigned long 	XSDT;
    char 			ExtendedChecksum;
    char 			reserved[3];
} rsdp;

typedef struct{
    __volatile__ unsigned long lock;
}spinlock_T;

typedef struct {
    int XResolution;
    int YResolution;

    unsigned long FB_addr;
    unsigned long FB_length;
} position;

typedef struct __attribute__((__packed__)) {
    unsigned long address;
    unsigned long length;
    unsigned int  type;
} EFI_E820_MEMORY_DESCRIPTOR;

typedef struct {
    unsigned int E820_Entry_count;
    EFI_E820_MEMORY_DESCRIPTOR E820_Entry[0];
} EFI_E820_MEMORY_DESCRIPTOR_INFORMATION;

typedef struct {
    position Graphics_Info;
    rsdp RSDP;
    EFI_E820_MEMORY_DESCRIPTOR_INFORMATION E820_Info;
} KERNEL_BOOT_PARAMETER_INFORMATION;