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
    uint8_t lo;
    uint8_t hi;
} GBWORD;

typedef union {
    uint16_t reg16;
    GBWORD reg8;
} reg;

reg reg_af, reg_bc, reg_de, reg_hl, reg_pc, reg_sp;
uint16_t *af, *bc, *de, *hl, *pc, *sp;
uint8_t *a, *f, *b, *c, *d, *e, *h, *l;

void defregs(reg *unionreg, uint16_t **reg16, uint8_t **lo8, uint8_t **hi8);
void buildregs(void);

#endif
