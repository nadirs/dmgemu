/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include "cpu.h"

uint16_t *af, *bc, *de, *hl, *pc, *sp;
uint8_t *a, *f, *b, *c, *d, *e, *h, *l;

void defregs(REG *reg_16, uint16_t **r16, uint8_t **l8, uint8_t **h8)
{
    *r16 = &(reg_16->reg16);
    *l8 = &(reg_16->reg8.lo);
    *h8 = &(reg_16->reg8.hi);
}

void initregs(int gbtype)
{
    *sp = 0xFFFE;
    *pc = CODE_START;
    switch(gbtype) {
        case DMG:
            *a = 0x01;
            *f = ZEROF | HALFCF | CARRYF;
            *bc = 0x0013;
            *de = 0x00D8;
            *hl = 0x014D;
            break;
        case CGB:
            *a = 0x11;
            *f = ZEROF;
            *bc = 0x0000;
            *de = 0x0008;
            *hl = 0x007C;
            break;
        default:
            break;
    }
}

void buildregs(void)
{
    static uint16_t _pc, _sp;
    static REG reg_af, reg_bc, reg_de, reg_hl;

    defregs(&reg_af, &af, &f, &a);
    defregs(&reg_bc, &bc, &c, &b);
    defregs(&reg_de, &de, &e, &d);
    defregs(&reg_hl, &hl, &l, &h);
    pc = &_pc;
    sp = &_sp;

    initregs(DMG);
}
