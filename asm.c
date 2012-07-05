/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include "asm.h"
#include "cpu.h"
#include <stdio.h>

uint8_t* ptrtoreg(uint8_t reg_id)
{
    uint8_t *ptr = 0;
    
    switch (reg_id) {
    case 0x0:
        ptr = b;
        break;
    case 0x1:
        ptr = c;
        break;
    case 0x2:
        ptr = d;
        break;
    case 0x3:
        ptr = e;
        break;
    case 0x4:
        ptr = h;
        break;
    case 0x5:
        ptr = l;
        break;
    case 0x7:
        ptr = a;
        break;
    }
    
    return ptr;
}

uint8_t *reg_src(uint8_t opcode)
{
    return ptrtoreg(opcode & 0b00000111);
}

uint8_t *reg_dst(uint8_t opcode)
{
    return ptrtoreg((opcode & 0b00111000) >> 3);
}

void runop(uint8_t *op)
{
    switch (*op)
    {
        default:
            break;
    }
}

int main(void)
{
    uint8_t opcode;
    uint8_t *regsrc, *regdst;

    buildregs();

    opcode = 0x7A;
    *a = 0x12;
    *d = 0x34;
    regsrc = reg_src(opcode);
    regdst = reg_dst(opcode);

    printf("%X\tLD $%X, $%X\n", opcode, *regdst, *regsrc);

    return 0;
}
