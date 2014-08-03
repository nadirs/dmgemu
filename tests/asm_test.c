/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "../src/cpu.h"
#include "../src/asm.h"
#include "../src/memory.h"

int main(void)
{
    cpuinit();

    printf("af: 0x%04X (%p)\n", *af, af);
    printf(" a:   0x%02X (%p)\n", *a, a);
    printf(" f:   0x%02X (%p)\n", *f, f);

    uint8_t code[] = {
        0x21, 0x34, 0x12,   // ld hl, 0x1234
        0xCB, 0x04,         // rl h
    };
    *pc = 0;
    setmemory(code);

    assert(0x21 == getmembyte(0));
    // normal code
    run_opcode();
    assert(*hl == 0x1234);
    // CB-prefixed code
    run_opcode();
    assert(*h == 0x12 << 1);

    return 0;
}
