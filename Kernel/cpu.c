#include "cpu.h"

void init_cpu(void)
{
	int i,j;
	unsigned int CpuFacName[4] = {0,0,0,0};
	char	FactoryName[17] = {0};

	//vendor_string
	get_cpuid(0,0,&CpuFacName[0],&CpuFacName[1],&CpuFacName[2],&CpuFacName[3]);

	*(unsigned int*)&FactoryName[0] = CpuFacName[1];

	*(unsigned int*)&FactoryName[4] = CpuFacName[3];

	*(unsigned int*)&FactoryName[8] = CpuFacName[2];	

	FactoryName[12] = '\0';
	if(MACRO_PRINT){
		color_printk(YELLOW,BLACK,"%s\t%8p\t%8p\t%8p\n",FactoryName,CpuFacName[1],CpuFacName[3],CpuFacName[2]);
	}
	
	//brand_string
	for(i = 0x80000002;i < 0x80000005;i++)
	{
		get_cpuid(i,0,&CpuFacName[0],&CpuFacName[1],&CpuFacName[2],&CpuFacName[3]);

		*(unsigned int*)&FactoryName[0] = CpuFacName[0];

		*(unsigned int*)&FactoryName[4] = CpuFacName[1];

		*(unsigned int*)&FactoryName[8] = CpuFacName[2];

		*(unsigned int*)&FactoryName[12] = CpuFacName[3];

		FactoryName[16] = '\0';
		if(MACRO_PRINT){
			color_printk(YELLOW,BLACK,"%s",FactoryName);
		}
	}
	if(MACRO_PRINT){
		color_printk(YELLOW,BLACK,"\n");
	}
	//Version Informatin Type,Family,Model,and Stepping ID
	get_cpuid(1,0,&CpuFacName[0],&CpuFacName[1],&CpuFacName[2],&CpuFacName[3]);
	if(MACRO_PRINT){
		color_printk(YELLOW,BLACK,"Family Code:%08p\t\tExtended Family:%08p\tModel Number:%08p\nExtended Model:%08p\tProcessor Type:%08p\tStepping ID:%08p\n",
			(CpuFacName[0] >> 8 & 0xf),
			(CpuFacName[0] >> 20 & 0xff),
			(CpuFacName[0] >> 4 & 0xf),
			(CpuFacName[0] >> 16 & 0xf),
			(CpuFacName[0] >> 12 & 0x3),
			(CpuFacName[0] & 0xf));
	}

	//get Linear/Physical Address size
	get_cpuid(0x80000008,0,&CpuFacName[0],&CpuFacName[1],&CpuFacName[2],&CpuFacName[3]);
	if(MACRO_PRINT){
		color_printk(YELLOW,BLACK,"Physical Address size:%08d\nLinear Address size:%08d\n",(CpuFacName[0] & 0xff),(CpuFacName[0] >> 8 & 0xff));
	}

	//max cpuid operation code
	get_cpuid(0,0,&CpuFacName[0],&CpuFacName[1],&CpuFacName[2],&CpuFacName[3]);
	if(MACRO_PRINT){
		color_printk(WHITE,BLACK,"MAX Basic Operation Code :%08p\t",CpuFacName[0]);
	}
	get_cpuid(0x80000000,0,&CpuFacName[0],&CpuFacName[1],&CpuFacName[2],&CpuFacName[3]);
	if(MACRO_PRINT){
		color_printk(WHITE,BLACK,"MAX Extended Operation Code :%08p\n",CpuFacName[0]);
	}
	#pragma region PAT
	get_cpuid(0x01,0,&CpuFacName[0],&CpuFacName[1],&CpuFacName[2],&CpuFacName[3]);
	if(MACRO_PRINT){
		color_printk(WHITE,BLACK,"CR0: %p\n",get_cr0());
		if(CpuFacName[3] &1 << 16){
			color_printk(WHITE,BLACK,"Support PAT!\n",CpuFacName[0]);
			color_printk(WHITE,BLACK,"IA32_PAT: %p\n",rdmsr(0x277));
			color_printk(WHITE,BLACK,"IA32_MTRR_DEF_TYPE: %p\n",rdmsr(0x2ff));
		} else {
			color_printk(WHITE,BLACK,"Do not support PAT!\n",CpuFacName[0]);
		}
	}
	#pragma endregion
}