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
    cpu = new_cpu(1 << 24); //8 MB, first half of memory is 4 2 MB page frames, second half is 2047 4 KB page tables 
   
    map(cpu, 0x0, 0x200000, PS_2M);
    map(cpu, 0x200000, 0x800000, PS_2M);
    map(cpu, 0x400000, 0x100800000, PS_2M);

    mem_set(cpu, 0xfff, 123);
    mem_set(cpu, 0x200fff, 456);
    mem_set(cpu, 0x400fff, 789);

    printf("PAGING OFF, This should be 123: %d\n", mem_get(cpu, 0xfff)); 
    printf("PAGING OFF, This should be 456: %d\n", mem_get(cpu, 0x200fff)); 
    printf("PAGING OFF, This should be 789: %d\n", mem_get(cpu, 0x400fff)); 
    
    cpu->cr0 |= 1 << 31;

    printf("PAGING ON, This should be 123: %d\n", mem_get(cpu, 0x200fff)); 
    printf("PAGING ON, This should be 456: %d\n", mem_get(cpu, 0x800fff)); 
    printf("PAGING ON, This should be 789: %d\n", mem_get(cpu, 0x100800fff)); 

    printf("PAGING ON, This should be a page fault: %d\n", mem_get(cpu, 0xa00fff)); 
    
    mem_set(cpu, 0x800001, 5656);
    printf("PAGING ON, This should be 5656: %d\n", mem_get(cpu, 0x800001)); 
    cpu->cr0 &= ~(1 << 31);
    printf("PAGING OFF, This should be 5656: %d\n", mem_get(cpu, 0x200001)); 

    cpu->cr0 |= 1 << 31;
   
    unmap(cpu, 0x200000, PS_2M);
    printf("PAGING ON, This should be a page fault: %d\n", mem_get(cpu, 0x200fff)); 
    map(cpu, 0x0, 0xa00000, PS_2M);

    printf("PAGING ON, This should be 123: %d\n", mem_get(cpu, 0xa00fff)); 
    
    //tests that no map does not execute if our chunk of memory is full

	return 0;
}
