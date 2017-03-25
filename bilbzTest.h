#include <stdlib.h>
#include "mmu.h"

int bilbz(){
    CPU *cpu;
    //Test Mapping of 4 KB paging
    cpu = new_cpu(1 << 16); //There is enough room for exactly 16 4096 chunks
   
    printf("MAP 0x1000 phys to 0x0 virt\n");
    map(cpu, 0x0ul, 0x1000ul, PS_4K);

    printf("\nMEMSET 0xf00 123\n");
    mem_set(cpu, 0xf00ul, 123);

    
    printf("\nMEMGET 0xf00 123, paging off\n");
    printf("TEST: value at 0xf00 = %d\n", mem_get(cpu, 0xf00ul));
    
    cpu->cr0 |= 1 << 31;
    printf("\nMEMGET 0x1f00 123, paging on\n");
    printf("TEST: value at virt 0x1f00 = %d\n", mem_get(cpu, 0x1f00ul));
    
    return 0;

}
