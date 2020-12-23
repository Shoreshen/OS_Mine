#include "BootX64.h"

KERNEL_BOOT_PARAMETER_INFORMATION   *kernel_boot_para_info = NULL;

EFI_STATUS Set_Video(int verbose)
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gGraphicsOutput = 0;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* Info = 0;
    EFI_STATUS Status;
    UINTN InfoSize = 0;
    int i = 0;

    Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,NULL,(VOID **)&gGraphicsOutput);
    ERROR_BREAK(Status, L"LocateProtocol error\n");
    
    Print(L"Current Mode:%02d,Version:%x,Format:%d,Horizontal:%d,Vertical:%d,ScanLine:%d,FrameBufferBase:%010lx,FrameBufferSize:%010lx\n",
        gGraphicsOutput->Mode->Mode,
        gGraphicsOutput->Mode->Info->Version,
        gGraphicsOutput->Mode->Info->PixelFormat,
        gGraphicsOutput->Mode->Info->HorizontalResolution,
        gGraphicsOutput->Mode->Info->VerticalResolution,
        gGraphicsOutput->Mode->Info->PixelsPerScanLine,
        gGraphicsOutput->Mode->FrameBufferBase,
        gGraphicsOutput->Mode->FrameBufferSize
    );

    long H_V_Resolution = gGraphicsOutput->Mode->Info->HorizontalResolution * gGraphicsOutput->Mode->Info->VerticalResolution;
    int MaxResolutionMode = gGraphicsOutput->Mode->Mode;

    for(i = 0;i < gGraphicsOutput->Mode->MaxMode;i++) {
        Status = gGraphicsOutput->QueryMode(gGraphicsOutput,i,&InfoSize,&Info);
        ERROR_BREAK(Status, L"QueryMode error\n");

        Print(L"Mode:%02d,Version:%x,Format:%d,Horizontal:%d,Vertical:%d,ScanLine:%d\n",i,
            Info->Version,
            Info->PixelFormat,
            Info->HorizontalResolution,
            Info->VerticalResolution,
            Info->PixelsPerScanLine
        );
        if((Info->PixelFormat == 1) && 
            (Info->HorizontalResolution * Info->VerticalResolution > H_V_Resolution) &&
            Info->HorizontalResolution <= 1600 &&
            Info->VerticalResolution <= 900
        ) {
            H_V_Resolution = Info->HorizontalResolution * Info->VerticalResolution;
            MaxResolutionMode = i;
        } 
        gBS->FreePool(Info);
    }
    if(gGraphicsOutput->Mode->Mode != MaxResolutionMode){
        gGraphicsOutput->SetMode(gGraphicsOutput,MaxResolutionMode);
        ERROR_BREAK(Status, L"SetMode error\n");
        kernel_boot_para_info->Graphics_Info.FB_addr = gGraphicsOutput->Mode->FrameBufferBase;
        kernel_boot_para_info->Graphics_Info.FB_length = gGraphicsOutput->Mode->FrameBufferSize;
        kernel_boot_para_info->Graphics_Info.XResolution = gGraphicsOutput->Mode->Info->HorizontalResolution;
        kernel_boot_para_info->Graphics_Info.YResolution = gGraphicsOutput->Mode->Info->VerticalResolution;
    }
    Print(L"Current Mode:%02d,Version:%x,Format:%d,Horizontal:%d,Vertical:%d,ScanLine:%d,FrameBufferBase:%010lx,FrameBufferSize:%010lx\n",
        gGraphicsOutput->Mode->Mode,
        gGraphicsOutput->Mode->Info->Version,
        gGraphicsOutput->Mode->Info->PixelFormat,
        gGraphicsOutput->Mode->Info->HorizontalResolution,
        gGraphicsOutput->Mode->Info->VerticalResolution,
        gGraphicsOutput->Mode->Info->PixelsPerScanLine,
        gGraphicsOutput->Mode->FrameBufferBase,
        gGraphicsOutput->Mode->FrameBufferSize
    );

    return EFI_SUCCESS;
}

EFI_STATUS Get_MemMap(int verbose, UINTN *MapKey)
{
    UINTN MemMapSize = 0;
    EFI_MEMORY_DESCRIPTOR* MemMap = 0;
    UINTN DescriptorSize = 0;
    UINT32 DesVersion = 0;
    EFI_STATUS status;
    EFI_MEMORY_DESCRIPTOR* MMap;
    int i, j, MemType, E820Count = 0;
    unsigned long LastEndAddr = 0;
    EFI_E820_MEMORY_DESCRIPTOR  *E820p = kernel_boot_para_info->E820_Info.E820_Entry, 
                                *LastE820 = NULL, *e820i, *e820j;
    EFI_E820_MEMORY_DESCRIPTOR  TempE820;

    Print(L"Get EFI_MEMORY_DESCRIPTOR Structure\n");
    gBS->GetMemoryMap(
        &MemMapSize,    //Size of MemMap buffer, if it is too small, return EFI_BUFFER_TOO_SMALL and fill in the size needed
        MemMap,         //MemMap pointer to the buffer, array of EFI_MEMORY_DESCRIPTOR
        MapKey,
        &DescriptorSize,//Size of each EFI_MEMORY_DESCRIPTOR
        &DesVersion     //Version of EFI_MEMORY_DESCRIPTOR
    );
    MemMapSize += DescriptorSize * 5;
    gBS->AllocatePool(
        EfiRuntimeServicesData, //Type of pool to allocate, 
                                //EfiRuntimeServicesData is the data allocation type used by a Runtime Services Driver to allocate pool memory
        MemMapSize,             //Size of pool to allocate
        (VOID**)&MemMap         //Pointer
    ); //Allocate memory
    status = gBS->GetMemoryMap(&MemMapSize,MemMap,MapKey,&DescriptorSize,&DesVersion); //First time get needed buffer size, second get MemMap information
    Print(L"status: %d, %d, %d\n", status, EFI_INVALID_PARAMETER, EFI_BUFFER_TOO_SMALL);
    ERROR_BREAK(status, L"GetMemoryMap error\n");
    
    Print(L"Sizeof(EFI_MEMORY_DESCRIPTOR) = %d, number of discriptro: %d\n", DescriptorSize, MemMapSize / DescriptorSize);
    for(i = 0; i< MemMapSize / DescriptorSize; i++) {
        MMap = (EFI_MEMORY_DESCRIPTOR*) (((unsigned long)MemMap) + i * DescriptorSize);
        // Print(L"MemoryMap %4d %10d (%10lx~%10lx) %016lx\n",
        //     MMap->Type,                                         //Speficied in EFI_MEMORY_TYPE
        //     MMap->NumberOfPages,
        //     MMap->PhysicalStart,
        //     MMap->PhysicalStart + (MMap->NumberOfPages << 12),  //Page size 4KB = 1<<12
        //     MMap->Attribute
        // );
        switch(MMap->Type) {
            case EfiReservedMemoryType:
            case EfiMemoryMappedIO:
            case EfiMemoryMappedIOPortSpace:
            case EfiPalCode:
                MemType= 2;    //2:ROM or Reserved
                break;

            case EfiUnusableMemory:
                MemType= 5;    //5:Unusable
                break;

            case EfiACPIReclaimMemory:
                MemType= 3;    //3:ACPI Reclaim Memory
                break;

            case EfiLoaderCode:
            case EfiLoaderData:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiRuntimeServicesCode:
            case EfiRuntimeServicesData:
            case EfiConventionalMemory:
            case EfiPersistentMemory:
                MemType= 1;    //1:RAM
                break;

            case EfiACPIMemoryNVS:
                MemType= 4;    //4:ACPI NVS Memory
                break;

            default:
                Print(L"Invalid UEFI Memory Type:%4d\n",MMap->Type);
                continue;
        }
        if((LastE820 != NULL) && (LastE820->type == MemType) && (MMap->PhysicalStart == LastEndAddr))
        {
            LastE820->length += MMap->NumberOfPages << 12;
            LastEndAddr += MMap->NumberOfPages << 12;
        }
        else
        {
            E820p->address = MMap->PhysicalStart;
            E820p->length = MMap->NumberOfPages << 12;
            E820p->type = MemType;
            LastEndAddr = MMap->PhysicalStart + (MMap->NumberOfPages << 12);
            LastE820 = E820p;
            E820p++;
            E820Count++;
        }
    }
    kernel_boot_para_info->E820_Info.E820_Entry_count = E820Count;
    LastE820 = kernel_boot_para_info->E820_Info.E820_Entry;
    for(i = 0; i< E820Count; i++) {
        e820i = LastE820 + i;
        for(j = i + 1; j< E820Count; j++) {
            e820j = LastE820 + j;
            if(e820i->address > e820j->address) {
                TempE820 = *e820i;
                *e820i   = *e820j;
                *e820j   = TempE820;
            }
        }
    }
    for(i = 0;i < kernel_boot_para_info->E820_Info.E820_Entry_count;i++) {
        Print(L"MemoryMap (%10lx<->%10lx) %4d\n",
            kernel_boot_para_info->E820_Info.E820_Entry[i].address,
            kernel_boot_para_info->E820_Info.E820_Entry[i].length + kernel_boot_para_info->E820_Info.E820_Entry[i].address,
            kernel_boot_para_info->E820_Info.E820_Entry[i].type
        );
    }
    gBS->FreePool(MemMap);
    ERROR_BREAK(status, L"GetMemoryMap error3\n");
    return EFI_SUCCESS;
}

EFI_STATUS Load_File(EFI_HANDLE ImageHandle, int verbose)
{
    EFI_STATUS                          Status;
    UINTN                               HandleCount;
    EFI_HANDLE                          *DiskControllerHandles = NULL;
    EFI_DEVICE_PATH_PROTOCOL            *DiskDevicePath;
    EFI_DEVICE_PATH_TO_TEXT_PROTOCOL    *Device2TextProtocol = 0;
    CHAR16                              *TextDevicePath = 0;
    EFI_FILE_IO_INTERFACE               *Vol;
    EFI_FILE_HANDLE                     RootFs;
    EFI_FILE_HANDLE                     FileHandle;
    EFI_FILE_INFO                       *FileInfo;
    UINTN                               BufferSize = 0;
    EFI_PHYSICAL_ADDRESS                pages = 0x100000;
    int i;
    
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiDiskIoProtocolGuid, NULL, &HandleCount, &DiskControllerHandles);
    ERROR_BREAK(Status, L"LocateHandleBuffer for DiskControllerHandles error\n");
    
    Status = gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid,NULL,(VOID**)&Device2TextProtocol);
    ERROR_BREAK(Status, L"LocateProtocol for Device2TextProtocol error\n");
    
    for (i = 0; i < HandleCount; i++) {
        Status = gBS->OpenProtocol(DiskControllerHandles[i],&gEfiDevicePathProtocolGuid,(VOID**)&DiskDevicePath,ImageHandle,NULL,EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
        ERROR_BREAK(Status, L"Get DiskDevicePath protocol error\n");

        TextDevicePath = Device2TextProtocol->ConvertDevicePathToText(DiskDevicePath, TRUE, TRUE);        
        Print(L"%d: %s\n", i, TextDevicePath);
        if(TextDevicePath){
            gBS->FreePool(TextDevicePath);
        }

        Status = gBS->CloseProtocol(DiskControllerHandles[i],&gEfiDevicePathProtocolGuid,ImageHandle,NULL);
        ERROR_BREAK(Status, L"Close gEfiDevicePathProtocolGuid error: %d\n");
    }

    Status = gBS->OpenProtocol(DiskControllerHandles[KERNEL_DISK_NO],&gEfiSimpleFileSystemProtocolGuid,(VOID**)&Vol,ImageHandle,NULL,EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    ERROR_BREAK(Status,L"Simple file system protocol open failed!\n");
    
    Vol->OpenVolume(Vol,&RootFs);
    RootFs->Open(RootFs,&FileHandle,(CHAR16*)L"Kernel.bin",EFI_FILE_MODE_READ,0);
    BufferSize = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 100;
    
    Status = gBS->AllocatePool(EfiRuntimeServicesData,BufferSize,(VOID**)&FileInfo);
    ERROR_BREAK(Status,L"Allocate memory failed!\n");
    Status = FileHandle->GetInfo(FileHandle,&gEfiFileInfoGuid,&BufferSize,FileInfo);
    ERROR_BREAK(Status,L"Get file info failed!\n");
    
    Print(L"\tFileName:%s\t Size:%d\t FileSize:%d\t Physical Size:%d\n",
        FileInfo->FileName,
        FileInfo->Size,
        FileInfo->FileSize,
        FileInfo->PhysicalSize
    );

    Print(L"Read kernel file to memory\n");
    
    Status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, (FileInfo->FileSize + 0x1000 - 1) / 0x1000, &pages);
    ERROR_BREAK(Status,L"AllocatePages failed: %d\n");
    
    BufferSize = FileInfo->FileSize;
    FileHandle->Read(FileHandle,&BufferSize,(VOID*)pages);
    gBS->FreePool(FileInfo);
    FileHandle->Close(FileHandle);
    RootFs->Close(RootFs);

    // Print(L"%p\n",*((unsigned long*)pages + 1));
    
    Status = gBS->CloseProtocol(DiskControllerHandles[KERNEL_DISK_NO],&gEfiSimpleFileSystemProtocolGuid,ImageHandle,NULL);
    ERROR_BREAK(Status,L"Get file info failed!\n");

    return EFI_SUCCESS;
}

EFI_STATUS Locate_RSDP(EFI_SYSTEM_TABLE *SystemTable)
{
    int i;
    rsdp *RSDP;
    for(i = 0; i < SystemTable->NumberOfTableEntries; i++){
        if(CompareGuid(&gEfiAcpi20TableGuid,&(SystemTable->ConfigurationTable[i].VendorGuid))){
            RSDP = SystemTable->ConfigurationTable[i].VendorTable;
            kernel_boot_para_info->RSDP = *RSDP;
            Print(L"ACPI Version: %d, RSDT: %p, XSDT: %p\n", 
                kernel_boot_para_info->RSDP.Revision,
                kernel_boot_para_info->RSDP.RSDT,
                kernel_boot_para_info->RSDP.XSDT);
        }
    }
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle,IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS              Status;
    EFI_PHYSICAL_ADDRESS    pages = 0;
    UINTN                   MapKey = 0;
    UINTN MemMapSize = 0;
    EFI_MEMORY_DESCRIPTOR* MemMap = 0;
    UINTN DescriptorSize = 0;
    UINT32 DesVersion = 0;
    void                    (*func)(void);

    kernel_boot_para_info = (KERNEL_BOOT_PARAMETER_INFORMATION *)0x60000;
    pages = 0x60000;
    
    Status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, 1, &pages);
    ERROR_BREAK(Status,L"AllocatePages failed: %d\n");
    gBS->SetMem((void*)kernel_boot_para_info,0x1000,0);
    ERROR_BREAK(Status,L"SetMem failed: %d\n");

    Set_Video(1);
    Load_File(ImageHandle, 1);
    Locate_RSDP(SystemTable);
    Get_MemMap(1, &MapKey);
    // Set_Video(1);
    gBS->GetMemoryMap(&MemMapSize,MemMap,&MapKey,&DescriptorSize,&DesVersion);
    Status = gBS->ExitBootServices(ImageHandle,MapKey);
    ERROR_BREAK(Status, L"ExitBootServices: Failed, Memory Map has Changed.\n");
    func = (void *)0x100000;
    func();

    return EFI_SUCCESS;
}

