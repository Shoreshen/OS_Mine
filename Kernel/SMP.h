#include "lib.h"

#define MSR_ICR             0x830
#define ICR_DEL_MODE_INIT   5
#define ICR_DEL_MODE_FIX    0
#define ICR_DEL_MODE_START  6
#define ICR_DES_MODE_PHY    0
#define ICR_DES_MODE_LOG    1
#define ICR_LEVEL_OTHER     1
#define ICR_LEVEL_DE_ASS    0
#define ICR_TRIG_EDGE       0
#define ICR_TRIG_LEVEL      1
#define ICR_SH_NONE         0
#define ICR_SH_SELF         1
#define ICR_SH_ALL          2
#define ICR_SH_OTHERS       3

typedef struct{
    unsigned long Vector    :8;
    unsigned long Del_Mode  :3;
    unsigned long Des_Mode  :1;
    unsigned long Del_Stat  :1;
    unsigned long zero0     :1;
    unsigned long Level     :1;
    unsigned long Trig_Mode :1;
    unsigned long zero1     :2;
    unsigned long SH        :2;
    unsigned long zero3     :12;
    union{
        struct{
            unsigned int zero4:24;
            unsigned int Des_Field:8;
        } apic_dest;
        unsigned int x2apic_dest :32; 
    }dest;
} icr;

void SMP_init();
void Start_SMP();