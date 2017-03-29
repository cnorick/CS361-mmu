#include <stdlib.h>
#include "mmu.h"

int numFreeTables(CPU *cpu);

void bigTest4k(){
    CPU *cpu;
    cpu = new_cpu(0x9000); // Just big enough to hold 4 tables.

//    printf("&cpu->memory[cpu->mem_size / 2]: %#lx\n", &cpu->memory[cpu->mem_size / 2]);

    map(cpu, 0x0, 0x1000, PS_4K);
    printf("free tables %d\n", numFreeTables(cpu));
 //   printf("&cpu->memory[cpu->mem_size]: %#lx\n", &cpu->memory[cpu->mem_size]);

    mem_set(cpu, 0xffb, 123);

    printf("PAGING OFF, This should be 123: %d\n", mem_get(cpu, 0xffb)); 

    cpu->cr0 |= 1 << 31;

    printf("PAGING ON, This should be 123: %d\n", mem_get(cpu, 0x1ffb)); 


    unmap(cpu, 0x1000, PS_4K);
    printf("PAGING ON, This should be a page fault: %d\n", mem_get(cpu, 0x1ffb)); 



    map(cpu, 0x0, 0x698941000, PS_4K);
    printf("free tables %d\n", numFreeTables(cpu));

    printf("PAGING ON, This should be 123: %d\n", mem_get(cpu, 0x698941ffb)); 

}
