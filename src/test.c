/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include "dmgemu.h"
#include <stdio.h>

void printregs(void);

int main(void)
{
    cpuinit();
    printregs();

    return 0;
}

void printregs(void)
{
    printf("af: %04X\nbc: %04X\nde: %04X\nhl: %04X\n", *af, *bc, *de, *hl);
    printf("\npc: %04X\nsp: %04X\n\n", *pc, *sp);
}
