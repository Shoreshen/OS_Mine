#include "PCI.h"

unsigned int PCI_CONF_Read(unsigned int bus, unsigned int slot, unsigned int func, unsigned int offset)
{
    unsigned int address;
    unsigned int tmp = 0, i;
    
    if(ACPI.PCI_CONF_BASE){
        for(i=0;i<ACPI.No_PCI_Base;i++){
            if(ACPI.PCI_CONF_BASE[i].Start_Bus <= bus && ACPI.PCI_CONF_BASE[i].End_Bus){
                tmp = *(unsigned int *)((ACPI.PCI_CONF_BASE[i].Base_Addr + PAGE_OFFSET) +
                    ((bus - ACPI.PCI_CONF_BASE[i].Start_Bus)<<20 | slot << 15 | func << 12 | (offset & 0xfc)));
            }
        }
    }else{
        address = (unsigned int)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc) | ((unsigned int)0x80000000));
        io_out32(0xCF8, address);
        tmp = io_in32(0xCFC);
    }
    return (tmp);
}

void Scan_PCI_Bus(unsigned int bus)
{
    unsigned int device = 0, function = 0, Header, VenderID, DeviceID, SecondBus, BaseClass, SubClass, Int_Pin, Int_Line, Status;
    
    for(device = 0; device < 32; device++){

        VenderID = PCI_CONF_Read(bus, device, function, 0x00);
        DeviceID = VenderID >> 16;
        VenderID = VenderID & 0xffff;

        if(VenderID != 0xffff){

            Header = (PCI_CONF_Read(bus, device, function, 0x0C) & 0xff0000) >> 16;
            BaseClass = PCI_CONF_Read(bus, device, function, 0x08) >> 16;
            SubClass = BaseClass & 0xff;
            BaseClass = BaseClass >> 8;

            if((BaseClass == 0x06) && (SubClass == 0x04)){
                SecondBus = (PCI_CONF_Read(bus, device, function, 0x18) >> 8) & 0xff;
                Scan_PCI_Bus(SecondBus);
            }else{
                if(Header & 0x80){
                    for(function = 0; function < 8; function++){
                        VenderID = PCI_CONF_Read(bus, device, function, 0x00);
                        DeviceID = VenderID >> 16;
                        VenderID = VenderID & 0xffff;
                        if(VenderID != 0xffff){
                            Header = (PCI_CONF_Read(bus, device, function, 0x0C) & 0xff0000) >> 16;
                            BaseClass = PCI_CONF_Read(bus, device, function, 0x08) >> 16;
                            SubClass = BaseClass & 0xff;
                            BaseClass = BaseClass >> 8;
                            Int_Pin = PCI_CONF_Read(bus, device, function, 0x3C) & 0xffff;
                            Int_Line = Int_Pin & 0xff;
                            Int_Pin = Int_Pin >> 8;
                            Status = PCI_CONF_Read(bus, device, function, 0x04) >> 16;
                            if(MACRO_PRINT){
                                color_printk(WHITE, BLACK, "BDF: %d|%d|%d, Header: 0x%02x, VenderID: 0x%04X, Device ID: " 
                                    "0x%04X, Class_Code: 0x%02X, Sub_Class Code: 0x%02X, Int_Pin: 0x%02X, Int_Line: 0x%02X\n",
                                    bus, device, function, Header, VenderID, DeviceID, BaseClass, SubClass, Int_Pin, Int_Line);
                            }
                            if(Status & 1<<4){
                                Scan_Capabilities(bus,device,function,BaseClass,SubClass);
                            }
                        }
                    }
                }else{
                    Int_Pin = PCI_CONF_Read(bus, device, function, 0x3C) & 0xffff;
                    Int_Line = Int_Pin & 0xff;
                    Int_Pin = Int_Pin >> 8;
                    Status = PCI_CONF_Read(bus, device, function, 0x04) >> 16;
                    if(MACRO_PRINT){
                        color_printk(WHITE, BLACK, "BDF: %d|%d|%d, Header: 0x%02x, VenderID: 0x%04X, Device ID: "
                            "0x%04X, Class_Code: 0x%02X, Sub_Class Code: 0x%02X, Int_Pin: 0x%02X, Int_Line: 0x%02X\n",
                            bus, device, function, Header, VenderID, DeviceID, BaseClass, SubClass, Int_Pin, Int_Line);
                    }
                    if(Status & 1<<4){
                        Scan_Capabilities(bus,device,function,BaseClass,SubClass);
                    }
                }
            }
        }
    }
}

void Scan_Capabilities(int bus, int device, int function, int BaseClass, int SubClass)
{
    int Cap_Ptr, Cap_Ver;
    unsigned long Cap_Addr;
    Cap_Ptr = PCI_CONF_Read(bus, device, function, 0x34) & 0xff;
    //Qemu defualtly enable Bus Master on PCI.CMD[2] bit, which is offset 0x04 register
    while(Cap_Ptr!=0){
        Cap_Addr = (unsigned long)PCIE_ADDR(bus, device, function, Cap_Ptr);
        Cap_Ptr = PCI_CONF_Read(bus, device, function, Cap_Ptr);
        Cap_Ver = Cap_Ptr & 0xff;
        Cap_Ptr = (Cap_Ptr & 0xffff) >> 8;
        if(MACRO_PRINT){
            color_printk(WHITE,BLACK, "Cap_Ver: 0x%02X, Next Cap_Ptr: 0x%02X Cap_Addr: %p\n", Cap_Ver, Cap_Ptr, Cap_Addr);
        }
        if(Cap_Ver==0x05 && BaseClass == 0x01 && SubClass == 0x06){//AHCI controller
            AHCI.MSI._64 = (msi_64*)Cap_Addr;
            AHCI.ABAR = (HBA_MEM *)((unsigned long)PCI_CONF_Read(bus, device, function, 0x24) + PAGE_OFFSET);
            if(MACRO_PRINT){
                color_printk(BLUE,BLACK, "MCR:     0x%04X\n", AHCI.MSI._64->MCR);
                color_printk(BLUE,BLACK, "MAR low: 0x%08X\n", AHCI.MSI._64->MAR);
                color_printk(BLUE,BLACK, "MAR up:  0x%08X\n", AHCI.MSI._64->MAR_up);
                color_printk(BLUE,BLACK, "MDR:     0x%04X\n", AHCI.MSI._64->MDR);
                color_printk(BLUE,BLACK, "AHCI.ABAR: %p\n",AHCI.ABAR);
            }
        } else if(Cap_Ver==0x11 && BaseClass == 0x01 && SubClass == 0x08) {
            //BIT 0:13 Excluded in MBAR
            NVME.MBAR = (mbar *)((*(unsigned long *)PCIE_ADDR(bus, device, function, 0x10) + PAGE_OFFSET) & 0xFFFFFFFFFFFFF000);
            NVME.MSIX = (msi_x *)Cap_Addr;
            NVME.MSIXTBL = (msixtbl *)(*(unsigned long *)PCIE_ADDR(bus, device, function, 0x10 + 4 * NVME.MSIX->TlbBIR) + 
                (NVME.MSIX->TblOff << 3) + PAGE_OFFSET) + NVME.MSIX->TblOff;
            if(MACRO_PRINT){
                color_printk(PURPLE,BLACK,"Table BIR: 0x%X, Table Offset: 0x%X,Cap_Ver: 0x%X, Next Cap_Ptr: 0x%X, NVME.MSIXTBL: %p\n",
                    NVME.MSIX->TlbBIR,
                    NVME.MSIX->TblOff,
                    NVME.MSIX->Cap_ID,
                    NVME.MSIX->Next_Ptr,
                    NVME.MSIXTBL
                );
                color_printk(PURPLE,BLACK,"NVME.MBAR: %p\n",NVME.MBAR);
                color_printk(PURPLE,BLACK, "Table size:%d, Func Mask: %d, Enable: %d, Table BIR: 0x%02X Table Off: %p\n", 
		            NVME.MSIX->Tbl_Size, NVME.MSIX->Fnc_Msk, NVME.MSIX->Enable, NVME.MSIX->TlbBIR, NVME.MSIX->Tbl_Size << 3);
            }
        }
    }
}