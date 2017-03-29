#include <stdlib.h>
#include "mmu.h"

int numFreeTables(CPU *cpu);

void bigTest2m(){
    CPU *cpu;
    cpu = new_cpu(1 << 23); // Just big enough to hold 4 tables.

//    printf("&cpu->memory[cpu->mem_size / 2]: %#lx\n", &cpu->memory[cpu->mem_size / 2]);

    ADDRESS fuck = 0x123354567;

    map(cpu, 0x0, fuck, PS_2M);
    printf("free tables %d\n", numFreeTables(cpu));
 //   printf("&cpu->memory[cpu->mem_size]: %#lx\n", &cpu->memory[cpu->mem_size]);

    mem_set(cpu, 0xffb, 123);

    printf("PAGING OFF, This should be 123: %d\n", mem_get(cpu, 0xffb)); 

    cpu->cr0 |= 1 << 31;

    printf("PAGING ON, This should be 123: %d\n", mem_get(cpu, 0x1ffb)); 


    unmap(cpu, fuck, PS_2M);
    printf("PAGING ON, This should be a page fault: %d\n", mem_get(cpu, 0x1ffb)); 



    map(cpu, 0x0, 0x698941000, PS_2M);
    printf("free tables %d\n", numFreeTables(cpu));

    printf("PAGING ON, This should be 123: %d\n", mem_get(cpu, 0x698941ffb)); 

}
