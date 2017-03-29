// Large, Special Boys
//    Kane Penley
//    Nathan Orick

#include "mmu.h"

ADDRESS getBaseAddress(ADDRESS);
int isPresent(ADDRESS cur);
int get7thBit(unsigned long te);
ADDRESS addTable(ADDRESS* entry, CPU *cpu);
void addToTLB(CPU *cpu, ADDRESS virt, ADDRESS phys);
int numFreeTables(CPU *cpu);
void free_table(ADDRESS *t, CPU *cpu);
void deallocateTables(ADDRESS *pdp, ADDRESS *pd, ADDRESS *pt, ADDRESS *pml4e, ADDRESS *pdpe, ADDRESS *pde, CPU *cpu);
void removeFromTLB(CPU *cpu, ADDRESS virt, PAGE_SIZE ps);

#define NULL_ADDRESS	(0ul)
#define PHYS_MASK	(0xffful)
#define ENTRY_MASK	(0x1fful)
#define GB_MASK		(0x3ffffffful)
#define MB_MASK		(0x1ffffful) //2fffful -Marz
#define TOP (*((ADDRESS*)&cpu->memory[cpu->mem_size / 2])) // Pointer to the front of the free memory list. If null, we're out of memory.

void StartNewPageTable(CPU *cpu)
{
	//Create cr3 all the way!

	//Take memory 1/2 up:
    //Store the pointer to the top of the stack at the first block of memory.
    TOP = (ADDRESS)&cpu->memory[cpu->mem_size / 2];
    TOP += 8; // Move the top of stack past the top pointer.

    // Align top to a 4k boundary.
    TOP = (ADDRESS)TOP + 0x1000 - ((ADDRESS)TOP & 0xfff);

    // Divide up memory into 4k tables. The first entry in each table points to the next free space in memory.
    ADDRESS *t = (ADDRESS *)TOP;
    while(1) {
        if((ADDRESS)(t + 1024) < (ADDRESS)(&cpu->memory[cpu->mem_size]))
            *t = (ADDRESS)(t + 512);
        else {
            *t = NULL_ADDRESS;
            break;
        }
        t += 512;
    }

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

    int i;
    for (i = 0; i < TLB_SIZE; i++) {
      if (cpu->tlb[i].virt == virt && cpu->tlb[i].tag != 0) {
        cpu->tlb[i].tag++;
        return cpu->tlb[i].phys;
      }
    }

    // We get the pml4 address from the cr3, and it's offset from the virtual address.
    ADDRESS *pml4 = (ADDRESS*)(getBaseAddress(cpu->cr3));
    ADDRESS pml4e = *(pml4 + ((virt >> 39) & ENTRY_MASK)); // Pointer arithmetic.
    if(!isPresent(pml4e))
        return RET_PAGE_FAULT;


    // Get pdp from pml4e. If it's entry's 7th bit is 1, go straight to physical page (These are 1Gb pages).
    // If it's 7th bit is 0, it's either 4Kb or 2Mb. Keep going.
    ADDRESS *pdp = (ADDRESS*)(getBaseAddress(pml4e));
    ADDRESS pdpe = *(pdp + ((virt >> 30) & ENTRY_MASK));
    if(!isPresent(pdpe))
        return RET_PAGE_FAULT;
    if(get7thBit(pdpe) == 1) {
        ADDRESS phys = (ADDRESS)getBaseAddress(pdpe) + (virt & GB_MASK);
        addToTLB(cpu, virt, phys);
        return phys;
    }
    // Get pd from pdpe. If it's entry's 7th bit is 1, go straight to physical page (These are 2Mb pages).
    // If it's 7th bit is 0, it's either 4Kb. Keep going.
    ADDRESS *pd = (ADDRESS*)getBaseAddress(pdpe);
    ADDRESS pde = *(pd + ((virt >> 21) & ENTRY_MASK));
    if(!isPresent(pde))
        return RET_PAGE_FAULT;
    if(get7thBit(pde) == 1) {
        ADDRESS phys = (ADDRESS)getBaseAddress(pde) + (virt & MB_MASK);
        addToTLB(cpu, virt, phys);
        return phys;
    }

    // Get pt from pde. These are 4Kb tables. Go straight to physical page from here.
    // We don't have to check the 7th bit.
    ADDRESS *pt = (ADDRESS*)getBaseAddress(pde);
    ADDRESS pte = *(pt + ((virt >> 12) & ENTRY_MASK));
    if(!isPresent(pte))
        return RET_PAGE_FAULT;
    // Here, we're not multiplying virt&PHYS_MASE by 8 because mem_set/get is using the value as an index into the memory array.
    ADDRESS phys = (ADDRESS)(getBaseAddress(pte)) + (virt & PHYS_MASK);
    addToTLB(cpu, virt, phys);
    return phys;
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

    
    ADDRESS *pml4, *pdp, *pd, *pt;
    ADDRESS *pml4e, *pdpe, *pde, *pte;

	if (cpu->cr3 == 0) {
		//Nothing has been created, so start here
		StartNewPageTable(cpu);
	}

    pml4 = (ADDRESS*)getBaseAddress(cpu->cr3);


    pml4e = pml4 + ((virt >> 39) & ENTRY_MASK);
    if(!isPresent(*pml4e)) {
        if(numFreeTables(cpu) < 3)
            return;
        pdp = (ADDRESS *)addTable(pml4e, cpu);
        if(pdp == 0) return;
    }
    else {
        pdp = (ADDRESS*)(getBaseAddress(*pml4e));
    }


    pdpe = pdp + ((virt >> 30) & ENTRY_MASK);
    if(ps == PS_1G) {
        *pdpe = NULL_ADDRESS;

        // Set the Physical Page Base Address to phys.
        *pdpe |= (phys & (0xffffful << 30)); // Only interested in bits 30-51 as per docs.

        // Set the present bit to 1.
        *pdpe |= 0x1ul;

        // Set bit 7 to 1.
        *pdpe |= (0x1ul << 7);
        
        return;
    }
    else if(!isPresent(*pdpe)) {
        if(numFreeTables(cpu) < 2)
            return;
        pd = (ADDRESS *)addTable(pdpe, cpu);
        if(pd == 0) return;
    }
    else {
        pd = (ADDRESS*)(getBaseAddress(*pdpe));
    }


    pde = pd + ((virt >> 21) & ENTRY_MASK);
    if(ps == PS_2M) {
        *pde = NULL_ADDRESS;

        // Set the Physical Page Base Address to phys.
        *pde |= (phys & (0x7ffffffful << 21)); // Only interested in bits 21-51 as per docs.

        // Set the present bit to 1.
        *pde |= 0x1ul;

        // Set bit 7 to 1.
        *pde |= (0x1ul << 7);
        
        return;
    }
    else if(!isPresent(*pde)) {
        if(numFreeTables(cpu) < 1)
            return;
        pt = (ADDRESS*)addTable(pde, cpu);
        if(pt == 0) return;
    }
    else {
        pt = (ADDRESS*)(getBaseAddress(*pde));
    }


    pte = pt + ((virt >> 12) & ENTRY_MASK);
    // We know this has to 2K.
    *pte = NULL_ADDRESS;

    // Set the Physical Page Base Address to phys.
    *pte |= (phys & (0xfffffffffful << 12)); // Only interested in bits 12-51 as per docs.

    // Set the present bit to 1.
    *pte |= 0x1ul;

    // Bit 7 doesn't matter here.
    
    return;
    
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
    ADDRESS *pml4, *pdp, *pd, *pt;
    ADDRESS *pml4e, *pdpe, *pde, *pte;

    pml4 = pdp = pd = pt = pml4e = pdpe = pde = pte = NULL;

    //Simply set the present bit (P) to 0 of the virtual address page
    //If the page size is 1G, set the present bit of the PDP to 0
    //If the page size is 2M, set the present bit of the PD  to 0
    //If the page size is 4K, set the present bit of the PT  to 0

    if (cpu->cr3 == 0)
        return;

    removeFromTLB(cpu, virt, ps);
    
    pml4 = (ADDRESS*)getBaseAddress(cpu->cr3);

    pml4e = pml4 + ((virt >> 39) & ENTRY_MASK);
    if (!isPresent(*pml4e))
        return;

    // If ps is 1G, the pdpe must have it's present bit set to 0.
    pdp = (ADDRESS*)getBaseAddress(*pml4e);
    pdpe = pdp + ((virt >> 30) & ENTRY_MASK);

    if (!isPresent(*pdpe))
        return;
    else if (ps == PS_1G) {
        *pdpe &= ~0x1ul;
        deallocateTables(pdp, pd, pt, pml4e, pdpe, pde, cpu);
        return;
    }

    // If ps is 2M, the pde must have it's present bit set to 0.
    pd = (ADDRESS*)getBaseAddress(*pdpe);
    pde = pd + ((virt >> 21) & ENTRY_MASK);

    if (!isPresent(*pde))
        return;
    else if (ps == PS_2M) {
        *pde &= ~0x1ul;
        deallocateTables(pdp, pd, pt, pml4e, pdpe, pde, cpu);
        return;
    }

    // If ps is 4K, the pte must have it's present bit set to 0.
    pt = (ADDRESS*)getBaseAddress(*pde);
    pte = pt + ((virt >> 12) & ENTRY_MASK);

    *pte &= ~0x1ul;
    deallocateTables(pdp, pd, pt, pml4e, pdpe, pde, cpu);
}

void invlpg(CPU *cpu, ADDRESS virt) {
    int i;
    for (i = 0; i < TLB_SIZE; i++) {
        if (cpu->tlb[i].virt == virt) {
            cpu->tlb[i].virt = 0;
            cpu->tlb[i].phys = 0;
            cpu->tlb[i].tag = 0;
        }
    }
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
ADDRESS addTable(ADDRESS* entry, CPU *cpu) {
    // If TOP is null, there is no more memory.
    if(TOP == NULL_ADDRESS) {
        return NULL_ADDRESS;
    }

    ADDRESS p = TOP;

    // Move TOP to the top of the next free table in memory.
    TOP = *((ADDRESS *)TOP);

    *entry |= p; // Sets 51-12 to p. 11-0 are all 0 because p is 4k aligned.
    *entry |= 0x1ul; // Sets present bit 1.
    *entry &= ~(0x1ul << 7); // Sets bit 7 to 0.


    // Mark every entry in the new table as being not present and 7th bit as zero.
    int i;
    ADDRESS *q = (ADDRESS*)p;
    for(i = 0; i < 512; q++, i++) {
        *q = NULL_ADDRESS;
    }

    return p;
}

void addToTLB(CPU *cpu, ADDRESS virt, ADDRESS phys) {
    int index = 0;
    int i;

    for (i = 0; i < TLB_SIZE; i++) {
        if (cpu->tlb[i].tag < cpu->tlb[index].tag) index = i;
    }

    cpu->tlb[index].virt = virt;
    cpu->tlb[index].phys = phys;
    cpu->tlb[index].tag = 1;
}


// Returns the number of free tables available in memory.
int numFreeTables(CPU *cpu) {
    int i = 0;
    ADDRESS *j;

    for(j = (ADDRESS *)TOP; j != NULL; j = (ADDRESS *)*j, i++);

    return i;
}

// Starts at the lowest level. If an address is null, that level is skipped.
// If all of the entries in one of the tables are not present, then the entry
// pointing to it from table above is marked as not present and the empty
// table is deallocated.
void deallocateTables(ADDRESS *pdp, ADDRESS *pd, ADDRESS *pt, ADDRESS *pml4e, ADDRESS *pdpe, ADDRESS *pde, CPU *cpu) {
    int i;
    int clear = 1;

    if(pt != NULL) {
        // Check each entry in this table for presentness.
        // If none are present, deallocate the table.
        for(i = 0; i < 512; i++) {
            if(isPresent(*(pt + i))) // If any entry is present, don't free the table.
                clear = 0;
        }

        // Deallocate table.
        if(clear) {
            *pde &= ~0x1ul;
            free_table(pt, cpu);
        }
    }



    if(pd != NULL) {
        clear = 1;
        // Check each entry in this table for presentness.
        // If none are present, deallocate the table.
        for(i = 0; i < 512; i++) {
            if(isPresent(*(pd + i))) // If any entry is present, don't free the table.
                clear = 0;
        }

        // Deallocate table.
        if(clear) {
            *pdpe &= ~0x1ul;
            free_table(pd, cpu);
        }
    }


    if(pd != NULL) {
        clear = 1;
        // Check each entry in this table for presentness.
        // If none are present, deallocate the table.
        for(i = 0; i < 512; i++) {
            if(isPresent(*(pdp + i))) // If any entry is present, don't free the table.
                clear = 0;
        }

        // Deallocate table.
        if(clear) {
            *pml4e &= ~0x1ul;
            free_table(pdp, cpu);
        }
    }
}

// Adds a table back to the free list.
void free_table(ADDRESS *t, CPU *cpu) {
    *t = TOP;
    TOP = (ADDRESS)t;
}

void removeFromTLB(CPU *cpu, ADDRESS virt, PAGE_SIZE ps) {
    int i;
    ADDRESS tlbVirt;

    for (i = 0; i < TLB_SIZE; i++) {
        tlbVirt = cpu->tlb[i].virt;
        if (ps == PS_4K && ((tlbVirt & ~PHYS_MASK) == (virt & ~PHYS_MASK))) {
            cpu->tlb[i].virt = 0;
            cpu->tlb[i].phys = 0;
            cpu->tlb[i].tag = 0;
        }
        if (ps == PS_1G && ((tlbVirt & ~GB_MASK) == (virt & ~GB_MASK))) {
            cpu->tlb[i].virt = 0;
            cpu->tlb[i].phys = 0;
            cpu->tlb[i].tag = 0;
        }
        if (ps == PS_2M && ((tlbVirt & ~MB_MASK) == (virt & ~MB_MASK))) {
            cpu->tlb[i].virt = 0;
            cpu->tlb[i].phys = 0;
            cpu->tlb[i].tag = 0;
        }
    }
}
