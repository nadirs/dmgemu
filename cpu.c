/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include "cpu.h"

uint16_t *af, *bc, *de, *hl, *pc, *sp;
uint8_t *a, *f, *b, *c, *d, *e, *h, *l, *s, *p;

void defregs(REG *reg_16, uint16_t **r16, uint8_t **l8, uint8_t **h8)
{
    *r16 = &(reg_16->reg16);
    *l8 = &(reg_16->reg8.lo);
    *h8 = &(reg_16->reg8.hi);
}

void initregs()
{
    *sp = 0xFFFE;
    *pc = CODE_START;
    
    *a = 0x01;
    *f = ZEROF | HALFCF | CARRYF;
    *bc = 0x0013;
    *de = 0x00D8;
    *hl = 0x014D;
    
    /*
        case CGB:
            *a = 0x11;
            *f = ZEROF;
            *bc = 0x0000;
            *de = 0x0008;
            *hl = 0x007C;
            break;
     */
}

uint8_t fetchbyte(void)
{
    uint8_t result = *(memory_area + *pc);
    (*pc)++;
    return result;
}

uint8_t refetchbyte(void)
{
    return *(memory_area + *pc - 1);
}

uint16_t fetchword(void)
{
    uint16_t word;
    word = getmemword(*pc);
    *pc += 2;

    return word;
}

void buildregs(void)
{
    static uint16_t _pc;//, _sp;
    static REG reg_af, reg_bc, reg_de, reg_hl, reg_sp;

    defregs(&reg_af, &af, &f, &a);
    defregs(&reg_bc, &bc, &c, &b);
    defregs(&reg_de, &de, &e, &d);
    defregs(&reg_hl, &hl, &l, &h);
    defregs(&reg_sp, &sp, &s, &p);
    pc = &_pc;
//    sp = &_sp;
}

void cpuinit(void)
{
    buildregs();
    initregs();
}
