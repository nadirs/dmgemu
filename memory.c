/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include "memory.h"
#include <stdlib.h>
#include <stdio.h>

static uint8_t *memory_area;

void allocmemory(void)
{
    while(!memory_area) {
        memory_area = malloc(MEM_SIZE * sizeof(uint8_t));
    }
}

uint8_t getmembyte(uint16_t addr)
{
    return *(memory_area + addr);
}

uint16_t getmemword(uint16_t addr)
{
    return (getmembyte(addr+1) << 8 | getmembyte(addr));
}

void putmembyte(uint16_t addr, uint8_t byte)
{
    // FIXME: must control if addr is readonly (TODO: implement MBC controllers)
    *(memory_area + addr) = byte;
}

void putmemword(uint16_t addr, uint16_t word)
{
    putmembyte(addr, (word >> 8));
    putmembyte(addr+1, (word & 0xFF));
}
