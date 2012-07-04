/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include "cpu.h"
#include <stdio.h>

void defregs(reg *reg_16, uint16_t *r16, uint8_t *l8, uint8_t *h8)
{
    r16 = &(reg_16.reg16);
    l8 = &(reg_16.reg8.lo);
    h8 = &(reg_16.reg8.hi);
}

int main(void)
{
    int i;
    reg reg_af, reg_bc, reg_de, reg_hl;
    uint16_t *af, *bc, *de, *hl;
    uint8_t *a, *f, *b, *c, *d, *e, *h, *l;

    defregs(&reg_af, &af, &a, &f);
    f = 0xff;
    printf("af: 0x%u\n", af);

    return 0;
}
