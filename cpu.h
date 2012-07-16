/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

/* --------------------------------------------------------
 * SPECS:
 * basic clock frequency:
 *      f = 4.194394 MHz
 *
 * == REGISTERS ==
 *
 *  P1      [FF00]
 *      The P1 ports are connected with a matrix for reading key operations.
 *  DIV     [FF04]
 *      The upper 8 bits of the 16-bit counter that counts the basic clock 
 *      frequency (f) can be referenced. If an LD instruction is executed, 
 *      these bits are cleared to 0 regardless of the value being written.
 *
 * Timer Registers:
 *  TIMA    [FF05]
 *      Timer Counter. The main timer unit. Generates an interrupt when it 
 *      overflows.
 *  TMA     [FF06]
 *      Timer Modulo. The value of TMA is loaded when TIMA overflows.
 *  TAC     [FF07]
 *      Timer Controller.
 *
 * Interrupt Flags:
 *          bit | priority | vector | name
 *           0        1      0x0040   V-Blank
 *           1        2      0x0048   LCDC (STAT referenced)
 *           2        3      0x0050   Timer overflow
 *           3        4      0x0058   Serial I/O transfer completion
 *           4        5      0x0060   P10-P13 terminal negative edge
 *  IF [FF0F]
 *      Interrupt Request.
 *  IE [FFFF]
 *      Interrupt Enable.
 *          0: Disabled
 *          1: Enable
 *  IME
 *      Interupt Master Enable.
 *      0: Reset by DI instruction; prohibits all interrupts.
 *      1: Set by EI instruction; the interrupt set by IE registers are 
 *          enabled.
 * --------------------------------------------------------
 */

#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include <inttypes.h>

#define CODE_START 0x0100 // code entry point

/* cpu flag bits positions (register F) */
#define ZEROB   7   // zero flag
#define NEGB    6   // negative flag
#define HALFCB  5   // half carry flag (carry from bit 3/11 (byte/word))
#define CARRYB  4   // carry flag (carry from bit 7/15 (byte/word))
/* cpu flag masks (register F) */
#define ZEROF   1 << ZEROB  // zero flag
#define NEGF    1 << NEGB   // negative flag
#define HALFCF  1 << HALFCB // half carry flag (carry from bit 3/11 (byte/word))
#define CARRYF  1 << CARRYB // carry flag (carry from bit 7/15 (byte/word))

#define FLAG_ON_COND(flag, condition) \
    do { \
        if (condition) *f |= (flag); \
        else *f &= ~(flag); \
    } while(0)

enum { DMG, CGB };

/* 16-bit WORDs are little endian */
typedef struct {
    uint8_t lo;
    uint8_t hi;
} GBWORD;

/*
 * reference the same two bytes as:
 *  - 1 WORD (16 bit);
 *  - 2 BYTE ( 8 bit);
 */
typedef union {
    uint16_t reg16;
    GBWORD reg8;
} REG;

/* You can use a single 8-bit register or the coupled 16-bit registers */
extern uint16_t *af, *bc, *de, *hl, *pc, *sp;
extern uint8_t *a, *f, *b, *c, *d, *e, *h, *l;

uint8_t fetchbyte(void);
uint8_t refetchbyte(void);
uint16_t fetchword(void);
void cpuinit(void);

#endif
