#ifndef __unitTests_h__
#define __unitTests_h__

#include <assert.h>
#include "mmu.h"

ADDRESS getBaseAddress(ADDRESS cur);
int isPresent(ADDRESS cur);
int pagingIsEnabled(CPU *cpu);
int get7thBit(unsigned long te);


void test_getBaseAddress() {
    ADDRESS a = getBaseAddress(0xfff123456789abcdul);
    assert(a == 0x000123456789a000ul);
}

void test_isPresent() {
    unsigned long a = 0xfffffffff01ul;
    unsigned long b = 0xfffffffff02ul;

    assert(isPresent(a));
    assert(!isPresent(b));
}


void test_pagingIsEnabled() {
    CPU cpu;
    cpu.cr0 = 0xfffffffful;
    
    assert(pagingIsEnabled(&cpu));

    cpu.cr0 = 0x7ffffffful;
    assert(!pagingIsEnabled(&cpu));
}

void test_get7thBit() {
    // bits are 0-based.
    unsigned long a = 0xff;
    unsigned long b = 0x7f;

    assert(get7thBit(a) == 1);
    assert(get7thBit(b) == 0);
}


void runTests() {
    test_getBaseAddress();
    test_isPresent();
    test_pagingIsEnabled();
    test_get7thBit();
}


#endif
