// Large, Special Boys
//    Kane Penley
//    Nathan Orick


#include "mmu.h"

int pagingIsEnabled(CPU *cpu);
ADDRESS getBaseAddress(ADDRESS);
int isPresent(ADDRESS cur);
int get7thBit(unsigned long te);


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
    if(!pagingIsEnabled(cpu))
        return virt;

	//Implement the logic to walk the page tables to convert
	//the virt address into a physical one.
	//If the page entry doesn't exist (present = 0), then
	//return RET_PAGE_FAULT to signal a page fault
    
    // We get the pml4 address from the cr3, and it's offset from the virtual address.
    ADDRESS pml4 = getBaseAddress(cpu->cr3) + ((virt >> 39) & 0x1fful);
    unsigned long pml4e = cpu->memory[pml4]; //pml4 entry
    if(!isPresent(pml4e)) {
        cpu->cr2 = virt;
        return RET_PAGE_FAULT;
    }

    // Get pdp from pml4e. If it's entry's 7th bit is 0, go straight to physical page (These are 1Gb pages).
    // If it's 7th bit is 0, it's either 4Kb or 2Mb. Keep going.
    ADDRESS pdp = getBaseAddress(pml4e) + ((virt >> 30) & 0x1fful);
    unsigned long pdpe = cpu->memory[pdp];
    if(!isPresent(pdpe)) {
        cpu->cr2 = virt;
        return RET_PAGE_FAULT;
    }
    if(get7thBit(pdpe) == 0)
        return getBaseAddress(pdpe) + (virt & 0x3ffffffful); // 30 1's.

    // Get pd from pdpe. If it's entry's 7th bit is 0, go straight to physical page (These are 2Mb pages).
    // If it's 7th bit is 0, it's either 4Kb. Keep going.
    ADDRESS pd = getBaseAddress(pdpe) + ((virt >> 21) & 0x1fful);
    unsigned long pde = cpu->memory[pd];
    if(!isPresent(pde)) {
        cpu->cr2 = virt;
        return RET_PAGE_FAULT;
    }
    if(get7thBit(pde) == 0)
        return getBaseAddress(pde) + (virt & 0x1ffffful); // 21 1's.

    // Get pt from pde. These are 4Kb tables. Go straight to physical page from here.
    // We don't have to check the 7th bit.
    ADDRESS pt = getBaseAddress(pde) + ((virt >> 12) & 0x1fful);
    unsigned long pte = cpu->memory[pt];
    if(!isPresent(pte)) {
        cpu->cr2 = virt;
        return RET_PAGE_FAULT;
    }
    return getBaseAddress(pte) + (virt & 0xffful); // 12 1's.
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



/********************
 * Helper Functions *
 *******************/


// Returns 1 if paging is enabled, 0 otherwise.
int pagingIsEnabled(CPU *cpu) {
    return (cpu->cr0 >> 31) & 1;
}

// Gets the base address of the next table given the table entry te.
ADDRESS getBaseAddress(unsigned long te) {
    // Has to be a ul literal, else the casting causes some crazy shit.
    return (te & 0x000ffffffffff000ul); // 10 f's. 
}

// Returns 1 if the present bit (0) is set at the table entry te, and 0 otherwise.
int isPresent(unsigned long te) {
    return te & 1ul;
}

// Returns the 7th bit of the table entry te. Remember that the bits are 0-based.
int get7thBit(unsigned long te) {
    return (te >> 7) & 0x1ul;
}
