// Large, Special Boys
//    Kane Penley
//    Nathan Orick

#include "mmu.h"

ADDRESS getBaseAddress(ADDRESS);
int isPresent(ADDRESS cur);
int get7thBit(unsigned long te);
int addTable(ADDRESS* entry, CPU *cpu);

#define NULL_ADDRESS	(0ul)
#define PHYS_MASK	(0xffful)
#define ENTRY_MASK	(0x1fful)
#define GB_MASK		(0x3ffffffful)
#define MB_MASK		(0x1ffffful) //2fffful -Marz
#define TOP (*((ADDRESS*)cpu->memory[cpu->mem_size / 2])) // Pointer to the top of the stack.

void StartNewPageTable(CPU *cpu)
{
	//Create cr3 all the way!

	//Take memory 1/2 up:
    //Store the pointer to the top of the stack at the first block of memory.
    TOP = (ADDRESS)&cpu->memory[cpu->mem_size / 2];
    TOP += 1; // Move the top of stack past the top pointer.

    // Align top to a 4k boundary.
    TOP = (ADDRESS)TOP + 0x1000 - ((ADDRESS)TOP & 0xfff);

    // Bits 63-52 must be 0.
    cpu->cr3 = NULL_ADDRESS;
    addTable(&cpu->cr3, cpu);
}

//Write:
// virt_to_phys
// map
// unmap
ADDRESS virt_to_phys(CPU *cpu, ADDRESS virt)
{
	//Convert a physical address *virt* to
	//a physical address.

	//If the MMU is off, simply return the virtual address as
	//the physical address
	if (!(cpu->cr0 & (1 << 31))) {
		return virt;
	}
	if (cpu->cr3 == 0) {
		return RET_PAGE_FAULT;
	}

    // We get the pml4 address from the cr3, and it's offset from the virtual address.
    ADDRESS *pml4 = (ADDRESS*)(getBaseAddress(cpu->cr3));
    ADDRESS pml4e = *(pml4 + ((virt >> 39) & ENTRY_MASK)); // Pointer arithmetic.
    if(!isPresent(pml4e))
        return RET_PAGE_FAULT;

    // Get pdp from pml4e. If it's entry's 7th bit is 0, go straight to physical page (These are 1Gb pages).
    // If it's 7th bit is 0, it's either 4Kb or 2Mb. Keep going.
    ADDRESS *pdp = (ADDRESS*)(getBaseAddress(pml4e));
    ADDRESS pdpe = *(pdp + ((virt >> 30) & ENTRY_MASK));
    if(get7thBit(pdpe) == 0)
        return *(((ADDRESS*)getBaseAddress(pdpe)) + (virt & GB_MASK));
    if(!isPresent(pdpe))
        return RET_PAGE_FAULT;

    // Get pd from pdpe. If it's entry's 7th bit is 0, go straight to physical page (These are 2Mb pages).
    // If it's 7th bit is 0, it's either 4Kb. Keep going.
    ADDRESS *pd = (ADDRESS*)getBaseAddress(pdpe);
    ADDRESS pde = *(pd + ((virt >> 21) & ENTRY_MASK));
    if(get7thBit(pde) == 0)
        return *(((ADDRESS*)getBaseAddress(pde)) + (virt & MB_MASK));
    if(!isPresent(pde))
        return RET_PAGE_FAULT;

    // Get pt from pde. These are 4Kb tables. Go straight to physical page from here.
    // We don't have to check the 7th bit.
    ADDRESS *pt = (ADDRESS*)getBaseAddress(pde);
    ADDRESS pte = *(pt + ((virt >> 12) & ENTRY_MASK));
    if(!isPresent(pte))
        return RET_PAGE_FAULT;
    return *(((ADDRESS*)getBaseAddress(pte)) + (virt & PHYS_MASK));
}

void map(CPU *cpu, ADDRESS phys, ADDRESS virt, PAGE_SIZE ps)
{
	//ALL PAGE MAPS must be located in cpu->memory!!

	//This function will return the PML4 of the mapped address

	//If the page size is 1G, you need a valid PML4 and PDP
	//If the page size is 2M, you need a valid PML4, PDP, and PD
	//If the page size is 4K, you need a valid PML4, PDP, PD, and PT
	//Remember that I could have some 2M pages and some 4K pages with a smattering
	//of 1G pages!

	if (cpu->cr3 == 0) {
		//Nothing has been created, so start here
		StartNewPageTable(cpu);
	}


/*
	ADDRESS pml4e = (virt >> 39) & ENTRY_MASK;
	ADDRESS pdpe = (virt >> 30) & ENTRY_MASK;
	ADDRESS pde = (virt >> 21) & ENTRY_MASK;
	ADDRESS pte = (virt >> 12) & ENTRY_MASK;
	ADDRESS p = virt & PHYS_MASK;

	ADDRESS *pml4 = (ADDRESS *)cpu->cr3;
	ADDRESS *pdp;
	ADDRESS *pd;
	ADDRESS *pt;
*/
}

void unmap(CPU *cpu, ADDRESS virt, PAGE_SIZE ps)
{
	//Simply set the present bit (P) to 0 of the virtual address page
	//If the page size is 1G, set the present bit of the PDP to 0
	//If the page size is 2M, set the present bit of the PD  to 0
	//If the page size is 4K, set the present bit of the PT  to 0


	if (cpu->cr3 == 0)
		return;

}



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


	ret = (CPU*)calloc(1, sizeof(CPU));
	ret->memory = (char*)calloc(mem_size, sizeof(char));
	ret->mem_size = mem_size;

#if defined(I_DID_EXTRA_CREDIT)
	ret->tlb = (TLB_ENTRY*)calloc(TLB_SIZE, sizeof(TLB_ENTRY));
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

// Adds a new table and points to it from entry. Returns the address of the new table or 0 on failure.
int addTable(ADDRESS* entry, CPU *cpu) {
    ADDRESS p = TOP;
    TOP += 512;
    if(TOP > (ADDRESS)&cpu->memory[cpu->mem_size - 1])
        return 0;

    *entry |= p; // Sets 51-12 to p. 11-0 are all 0 because p is 4k aligned.
    *entry |= 0x1ul; // Sets present bit 1.
    *entry &= ~(0x1ul << 7); // Sets bit 7 to 0.


    // Mark every entry in the new table as being not present.
    int i;
    ADDRESS *q = (ADDRESS*)p;
    for(i = 0; i < 512; q++, i++) {
        *q &= ~(0x1ul); 
    }
    
    return p;
}
