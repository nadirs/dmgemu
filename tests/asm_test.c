/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include "../src/cpu.h"
#include "../src/memory.h"
#include <stdio.h>
#include <check.h>

int main(void)
{
    cpuinit();
    printf("af: 0x%04X (%p)\n", *af, af);
    printf(" a:   0x%02X (%p)\n", *a, a);
    printf(" f:   0x%02X (%p)\n", *f, f);

    return 0;
}
