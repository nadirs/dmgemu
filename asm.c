/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include "asm.h"
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

uint8_t *srcreg(uint8_t opcode)
{
    return ptrtoreg(opcode & 0b00000111);
}

uint8_t *dstreg(uint8_t opcode)
{
    return ptrtoreg((opcode & 0b00111000) >> 3);
}

void ld_r8r8(void)
{
    uint8_t opcode = refetchbyte();
    *dstreg(opcode) = *srcreg(opcode);
}

void ld_r8n(void)
{
    uint8_t opcode = refetchbyte();
    *dstreg(opcode) = fetchbyte();
}

void ld_r8hl(void)
{
    uint8_t opcode = refetchbyte();
    *dstreg(opcode) = getmembyte(*hl);
}

void ld_hlr8(void)
{
    uint8_t opcode = refetchbyte();

    putmembyte(*hl, *srcreg(opcode));
}

void ld_hln(void)
{
    uint8_t n = fetchbyte();

    putmembyte(*hl, n);
}

void ld_abc(void)
{
    *a = getmembyte(*bc);
}

void ld_ade(void)
{
    *a = getmembyte(*de);
}

void printregs(void)
{
    printf("af: %04X\nbc: %04X\nde: %04X\nhl: %04X\n", *af, *bc, *de, *hl);
    printf("\npc: %04X\nsp: %04X\n\n", *pc, *sp);
}

int main(void)
{
    buildregs();
    printregs();

    return 0;
}
