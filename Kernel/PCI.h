#include "lib.h"

#define CONFIG_ADDRESS  0xCF8
#define CONFIG_DATA     0xCFC

unsigned int PCI_CONF_Read(unsigned int bus, unsigned int slot, unsigned int func, unsigned int offset);
void Scan_PCI_Bus(unsigned int bus);
void Scan_Capabilities(int bus, int device, int function, int BaseClass, int SubClass);

static inline unsigned long PCIE_ADDR(unsigned int bus, unsigned int slot, unsigned int func, unsigned int offset)
{
    unsigned long i;
    
    if(ACPI.PCI_CONF_BASE){
        for(i=0;i<ACPI.No_PCI_Base;i++){
            if(ACPI.PCI_CONF_BASE[i].Start_Bus <= bus && ACPI.PCI_CONF_BASE[i].End_Bus){
                return ((ACPI.PCI_CONF_BASE[i].Base_Addr + PAGE_OFFSET) +
                    ((bus - ACPI.PCI_CONF_BASE[i].Start_Bus)<<20 | slot << 15 | func << 12 | (offset & 0xfc)));
            }
        }
    }

    return -1L;
}