/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <stdint.h>

/* flags (register F) */
// TODO: bit masks for these flags?
#define ZF  0x00    // zero flag
#define CF  0x00    // carry flag
#define HF  0x00    // half carry flag (nybble verflow)
#define NF  0x00    // XXX: what's this flag?

typedef struct {
    uint8_t hi;
    uint8_t lo;
} hilo;

typedef union {
    uint16_t reg16;
    hilo reg8;
} reg;

#endif
