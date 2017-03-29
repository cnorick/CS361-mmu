#include <stdlib.h>
#include "mmu.h"

void test4k(){
    CPU *cpu;
    cpu = new_cpu(1 << 20); // 
    
    map(cpu, 0x0, 0x1000, PS_4K);
    map(cpu, 0x1000, 0x800000, PS_4K);
    map(cpu, 0x2000, 0x280402000, PS_4K);

    mem_set(cpu, 0xffb, 123);
    mem_set(cpu, 0x1ffb, 456);
    mem_set(cpu, 0x2ffb, 789);

    printf("PAGING OFF, This should be 123: %d\n", mem_get(cpu, 0xffb)); 
    printf("PAGING OFF, This should be 456: %d\n", mem_get(cpu, 0x1ffb)); 
    printf("PAGING OFF, This should be 789: %d\n", mem_get(cpu, 0x2ffb)); 

    cpu->cr0 |= 1 << 31;

    printf("PAGING ON, This should be 123: %d\n", mem_get(cpu, 0x1ffb)); 
    printf("PAGING ON, This should be 456: %d\n", mem_get(cpu, 0x800ffb)); 
    printf("PAGING ON, This should be 789: %d\n", mem_get(cpu, 0x280402ffb)); 

    printf("PAGING ON, This should be a page fault: %d\n", mem_get(cpu, 0xa00fff)); 

    mem_set(cpu, 0x800001, 5656);
    printf("PAGING ON, This should be 5656: %d\n", mem_get(cpu, 0x800001)); 
    cpu->cr0 &= ~(1 << 31);
    printf("PAGING OFF, This should be 5656: %d\n", mem_get(cpu, 0x1001)); 

    cpu->cr0 |= 1 << 31;
   
    unmap(cpu, 0x1000, PS_4K);
    printf("PAGING ON, This should be a page fault: %d\n", mem_get(cpu, 0x1ffb)); 
    map(cpu, 0x0, 0xa00000, PS_4K);

    printf("PAGING ON, This should be 123: %d\n", mem_get(cpu, 0xa00ffb)); 

    //funny story if you care to read it:
    //You really have to know your shit when writing c test code lol
    //I set the mem locations originally to 
    //0xfff -> 123
    //0x1fff -> 456
    //0x2fff -> 789
    //then I went and set 0x1001 to 5656 and eventually i remapped a different page to 0x0
    //so when I did a mem_get of 0a00fff which pointed to 0xfff and displayed it it came out some random number
    //I was so confused by this, I know I had to be doing something wrong and I figured out what it was
    //It had nothing to do with how I implemented map, that worked fine, but I was so worried I was wrong and didn't know that it was correct at the time
    //It actually has to do with how mem_get works, it returns an int so when I did memget on 0xa00fff it gave me the int corresponding to 0xfff-1002 and since
    //I set 1001 to 5656 I ended up with 123 | (5656 <<  24) 
    //c is just funny like that lol 
    //Everything should be fixed in the test now
}
