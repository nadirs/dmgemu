/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include "memory.h"

uint8_t *memory_area[MEM_SIZE];

uint8_t fetchbyte(void)
{
    *pc++;
    return *memory_area[*pc];
}

uint8_t getmembyte(uint16_t addr)
{
    return *memory_area[addr];
}

void putmembyte(uint16_t addr, uint8_t byte)
{
    // FIXME: must control if addr is readonly (TODO: implement MBC controllers)

    *memory_area[addr] = byte;
}
