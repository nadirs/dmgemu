/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include "myrandom.h"

uint8_t myrand_uint8()
{
    unsigned shift = myrand() & 0x111;
    return (myrand() & (0xFF << shift) >> shift);
}

unsigned myrand()
{
    static unsigned lastseed;
    struct timeval tp;

    if (lastseed) {
        srand(lastseed);
    } else {
        gettimeofday(&tp, 0);
        srand(time(NULL) + tp.tv_usec);
    }

    lastseed = rand();

    return lastseed;
}
