/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include "cpu.h"
#include <stdio.h>
#include <inttypes.h>

uint16_t *af, *bc, *de, *hl;//, *pc, *sp;
uint8_t *a, *f, *b, *c, *d, *e, *h, *l;

void defregs(REG *reg_16, uint16_t **r16, uint8_t **l8, uint8_t **h8)
{
    *r16 = &(reg_16->reg16);
    *l8 = &(reg_16->reg8.lo);
    *h8 = &(reg_16->reg8.hi);
}

void buildregs(void)
{
    static REG reg_af, reg_bc, reg_de, reg_hl;//, reg_pc, reg_sp;

    defregs(&reg_af, &af, &f, &a);
    defregs(&reg_bc, &bc, &c, &b);
    defregs(&reg_de, &de, &e, &d);
    defregs(&reg_hl, &hl, &l, &h);
}
