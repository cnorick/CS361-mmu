//Put your project groups' member's name here!
//My Group Name
//John A. Student
//Jane B. Student
#include "mmu.h"

//Write:
// virt_to_phys
// map
// unmap
ADDRESS virt_to_phys(CPU *cpu, ADDRESS virt)
{
	//If you did the extra credit, your TLB
	//logic goes here. Also, uncomment the I_DID_EXTRA_CREDIT
	//define inside of mmu.h
	//Make sure that you consult your TLB before you walk the
	//pages. That's the whole point!


	//Only translate the pages if the MMU is on!!! Otherwise
	//return virt as the physical address!
	return virt;

	//Implement the logic to walk the page tables to convert
	//the virt address into a physical one.


	//If the page entry doesn't exist (present = 0), then
	//return RET_PAGE_FAULT to signal a page fault
}

void map(CPU *cpu, ADDRESS phys, ADDRESS virt, PAGE_SIZE ps)
{
	//ALL PAGE MAPS must be located in cpu->memory!!

	//If the page size is 1G, you need a valid PML4 and PDP
	//If the page size is 2M, you need a valid PML4, PDP, and PD
	//If the page size is 4K, you need a valid PML4, PDP, PD, and PT
	//Remember that I could have some 2M pages and some 4K pages with a smattering
	//of 1G pages!

}

void unmap(CPU *cpu, ADDRESS virt, PAGE_SIZE ps)
{
	//Simply set the present bit (P) to 0 of the virtual address page
	//If the page size is 1G, set the present bit of the PDP to 0
	//If the page size is 2M, set the present bit of the PD  to 0
	//If the page size is 4K, set the present bit of the PT  to 0
	//To avoid eating a lot of memory, when you map a new page that
	//doesn't already have an entry, use a table that has P = 0.

}

//////////////////////////////////////////
//
//Write your extra credit function invlpg
//below
//
//////////////////////////////////////////

#if defined(I_DID_EXTRA_CREDIT)
void invlpg(CPU *cpu, ADDRESS virt)
{

}
#endif


//////////////////////////////////////////
//
//Do not touch the functions below!
//
//////////////////////////////////////////
void cpu_pfault(CPU *cpu)
{
	printf("MMU: #PF @ 0x%016lx\n", cpu->cr2);
}


CPU *new_cpu(unsigned int mem_size)
{
	CPU *ret;


	ret = calloc(1, sizeof(CPU));
	ret->memory = calloc(mem_size, sizeof(char));
	ret->mem_size = mem_size;

#if defined(I_DID_EXTRA_CREDIT)
	ret->tlb = calloc(TLB_SIZE, sizeof(TLB_ENTRY));
#endif

	return ret;
}

void destroy_cpu(CPU *cpu)
{
	if (cpu->memory) {
		free(cpu->memory);
	}
#if defined(I_DID_EXTRA_CREDIT)
	if (cpu->tlb) {
		free(cpu->tlb);
	}
	cpu->tlb = 0;
#endif

	cpu->memory = 0;

	free(cpu);
}


int mem_get(CPU *cpu, ADDRESS virt)
{
	ADDRESS phys = virt_to_phys(cpu, virt);
	if (phys == RET_PAGE_FAULT || phys + 4 >= cpu->mem_size) {
		cpu->cr2 = virt;
		cpu_pfault(cpu);
		return -1;
	}
	else {
		return *((int*)(&cpu->memory[phys]));
	}
}

void mem_set(CPU *cpu, ADDRESS virt, int value)
{
	ADDRESS phys = virt_to_phys(cpu, virt);
	if (phys == RET_PAGE_FAULT || phys + 4 >= cpu->mem_size) {
		cpu->cr2 = virt;
		cpu_pfault(cpu);
	}
	else {
		*((int*)(&cpu->memory[phys])) = value;
	}
}

#if defined(I_DID_EXTRA_CREDIT)

void print_tlb(CPU *cpu)
{
	int i;
	TLB_ENTRY *entry;

	printf("#   %-18s %-18s Tag\n", "Virtual", "Physical");
	for (i = 0;i < TLB_SIZE;i++) {
		entry = &cpu->tlb[i];
		printf("%-3d 0x%016lx 0x%016lx %08x\n", i+1, entry->virt, entry->phys, entry->tag);
	}
}

#endif
