#include "lib.h"

#define ACPI_HEAD_LEN 36

void Init_ACPI(void);
unsigned int doChecksum(acpi_table *tableHeader);
void Init_MADT(acpi_table *SDT);
void Init_MCFG(acpi_table *SDT);
void Init_HPET(acpi_table *SDT);
void ConfigSDT(acpi_table *SDT);