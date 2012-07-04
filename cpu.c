/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include "cpu.h"
#include <stdio.h>
#include <time.h>

void defregs(reg *reg_16, uint16_t *r16, uint8_t *l8, uint8_t *h8)
{
    r16 = &(reg_16.reg16);
    l8 = &(reg_16.reg8.lo);
    h8 = &(reg_16.reg8.hi);
}

int main(void)
{
    time_t start, stop;
    clock_t ticks;
    int i;
    reg reg_af, reg_bc, reg_de, reg_hl;
    uint16_t *af, *bc, *de, *hl;
    uint8_t *a, *f, *b, *c, *d, *e, *h, *l;

    defregs(&reg_af, &af, &a, &f);

    f = 0xff;

    time(&start);

    for (i=0; i<10; ++i) {
        ticks = clock();
    }
    printf("af: 0x%u\n", af);

    time(&stop);

    printf("Used %.20f seconds of CPU time. \n", (double)ticks/CLOCKS_PER_SEC);
    printf("\nTIME ELAPSED:\t%.20f", difftime(stop, start));

    return 0;
}
