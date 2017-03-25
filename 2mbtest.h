/******************************************
 * COSC 361 Sp2017 MMU Project Test File  *
 * Stephen Marz                           *
 * 13 Dec 2016                            *
 ******************************************/
#include "mmu.h"
#include <stdio.h>
#include "unitTests.h"

int test2mb()
{
	CPU *cpu;

	//1 << 18 is about 262,000 bytes
	cpu = new_cpu(1 << 30);

	//Maps physical address 0x00004000 to
	//virtual address 0x00001000 using
	//4kilobyte pages
	map(cpu, 0x004000000, 0x01000, PS_2M);

	//Set the value at virtual address
	//0x00004000 to 12345
	mem_set(cpu, 0x00004f00000, 12345);

    printf("12345 - ");
	printf("Value at 0x00004f00000 = %d\n", mem_get(cpu, 0x00004f00));

	//Above should ONLY deal with physical addresses since MMU
	//is OFF!

	//Enable the MMU
	cpu->cr0 |= 1 << 31;

	//This should page fault since virtual address 0x00004000 doesn't
	//exist
    printf("Page fault - ");
	printf("Value at 0x00004000000 = %d\n", mem_get(cpu, 0x00004000000));

	//This should print 12345 since 0x00001f00 maps to physical address //0x00004000 which we set to 12345 above.
    printf("12345 - ");
	printf("Value at 0x00001f00 = %d\n", mem_get(cpu, 0x00001f00));

#if defined(I_DID_EXTRA_CREDIT)
	cpu->tlb_policy = RP_CLOCK;
	mem_get(cpu, 0x00001f00);
	//Print out the TLB entries
	print_tlb(cpu);
#endif

	//Free all resources here
	destroy_cpu(cpu);

	return 0;
}
