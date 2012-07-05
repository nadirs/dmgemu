/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

/*
 * INSTRUCTION OPCODEs:
 *  01rrrsss        LD r, s
 *  00rrr110 n      LD r, n
 *  01rrr110        LD r, [HL]
 *  01110rrr        LD [HL], r
 *  01110110 n      LD [HL], n
 *  00001010        LD A, [BC]
 *  00011010        LD A, [DE]
 *  11110010        LD A, [0xFF00+C]
 *  11100010        LD [0xFF00+C], A
 *  11110000 n      LD A, [0xFF00+n]
 *  11100000 n      LD [0xFF00+n], A
 *  11111010 nn     LD A, [nn]
 *  11101010 nn     LD [nn], A
 *  00101010        LD A, [HL+]
 *  00111010        LD [HL+], A
 *
 */

#ifndef ASM_H
#define ASM_H

#include <inttypes.h>
#include "cpu.h"

#define OPCODE_NUM 0xFF

enum {
    REG_A = 0x7,
    REG_B = 0x0,
    REG_C = 0x1,
    REG_D = 0x2,
    REG_E = 0x3,
    REG_H = 0x4,
    REG_L = 0x5,
};

struct instruction {
    uint8_t opcode;
    uint8_t mask; // example: ld r,s -> mask: 11000000
    void (* run)(void);
};

void (*opfunc[OPCODE_NUM]) (void);
uint8_t *reg_src(uint8_t opcode); /* get source register in a LD instr */
uint8_t *reg_dst(uint8_t opcode); /* get destination register in a LD instr */

/*
 * GB-z80 INSTRUCTIONS
 */
void ld_r8r8(void);
void ld_r8n(void);
void ld_r8hl(void);
void ld_hlr8(void);
void ld_hln(void);
void ld_abc(void);
void ld_ade(void);

static struct instruction instr_set[] = {
    {.opcode=0b01000000, .mask=0b11000000, .run=ld_r8r8},       // LD r,s
    {.opcode=0b00000110, .mask=0b11000111, .run=ld_r8n},        // LD r,n
    {.opcode=0b01000110, .mask=0b11000111, .run=ld_r8hl},       // LD r,[HL]
    {.opcode=0b01110000, .mask=0b11111000, .run=ld_hlr8},       // LD [HL],r
    {.opcode=0b00110110, .mask=0b11111111, .run=ld_hln},        // LD [HL],n
    {.opcode=0b00001010, .mask=0b11111111, .run=ld_abc},        // LD a,[BC]
    {.opcode=0b00011010, .mask=0b11111111, .run=ld_ade},        // LD a,[DE]
};

#endif
