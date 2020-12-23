#include "ACPI.h"
#include "Mem.h"

void Init_ACPI(void)
{
    unsigned long i, j, k, SDT_PAGE;
    int checksum = 0;
    rsdp *RSDP;
    acpi_table *RSDT,*XSDT,*SDT;
    pg_attr ATTR;

	memset(&ATTR, 0, sizeof(pg_attr));
	ATTR.PML4E_Attr.RW 	= 1;
	ATTR.PML4E_Attr.P	= 1;
	ATTR.PDPTE_Attr.P	= 1;
	ATTR.PDPTE_Attr.RW	= 1;
	ATTR.PDE_Attr.PS	= 1;
	ATTR.PDE_Attr.P		= 1;
	ATTR.PDE_Attr.RW	= 1;

    RSDP = (rsdp *)&kernel_boot_para_info->RSDP;
    color_printk(BLACK, WHITE, "ACPI Version: %d\n", RSDP->Revision);
    
    checksum = 0;
    for (j = (unsigned long)RSDP; j < (unsigned long)RSDP + 20; j++)
    {
        checksum += (int)*((char *)j) & 0xff;
    }
    if(!(checksum & 0xff)){
        if(RSDP->Revision < 2){//RSDT
            RSDT = (acpi_table *)((unsigned long)RSDP->RSDT + PAGE_OFFSET);
            SDT_PAGE = (unsigned long)RSDT & PAGE_2M_MASK;
            color_printk(BLACK, WHITE, "RSDT: %p\n", RSDT);
            pagetable_init(PML4E, SDT_PAGE - PAGE_OFFSET, SDT_PAGE, 3, &ATTR, 0, 1);
            if (!(doChecksum(RSDT) & 0xff))
            {
                color_printk(BLACK, WHITE, "RSDT.Signiture: %c%c%c%c, RSDT.Length: %d\n", 
                    RSDT->Signature[0],RSDT->Signature[1],RSDT->Signature[2],RSDT->Signature[3], RSDT->Length);
                for(k = 0; k<((RSDT->Length - 36)/4); k++){
                    SDT = (acpi_table *)((unsigned long)RSDT->Entry.L32[k] + PAGE_OFFSET);
                    ConfigSDT(SDT);
                    color_printk(BLACK, WHITE, "RSDT.Entry: %p, Signiture: %c%c%c%c\n", RSDT->Entry.L32[k],
                        SDT->Signature[0],SDT->Signature[1],SDT->Signature[2],SDT->Signature[3]);
                }
            }
        }
        else{//XSDT
            checksum = 0;
            for(j = (unsigned long)RSDP + 20; j < (unsigned long)RSDP + sizeof(rsdp); j++){
                checksum += (int)*((char *)j) & 0xff;
            }
            color_printk(BLACK, WHITE, "XSDT: %p\n", RSDP->XSDT);
            if(!(checksum & 0xff)){
                XSDT = (acpi_table *)((unsigned long)RSDP->XSDT + PAGE_OFFSET);
                SDT_PAGE = (unsigned long)XSDT & PAGE_2M_MASK;
                color_printk(BLACK, WHITE, "XSDT: %p\n", XSDT);
                pagetable_init(PML4E, SDT_PAGE - PAGE_OFFSET, SDT_PAGE, 3, &ATTR, 0, 1);
                if(!(doChecksum(XSDT) & 0xff)){
                    color_printk(BLACK, WHITE, "XSDT.Signiture: %c%c%c%c, XSDT.Length: %d\n", 
                        XSDT->Signature[0],XSDT->Signature[1],XSDT->Signature[2],XSDT->Signature[3], XSDT->Length);
                    for(k = 0; k<((XSDT->Length - 36)/8); k++){
                        SDT = (acpi_table *)((unsigned long)XSDT->Entry.L64[k] + PAGE_OFFSET);
                        color_printk(BLACK, WHITE, "XSDT.Entry1: %p, Signiture: %c%c%c%c\n", XSDT->Entry.L64[k],
                            SDT->Signature[0],SDT->Signature[1],SDT->Signature[2],SDT->Signature[3]);
                        ConfigSDT(SDT);                                
                    }
                }
            }
            else{
                color_printk(BLACK, WHITE, "Checksum 2.0 invalid!\n");
            }
        }
    }
    else{
        color_printk(BLACK, WHITE, "Checksum 1.0 invalid!\n");
    }
}

void ConfigSDT(acpi_table *SDT)
{
    if(1128878145 == (*((unsigned int *)&SDT->Signature[0]))){
        Init_MADT(SDT);
    }else if(1195787085 == (*((unsigned int *)&SDT->Signature[0]))){
        Init_MCFG(SDT);
    }else if(1413828680 == (*((unsigned int *)&SDT->Signature[0]))){
        Init_HPET(SDT);
    }
}

void Init_MADT(acpi_table *SDT)
{
    char *temp_ptr, *End;
    if (!(doChecksum(SDT) & 0xff)){
        temp_ptr    = (char *)&SDT->Entry.L32[2];
        End         = (char *)((unsigned long)SDT + SDT->Length);
        ACPI.MADT   = SDT;
        ACPI.LAPIC_BASE = &SDT->Entry.L32[0];
        while(temp_ptr < End){
            if(MACRO_PRINT){
                color_printk(YELLOW,BLACK,"Type: %02X, Length: %d", temp_ptr[0], temp_ptr[1]);
                switch (temp_ptr[0])
                {
                case 2:
                    color_printk(YELLOW,BLACK,", Bus source: %d, IRQ source: %d", temp_ptr[2], temp_ptr[3]);
                    break;
                default:
                    break;
                }
                color_printk(YELLOW,BLACK,"\n", temp_ptr[0], temp_ptr[1]);
            }
            temp_ptr = temp_ptr + temp_ptr[1];
            if(temp_ptr[0] == 1){
                ACPI.IOAPIC_BASE = (unsigned int *)(temp_ptr + 4);
            }
        }
        ACPI.LAPIC_VIRT_BASE = *ACPI.LAPIC_BASE + PAGE_OFFSET;
        ACPI.IOAPIC_VIRT_BASE = *ACPI.IOAPIC_BASE + PAGE_OFFSET;
        if(MACRO_PRINT){
            color_printk(YELLOW,BLACK, "Local APIC base: 0x%08X, I/O APIC base: %p, Flag: 0x%X\n", *ACPI.LAPIC_BASE, *ACPI.IOAPIC_BASE, SDT->Entry.L32[1]);
            color_printk(YELLOW,BLACK, "LAPIC_VIRT_BASE: %p\nIOAPIC_VIRT_BASE: %p\n", ACPI.LAPIC_VIRT_BASE, ACPI.IOAPIC_VIRT_BASE);
        }
    }
}

void Init_MCFG(acpi_table *SDT)
{
    char *temp_ptr, *End;
    unsigned long ADDR;
    pg_attr ATTR;
    memset(&ATTR, 0, sizeof(pg_attr));
    ATTR.PML4E_Attr.RW 	= 1;
    ATTR.PML4E_Attr.P	= 1;
    ATTR.PDPTE_Attr.P	= 1;
    ATTR.PDPTE_Attr.RW	= 1;
    ATTR.PDE_Attr.PS	= 1;
    ATTR.PDE_Attr.P		= 1;
    ATTR.PDE_Attr.RW	= 1;  
    
    if (!(doChecksum(SDT) & 0xff)){
        temp_ptr    = (char *)&SDT->Entry.L64[1];
        End         = (char *)((unsigned long)SDT + SDT->Length);
        ACPI.MCFG   = SDT;
        ACPI.PCI_CONF_BASE = (pci_conf_base *)temp_ptr;
        while(temp_ptr < End){
            pagetable_init(PML4E, 
                ACPI.PCI_CONF_BASE[ACPI.No_PCI_Base].Base_Addr, 
                ACPI.PCI_CONF_BASE[ACPI.No_PCI_Base].Base_Addr + PAGE_OFFSET, 
                (ACPI.PCI_CONF_BASE[ACPI.No_PCI_Base].End_Bus - ACPI.PCI_CONF_BASE[ACPI.No_PCI_Base].Start_Bus)/2 + 1, 
                &ATTR, 0, 1);
            temp_ptr += 16;
            if(MACRO_PRINT){
                color_printk(YELLOW,BLACK,"Base_Addr: %p, PCI_Seg_Grp: %d, Start_PCI_Bus: %d, End_PCI_Bus:%d\n", 
                    ACPI.PCI_CONF_BASE[ACPI.No_PCI_Base].Base_Addr,
                    ACPI.PCI_CONF_BASE[ACPI.No_PCI_Base].PCI_Seg_Grp,
                    ACPI.PCI_CONF_BASE[ACPI.No_PCI_Base].Start_Bus,
                    ACPI.PCI_CONF_BASE[ACPI.No_PCI_Base].End_Bus
                );  
                color_printk(YELLOW,BLACK,"First 8 byte: %p, Virtual addr: %p\n", 
                    *((unsigned long *)(ACPI.PCI_CONF_BASE[ACPI.No_PCI_Base].Base_Addr + PAGE_OFFSET)),
                    ACPI.PCI_CONF_BASE[ACPI.No_PCI_Base].Base_Addr + PAGE_OFFSET
                );
            }
            ACPI.No_PCI_Base++;
        }
        ADDR = ACPI.PCI_CONF_BASE[0].Base_Addr + PAGE_OFFSET; 
    }else{
        color_printk(YELLOW,BLACK,"Checksum invalid: %d", doChecksum(SDT));
    }
}

void Init_HPET(acpi_table *SDT)
{
    int i;

    if (!(doChecksum(SDT) & 0xff)){
        ACPI.No_HPET    = (SDT->Length-ACPI_HEAD_LEN)/sizeof(hpet);
        ACPI.HPET       = (hpet *)&SDT->Entry.L64[0];
        ACPI.HPET_REG   = (hpet_reg *)(ACPI.HPET->address.address + PAGE_OFFSET);
        if(MACRO_PRINT){
            color_printk(YELLOW,BLACK,"Totoal HPED entry: %d\n", ACPI.No_HPET);
            for(i=0; i<ACPI.No_HPET; i++){
                color_printk(YELLOW,BLACK,"Entry[%d].Address_space_ID: %d\n", i, ACPI.HPET[i].address.address_space_id);
                color_printk(YELLOW,BLACK,"Entry[%d].Address: %p\n", i, ACPI.HPET[i].address.address);
                color_printk(YELLOW,BLACK,"Entry[%d].register_bit_width: %d\n", i, ACPI.HPET[i].address.register_bit_width);
                color_printk(YELLOW,BLACK,"Entry[%d].register_bit_offset: %d\n", i, ACPI.HPET[i].address.register_bit_offset);
                color_printk(YELLOW,BLACK,"Entry[%d].comparator_count: %d\n", i, ACPI.HPET[i].comparator_count);
            }
            color_printk(YELLOW,BLACK,"HPET_VIRT_BASE: %p\n", (unsigned long)ACPI.HPET_REG);
        }
    }
}

unsigned int doChecksum(acpi_table *tableHeader)
{
    unsigned int sum = 0;
    for (int i = 0; i < tableHeader->Length; i++)
    {
        sum += ((char *) tableHeader)[i];
    }
    return sum;
}