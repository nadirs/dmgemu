/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#include "asm.h"
#include "cpu.h"
#include "memory.h"
#include <stdlib.h>
#include <assert.h>

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
// IF (bit7) THEN (negative ~u8) ELSE (positive u8)
#define U8TOS8(u8) ((0 == (0x80 & (u8))) ? ((u8) & 0x7F) : ((u8) - 0x100))
// get the source register ID
#define SRCREG_ID(r) ((r) & 0b111)
// get destination r8 ID
#define DSTREG8_ID(r) (((r) & 0b111000) >> 3)
// get referred r16 ID
#define REG16_ID(r) (((r) & 0b110000) >> 4)
// generic unmask
#define UNMASK(opcode) ((opcode) & 0b00111000)
// unmask condition flag
#define CONDITION(opcode) UNMASK(opcode)
// get bit index
#define BIT_INDEX(opcode) (((opcode) & 0b00111000) >> 3)
// unmask condition
#define J_COND(opcode) ((opcode) & 0b00011000)
// get RST destination address
#define RST_ADDR(opcode) UNMASK(opcode)
//
#define COND_FLAG_MATCH(zeroflag, carryflag, jumpcond) \
    (((0 == zeroflag) && (J_NZ == jumpcond)) \
     || ((0 != zeroflag) && (J_Z == jumpcond)) \
     || ((0 == carryflag) && (J_NC == jumpcond)) \
     || ((0 != carryflag) && (J_C == jumpcond)))

#define SET_ADD_FLAGS(res, oldval) \
    do { \
        FLAG_ON_COND(ZEROF, ((res) == 0)); \
        FLAG_ON_COND(HALFCF, (((res) & 0b1000) < ((oldval) & 0b1000))); \
        FLAG_ON_COND(CARRYF, \
                (((res) < (oldval)) | (((oldval) != 0) & ((res) == 0)))); \
        *f &= ~NEGF; \
    } while(0)

#define SET_ADC_FLAGS(res, oldval) SET_ADD_FLAGS(res, oldval)

#define SET_SUB_FLAGS(res, oldval) \
    do { \
        FLAG_ON_COND(ZEROF, ((res) == 0)); \
        FLAG_ON_COND(HALFCF, (((res) & 0b1000) > ((oldval) & 0b1000))); \
        FLAG_ON_COND(CARRYF, ((res) > (oldval))); \
        *f |= NEGF; \
    } while(0)

#define SET_SBC_FLAGS(res, oldval) SET_SUB_FLAGS(res, oldval)

#define SET_AND_FLAGS(res) \
    do { \
        *f &= ~(CARRYF | NEGF); \
        *f |= HALFCF; \
        FLAG_ON_COND(ZEROF, (0 == (res))); \
    } while(0)

#define SET_OR_FLAGS(res) \
    do { \
        *f &= ~(CARRYF | HALFCF | NEGF); \
        FLAG_ON_COND(ZEROF, (0 == (res))); \
    } while(0)

// XXX: is it good to reuse OR's macro?
#define SET_XOR_FLAGS(res) SET_OR_FLAGS(res)

#define SET_CP_FLAGS(res, rvalue) \
    do { \
        FLAG_ON_COND(CARRYF, (res) < (rvalue)); \
        FLAG_ON_COND(HALFCF, ((res) & 0xF) < ((rvalue) & 0xF)); \
        FLAG_ON_COND(ZEROF, (res) == (rvalue)); \
        *f |= NEGF; \
    } while(0)

#define SET_INC_FLAGS(res) \
    do { \
        FLAG_ON_COND(HALFCF, (0 == ((res) & 0xF))); \
        FLAG_ON_COND(ZEROF, (0 == (res))); \
        *f &= ~NEGF; \
    } while(0)

#define SET_DEC_FLAGS(res) \
    do { \
        FLAG_ON_COND(HALFCF, (0xF == ((res) & 0xF))); \
        FLAG_ON_COND(ZEROF, (0 == (res))); \
        *f |= NEGF; \
    } while(0)

enum {
    REG_A = 0x7,
    REG_B = 0x0,
    REG_C = 0x1,
    REG_D = 0x2,
    REG_E = 0x3,
    REG_H = 0x4,
    REG_L = 0x5,
    MEM_AT_HL = 0x6
};

enum {
    /* values for:
     *   LD r16,nn
     *   ADD HL,r16
     */
    REG_BC = 0x0,
    REG_DE = 0x1,
    REG_HL = 0x2,
    REG_SP = 0x3
};

enum {
    /* values for:
     *   PUSH r16
     *   POP r16
     */
    REG_BC_PUSHPOP = 0x0,
    REG_DE_PUSHPOP = 0x1,
    REG_HL_PUSHPOP = 0x2,
    REG_AF_PUSHPOP = 0x3
};

enum {
    /* values for:
     *   JP cc,nn
     *   JR cc nn
     */
    J_NZ = 0x0 << 3,
    J_Z  = 0x1 << 3,
    J_NC = 0x2 << 3,
    J_C  = 0x3 << 3
};

struct instruction {
    uint8_t opcode;
    uint8_t mask; // example: ld r,s -> mask: 11000000
    uint32_t (* run)(void);
};

uint8_t *srcreg8(uint8_t opcode); // get source register in a LD instr
uint8_t *dstreg8(uint8_t opcode); // get destination register in a LD instr

uint32_t get_CB(void); // lookup next opcode into CB instructions

/* GB-z80 INSTRUCTIONS */
// NOTE:
//  when cycles is like n-m:
//      n -> r8
//      m -> hl
//  when cycles is like n/m:
//      n -> condition true (does branch)
//      m -> condition false (does not branch)

/* 8-Bit Transfer and I/O Instructions */
//                                 C N H Z | cycles | more bytes |
uint32_t ld_r8xx(void);     /*     - - - - |  1-2   |      0     */
uint32_t ld_r8n(void);      /*     - - - - |   2    |      1     */
uint32_t ld_hlr8(void);     /*     - - - - |   2    |      0     */
uint32_t ld_hln(void);      /*     - - - - |   3    |      1     */
uint32_t ld_abc(void);      /*     - - - - |   2    |      0     */
uint32_t ld_ade(void);      /*     - - - - |   2    |      0     */
uint32_t ld_aff00c(void);   /*     - - - - |   2    |      0     */
uint32_t ld_ff00ca(void);   /*     - - - - |   2    |      0     */
uint32_t ld_aff00n(void);   /*     - - - - |   3    |      1     */
uint32_t ld_ff00na(void);   /*     - - - - |   3    |      1     */
uint32_t ld_ann(void);      /*     - - - - |   4    |      2     */
uint32_t ld_nna(void);      /*     - - - - |   4    |      2     */
uint32_t ld_ahli(void);     /*     - - - - |   2    |      0     */
uint32_t ld_ahld(void);     /*     - - - - |   2    |      0     */
uint32_t ld_bca(void);      /*     - - - - |   2    |      0     */
uint32_t ld_dea(void);      /*     - - - - |   2    |      0     */
uint32_t ld_hlia(void);     /*     - - - - |   2    |      0     */
uint32_t ld_hlda(void);     /*     - - - - |   2    |      0     */
/* 16-Bit Transfer Instructions */
/*                                 C N H Z | cycles | more bytes */
uint32_t ld_r16nn(void);    /*     - - - - |   3    |      2     */
uint32_t ld_sphl(void);     /*     - - - - |   2    |      0     */
uint32_t push_r16(void);    /*     - - - - |   4    |      0     */
uint32_t pop_r16(void);     /*     - - - - |   3    |      0     */
uint32_t ld_hlspe(void);    /*     * * 0 0 |   3    |      1     */
uint32_t ld_nnsp(void);     /*     - - - - |   5    |      2     */
/* 8-Bit Arithmetic and Logical Operation Instructions */
/*                                 C N H Z | cycles | more bytes */
uint32_t add_axx(void);     /*     * * 0 * |  1-2   |      0     */
uint32_t add_an(void);      /*     * * 0 * |   2    |      1     */
uint32_t adc_axx(void);     /*     * * 0 * |  1-2   |      0     */
uint32_t adc_an(void);      /*     * * 0 * |   2    |      1     */
uint32_t sub_axx(void);     /*     * * 1 * |  1-2   |      0     */
uint32_t sub_an(void);      /*     * * 1 * |   2    |      1     */
uint32_t sbc_axx(void);     /*     * * 1 * |  1-2   |      0     */
uint32_t sbc_an(void);      /*     * * 1 * |   2    |      1     */
uint32_t and_axx(void);     /*     0 1 0 * |  1-2   |      0     */
uint32_t and_an(void);      /*     0 1 0 * |   2    |      1     */
uint32_t or_axx(void);      /*     0 0 0 * |  1-2   |      0     */
uint32_t or_an(void);       /*     0 0 0 * |   2    |      1     */
uint32_t xor_axx(void);     /*     0 0 0 * |  1-2   |      0     */
uint32_t xor_an(void);      /*     0 0 0 * |   2    |      1     */
uint32_t cp_axx(void);      /*     * * 1 * |  1-2   |      0     */
uint32_t cp_an(void);       /*     * * 1 * |   2    |      1     */
uint32_t inc_xx(void);      /*     * 0 * * |  1-3   |      0     */
uint32_t dec_xx(void);      /*     * 1 * * |  1-3   |      0     */
/* 16-Bit Arithmetic Operation Instructions */
/*                                 C N H Z | cycles | more bytes */
uint32_t add_hlr16(void);   /*     * * 0 * |   2    |      0     */
uint32_t add_spe(void);     /*     * * 0 0 |   4    |      1     */
uint32_t inc_r16(void);     /*     - - - - |   2    |      0     */
uint32_t dec_r16(void);     /*     - - - - |   2    |      0     */
uint32_t rlca(void);        /*     * 0 0 0 |   1    |      0     */
uint32_t rla(void);         /*     * 0 0 0 |   1    |      0     */
uint32_t rrca(void);        /*     * 0 0 0 |   1    |      0     */
uint32_t rra(void);         /*     * 0 0 0 |   1    |      0     */
uint32_t rlc_xx(void);      /*     * 0 0 * |  2-4   |      0     */ // CB
uint32_t rl_xx(void);       /*     * 0 0 * |  2-4   |      0     */ // CB
uint32_t rrc_xx(void);      /*     * 0 0 * |  2-4   |      0     */ // CB
uint32_t rr_xx(void);       /*     * 0 0 * |  2-4   |      0     */ // CB
uint32_t sla_xx(void);      /*     * 0 0 * |  2-4   |      0     */ // CB
uint32_t sra_xx(void);      /*     * 0 0 * |  2-4   |      0     */ // CB
uint32_t srl_xx(void);      /*     * 0 0 * |  2-4   |      0     */ // CB
uint32_t swap_xx(void);     /*     0 0 0 * |  2-4   |      0     */ // CB
uint32_t bit_nxx(void);     /*     - 1 0 * |  2-3   |      0     */ // CB
uint32_t set_nxx(void);     /*     - - - - |  2-4   |      0     */ // CB
uint32_t res_nxx(void);     /*     - - - - |  2-4   |      0     */ // CB
/* Jump Instructions */
/*                                 C N H Z | cycles | more bytes */
uint32_t jp_nn(void);       /*     - - - - |   4    |      2     */
uint32_t jpcc_nn(void);     /*     - - - - |  4/3   |      2     */
uint32_t jr_e(void);        /*     - - - - |   3    |      1     */
uint32_t jrcc_e(void);      /*     - - - - |  3/2   |      1     */
uint32_t jp_hl(void);       /*     - - - - |   1    |      0     */
/* Call and Return Instructions */
/*                                 C N H Z | cycles | more bytes */
uint32_t call_nn(void);     /*     - - - - |   6    |      2     */
uint32_t callcc_nn(void);   /*     - - - - |  6/3   |      2     */
uint32_t ret(void);         /*     - - - - |   4    |      0     */
uint32_t reti(void);        /*     - - - - |   4    |      0     */
uint32_t ret_cc(void);      /*     - - - - |  5-2   |      0     */
uint32_t rst_t(void);       /*     - - - - |   4    |      0     */
/* General-Purpose Arithmetic Operations and CPU Control Instructions */
/*                                 C N H Z | cycles | more bytes */
uint32_t daa(void);         /*     * 0 - * |   1    |      0     */
uint32_t cpl(void);         /*     - 1 1 - |   1    |      0     */
uint32_t nop(void);         /*     - - - - |   1    |      0     */
uint32_t halt(void);        /*     - - - - |   1    |      0     */
uint32_t stop(void);        /*     - - - - |   1    |      0     */

static struct instruction instr_set[] = {
    /* 2-Byte Commands Starting with 0xCB */
    {.opcode=0b11001011, .mask=0b11111111, .run=get_CB},       // 0xCB 0x??
    /* 8-Bit Transfer and I/O Instructions */
    {.opcode=0b01000000, .mask=0b11000000, .run=ld_r8xx},       // LD r,xx
    {.opcode=0b00000110, .mask=0b11000111, .run=ld_r8n},        // LD r,n
    {.opcode=0b01110000, .mask=0b11111000, .run=ld_hlr8},       // LD [HL],r
    {.opcode=0b00110110, .mask=0b11111111, .run=ld_hln},        // LD [HL],n
    {.opcode=0b00001010, .mask=0b11111111, .run=ld_abc},        // LD A,[BC]
    {.opcode=0b00011010, .mask=0b11111111, .run=ld_ade},        // LD A,[DE]
    {.opcode=0b11110010, .mask=0b11111111, .run=ld_aff00c},     // LD A,[FF00+c]
    {.opcode=0b11100010, .mask=0b11111111, .run=ld_ff00ca},     // LD [FF00+c],A
    {.opcode=0b11110000, .mask=0b11111111, .run=ld_aff00n},     // LD A,[FF00+n]
    {.opcode=0b11100000, .mask=0b11111111, .run=ld_ff00na},     // LD [FF00+n],A
    {.opcode=0b11111010, .mask=0b11111111, .run=ld_ann},        // LD A,[nn]
    {.opcode=0b11101010, .mask=0b11111111, .run=ld_nna},        // LD [nn],A
    {.opcode=0b00101010, .mask=0b11111111, .run=ld_ahli},       // LD A,[HLI]
    {.opcode=0b00111010, .mask=0b11111111, .run=ld_ahld},       // LD A,[HLD]
    {.opcode=0b00000010, .mask=0b11111111, .run=ld_bca},        // LD [BC],A
    {.opcode=0b00010010, .mask=0b11111111, .run=ld_dea},        // LD [DE],A
    {.opcode=0b00100010, .mask=0b11111111, .run=ld_hlia},       // LD [HLI],A
    {.opcode=0b00110010, .mask=0b11111111, .run=ld_hlda},       // LD [HLD],A
    /* 16-Bit Transfer Instructions */
    {.opcode=0b00000001, .mask=0b11001111, .run=ld_r16nn},      // LD r16,nn
    {.opcode=0b11111001, .mask=0b11111111, .run=ld_sphl},       // LD SP,HL
    {.opcode=0b11000101, .mask=0b11001111, .run=push_r16},      // PUSH r16
    {.opcode=0b11000001, .mask=0b11001111, .run=pop_r16},       // POP r16
    {.opcode=0b11111000, .mask=0b11111111, .run=ld_hlspe},      // LD HL,SP+e
    {.opcode=0b00001000, .mask=0b11111111, .run=ld_nnsp},       // LD [nn],SP
    /* 8-Bit Arithmetic and Logical Operation Instructions */
    {.opcode=0b10000000, .mask=0b11111000, .run=add_axx},       // ADD A,xx
    {.opcode=0b11000110, .mask=0b11111111, .run=add_an},        // ADD A,n
    {.opcode=0b10001000, .mask=0b11111000, .run=adc_axx},       // ADC A,xx
    {.opcode=0b11001110, .mask=0b11111111, .run=adc_an},        // ADC A,n
    {.opcode=0b10010110, .mask=0b11111000, .run=sub_axx},       // SUB A,xx
    {.opcode=0b11010110, .mask=0b11111111, .run=sub_an},        // SUB A,n
    {.opcode=0b10011000, .mask=0b11111000, .run=sbc_axx},       // SBC A,xx
    {.opcode=0b11011110, .mask=0b11111111, .run=sbc_an},        // SBC A,n
    {.opcode=0b10100000, .mask=0b11111000, .run=and_axx},       // AND A,xx
    {.opcode=0b11100110, .mask=0b11111111, .run=and_an},        // AND A,n
    {.opcode=0b10110000, .mask=0b11111000, .run=or_axx},        // OR A,xx
    {.opcode=0b11110110, .mask=0b11111111, .run=or_an},         // OR A,n
    {.opcode=0b10101000, .mask=0b11111000, .run=xor_axx},       // XOR A,xx
    {.opcode=0b11101110, .mask=0b11111111, .run=xor_an},        // XOR A,n
    {.opcode=0b10111000, .mask=0b11111000, .run=cp_axx},        // CP A,xx
    {.opcode=0b11111110, .mask=0b11111111, .run=cp_an},         // CP A,n
    {.opcode=0b00000100, .mask=0b11000111, .run=inc_xx},        // INC xx
    {.opcode=0b00000101, .mask=0b11000111, .run=dec_xx},        // DEC xx
    /* 16-Bit Arithmetic Operation Instructions */
    {.opcode=0b00001001, .mask=0b11001111, .run=add_hlr16},     // ADD [HL],r16
    {.opcode=0b11101000, .mask=0b11111111, .run=add_spe},       // ADD SP,e
    {.opcode=0b00000011, .mask=0b11001111, .run=inc_r16},       // INC r16
    {.opcode=0b00001011, .mask=0b11111111, .run=dec_r16},       // DEC r16
    /* Rotate Shift Instructions (check instr_set_CB[] too) */
    {.opcode=0b00000111, .mask=0b11111111, .run=rlca},          // RLCA
    {.opcode=0b00010111, .mask=0b11111111, .run=rla},           // RLA
    {.opcode=0b00001111, .mask=0b11111111, .run=rrca},          // RRCA
    {.opcode=0b00011111, .mask=0b11111111, .run=rra},           // RRA
    /* Jump Instructions */
    {.opcode=0b11000011, .mask=0b11111111, .run=jp_nn},         // JP nn
    {.opcode=0b11000010, .mask=0b11100111, .run=jpcc_nn},       // JP cc,nn
    {.opcode=0b00011000, .mask=0b11111111, .run=jr_e},          // JR e
    {.opcode=0b00100000, .mask=0b11100111, .run=jrcc_e},        // JR cc,e
    {.opcode=0b11101001, .mask=0b11111111, .run=jp_hl},         // JP [HL]
    /* Call and Return Instructions */
    {.opcode=0b11001101, .mask=0b11111111, .run=call_nn},       // CALL nn
    {.opcode=0b11000100, .mask=0b11100111, .run=callcc_nn},     // CALL cc,nn
    {.opcode=0b11001001, .mask=0b11111111, .run=ret},           // RET
    {.opcode=0b11011001, .mask=0b11111111, .run=reti},          // RETI
    {.opcode=0b11000000, .mask=0b11100111, .run=ret_cc},        // RET CC
    {.opcode=0b11000111, .mask=0b11000111, .run=rst_t},         // RST t
    /* General-Purpose Arithmetic Operations and CPU Control Instructions */
    {.opcode=0b00100111, .mask=0b11111111, .run=daa},           // DAA
    {.opcode=0b00101111, .mask=0b11111111, .run=cpl},           // CPL
    {.opcode=0b00000000, .mask=0b11111111, .run=nop},           // NOP
    {.opcode=0b01110110, .mask=0b11111111, .run=halt},          // HALT
    {.opcode=0b00010000, .mask=0b11111111, .run=stop}           // STOP
};

static struct instruction instr_set_CB[11] = {
    /* Rotate Shift Instructions (check instr_set[] too) */
    {.opcode=0b00000000, .mask=0b11111000, .run=rlc_xx},        // RLC xx
    {.opcode=0b00010000, .mask=0b11111000, .run=rl_xx},         // RL xx
    {.opcode=0b00001000, .mask=0b11111000, .run=rrc_xx},        // RRC xx
    {.opcode=0b00011000, .mask=0b11111000, .run=rr_xx},         // RR xx
    {.opcode=0b00100000, .mask=0b11111000, .run=sla_xx},        // SLA xx
    {.opcode=0b00101000, .mask=0b11111000, .run=sra_xx},        // SRA xx
    {.opcode=0b00111000, .mask=0b11111000, .run=srl_xx},        // SRL xx
    {.opcode=0b00110000, .mask=0b11111000, .run=swap_xx},       // SWAP xx
    {.opcode=0b01000000, .mask=0b11000000, .run=bit_nxx},       // BIT n,xx
    {.opcode=0b11000000, .mask=0b11000000, .run=set_nxx},       // SET n,xx
    {.opcode=0b10000000, .mask=0b11000000, .run=res_nxx}        // RES n,xx
};

uint16_t *ptrtoreg16(uint8_t reg_id)
{
    assert(reg_id < 4);
    switch (reg_id) {
        case REG_BC:
            return bc;
        case REG_DE:
            return de;
        case REG_HL:
            return hl;
        case REG_SP:
            return sp;
        default:
            return NULL;
    }
}

uint16_t *ptrtoreg16_pushpop(uint8_t reg_id)
{
    assert(reg_id < 4);
    switch (reg_id) {
        case REG_BC_PUSHPOP:
            return bc;
        case REG_DE_PUSHPOP:
            return de;
        case REG_HL_PUSHPOP:
            return hl;
        case REG_AF_PUSHPOP:
            return af;
        default:
            return NULL;
    }
}

uint8_t* ptrtoreg8(uint8_t reg_id)
{
    assert((reg_id < 6) || reg_id == 7);
    switch (reg_id) {
        case REG_B:
            return b;
        case REG_C:
            return c;
        case REG_D:
            return d;
        case REG_E:
            return e;
        case REG_H:
            return h;
        case REG_L:
            return l;
        case REG_A:
            return a;
        default:
            return NULL;
    }
}

void setregcouple_ptrs(uint8_t reg_id, uint8_t **lo, uint8_t **hi)
{
    assert(reg_id < 4);
    switch (reg_id) {
        case REG_BC_PUSHPOP:
            *hi = b;
            *lo = c;
            break;
        case REG_DE_PUSHPOP:
            *hi = d;
            *lo = e;
            break;
        case REG_HL_PUSHPOP:
            *hi = h;
            *lo = l;
            break;
        case REG_AF_PUSHPOP:
            *hi = a;
            *lo = f;
            break;
        default:
            *hi = NULL;
            *lo = NULL;
            break;
    }
}

uint32_t run_opcode_inset(struct instruction *instrset)
{
    uint8_t opcode = fetchbyte();
    struct instruction *instr_slider = &(instrset[0]);

    for (uint16_t i=0; i<COUNT_OF(&instrset);++i) {
        instr_slider = &(instrset[i]);
        if ((opcode & (instr_slider->mask)) == instr_slider->opcode)
            break;
    }
    return instr_slider->run();
}

uint32_t run_opcode()
{
    return run_opcode_inset(instr_set);
}

uint32_t get_CB(void)
{
    return run_opcode(instr_set_CB);
}

uint8_t *srcreg8(uint8_t opcode)
{
    opcode &= 0b00000111;

    return ptrtoreg8(opcode);
}

uint8_t *dstreg8(uint8_t opcode)
{
    return ptrtoreg8((opcode & 0b00111000) >> 3);
}

uint32_t ld_r8xx(void)
{
    uint32_t cycles = 0;
    uint8_t opcode = refetchbyte();
    uint8_t src_id = SRCREG_ID(opcode);
    uint8_t *dst = ptrtoreg8(DSTREG8_ID(opcode));

    switch(src_id){
        case(MEM_AT_HL):
            *dst = getmembyte(*hl);
            cycles = 2;
            break;
        default:
            *dst = *ptrtoreg8(src_id);
            cycles = 1;
            break;
    }

    return cycles;
}

uint32_t ld_r8n(void)
{
    uint8_t opcode = refetchbyte();
    *dstreg8(opcode) = fetchbyte();

    return 2;
}

uint32_t ld_hlr8(void)
{
    uint8_t opcode = refetchbyte();

    putmembyte(*hl, *srcreg8(opcode));

    return 2;
}

uint32_t ld_hln(void)
{
    uint8_t n = fetchbyte();

    putmembyte(*hl, n);

    return 3;
}

uint32_t ld_abc(void)
{
    *a = getmembyte(*bc);

    return 2;
}

uint32_t ld_ade(void)
{
    *a = getmembyte(*de);

    return 2;
}

uint32_t ld_aff00c(void)
{
    *a = getmembyte(MEM_HRAM + *c);

    return 2;
}

uint32_t ld_ff00ca(void)
{
    putmembyte(MEM_HRAM + *c, *a);

    return 2;
}

uint32_t ld_aff00n(void)
{
    *a = getmembyte(MEM_HRAM + refetchbyte());

    return 3;
}

uint32_t ld_ff00na(void)
{
    putmembyte(MEM_HRAM + refetchbyte(), *a);

    return 3;
}

uint32_t ld_ann(void)
{
    *a = getmembyte(fetchword());

    return 4;
}

uint32_t ld_nna(void)
{
    putmembyte(fetchword(), *a);

    return 4;
}

uint32_t ld_ahli(void)
{
    *a = getmembyte(*hl);
    (*hl)++;

    return 2;
}

uint32_t ld_ahld(void)
{
    *a = getmembyte(*hl);
    (*hl)--;

    return 2;
}

uint32_t ld_bca(void)
{
    putmembyte(*bc, *a);

    return 2;
}

uint32_t ld_dea(void)
{
    putmembyte(*de, *a);

    return 2;
}

uint32_t ld_hlia(void)
{
    putmembyte(*hl, *a);
    (*hl)++;

    return 2;
}

uint32_t ld_hlda(void)
{
    putmembyte(*hl, *a);
    (*hl)--;

    return 2;
}

uint32_t ld_r16nn(void)
{
    uint16_t *r16 = ptrtoreg16(REG16_ID(refetchbyte()));

    *r16 = fetchword();

    return 3;
}

uint32_t ld_sphl(void)
{
    *sp = *hl;

    return 2;
}

uint32_t push_r16(void)
{
    uint8_t *lo = NULL;
    uint8_t *hi = NULL;
    setregcouple_ptrs(REG16_ID(refetchbyte()), &lo, &hi);

    (*sp)--;
    putmembyte(*sp, *lo);
    (*sp)--;
    putmembyte(*sp, *hi);

    return 4;
}

uint32_t pop_r16(void)
{
    uint16_t *r16 = ptrtoreg16_pushpop(REG16_ID(refetchbyte()));

    *r16 = getmembyte(*sp) << 8 | getmembyte(*sp+1);
    (*sp) += 2;

    return 3;
}

uint32_t ld_hlspe(void)
{
    uint8_t n = fetchbyte();
    *hl = (*sp) + n;
    //flags
    //  Z reset, N reset
    //  H set if bit11 overflows, else H reset
    //  C set if bit15 overflows, else C reset
    (*f) &= ~(ZEROF | NEGF | HALFCF | CARRYF);
    FLAG_ON_COND(HALFCF, (*h & 0b1000) < (*s & 0b1000));
    FLAG_ON_COND(CARRYF, (*h & 0b10000000) < (*s & 0b10000000));

    return 3;
}

uint32_t ld_nnsp(void)
{
    uint16_t nn = fetchword();
    putmembyte(nn, *p);
    putmembyte(nn+1, *s);

    return 5;
}

uint32_t add_axx(void)
{
    uint32_t cycles = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t oldval = *a;

    switch(src_id){
        case(MEM_AT_HL):
            *a += getmembyte(*hl);
            cycles = 2;
            break;
        default:
            *a += *ptrtoreg8(src_id);
            cycles = 1;
            break;
    }
    SET_ADD_FLAGS(*a, oldval);

    return cycles;
}

uint32_t add_an(void)
{
    uint8_t n = fetchbyte();
    uint8_t oldval = *a;

    *a += n;
    SET_ADD_FLAGS(*a, oldval);

    return 2;
}

uint32_t adc_axx(void)
{
    uint32_t cycles = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t oldval = *a;

    switch(src_id){
        case(MEM_AT_HL):
            *a += getmembyte(*hl);
            cycles = 2;
            break;
        default:
            *a += *ptrtoreg8(src_id);
            cycles = 1;
            break;
    }

    *a += ((*f & CARRYF) >> CARRYB);
    SET_ADC_FLAGS(*a, oldval);

    return cycles;
}

uint32_t adc_an(void)
{
    uint8_t n = fetchbyte();
    uint8_t oldval = *a;

    *a += n + ((*f & CARRYF) >> CARRYB);
    SET_ADC_FLAGS(*a, oldval);

    return 2;
}

uint32_t sub_axx(void)
{
    uint32_t cycles = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t oldval = *a;

    switch(src_id){
        case(MEM_AT_HL):
            *a -= getmembyte(*hl);
            cycles = 2;
            break;
        default:
            *a -= *ptrtoreg8(src_id);
            cycles = 1;
            break;
    }
    SET_SUB_FLAGS(*a, oldval);

    return cycles;
}

uint32_t sub_an(void)
{
    uint8_t oldval = *a;

    *a -= fetchbyte();
    SET_SUB_FLAGS(*a, oldval);

    return 2;
}

uint32_t sbc_axx(void)
{
    uint32_t cycles = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t oldval = *a;

    switch(src_id){
        case(MEM_AT_HL):
            *a -= getmembyte(*hl);
            cycles = 2;
            break;
        default:
            *a -= *ptrtoreg8(src_id);
            cycles = 1;
            break;
    }
    *a -= ((*f & CARRYF) >> CARRYB);
    SET_SBC_FLAGS(*a, oldval);

    return cycles;
}

uint32_t sbc_an(void)
{
    uint8_t oldval = *a;

    *a -= (fetchbyte() + ((*f & CARRYF) >> CARRYB));
    SET_SBC_FLAGS(*a, oldval);

    return 2;
}

uint32_t and_axx(void)
{
    uint32_t cycles = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());

    switch(src_id){
        case(MEM_AT_HL):
            *a &= getmembyte(*hl);
            cycles = 2;
            break;
        default:
            *a &= *ptrtoreg8(src_id);
            cycles = 1;
            break;
    }
    SET_AND_FLAGS(*a);

    return cycles;
}

uint32_t and_an(void)
{
    *a &= fetchbyte();
    SET_AND_FLAGS(*a);

    return 2;
}

uint32_t or_axx(void)
{
    uint32_t cycles = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());

    switch(src_id){
        case(MEM_AT_HL):
            *a |= getmembyte(*hl);
            cycles = 2;
            break;
        default:
            *a |= *ptrtoreg8(src_id);
            cycles = 1;
            break;
    }
    SET_OR_FLAGS(*a);

    return cycles;
}

uint32_t or_an(void)
{
    *a |= fetchbyte();
    SET_OR_FLAGS(*a);

    return 2;
}

uint32_t xor_axx(void)
{
    uint32_t cycles = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());

    switch(src_id){
        case(MEM_AT_HL):
            *a &= ~(getmembyte(*hl));
            cycles = 2;
            break;
        default:
            *a &= ~(*ptrtoreg8(src_id));
            cycles = 1;
            break;
    }
    SET_XOR_FLAGS(*a);

    return cycles;
}

uint32_t xor_an(void)
{
    *a &= ~(fetchbyte());
    SET_XOR_FLAGS(*a);

    return 2;
}

uint32_t cp_axx(void)
{
    uint32_t cycles = 0;
    uint8_t rvalue = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());

    switch(src_id){
        case(MEM_AT_HL):
            rvalue = getmembyte(*hl);
            cycles = 2;
            break;
        default:
            rvalue = *ptrtoreg8(src_id);
            cycles = 1;
            break;
    }
    SET_CP_FLAGS(*a, rvalue);

    return cycles;
}

uint32_t cp_an(void)
{
    uint8_t rvalue = fetchbyte();
    SET_CP_FLAGS(*a, rvalue);

    return 2;
}

uint32_t inc_xx(void)
{
    uint32_t cycles = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t target = 0;
    uint8_t *reg;

    switch(src_id){
        case(MEM_AT_HL):
            target = getmembyte(*hl) + 1;
            putmembyte(*hl, target);
            cycles = 3;
            break;
        default:
            reg = ptrtoreg8(src_id);
            target = ++(*reg);
            cycles = 1;
            break;
    }
    SET_INC_FLAGS(target);

    return cycles;
}

uint32_t dec_xx(void)
{
    uint32_t cycles = 0;
    uint8_t target = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t *reg;

    switch(src_id){
        case(MEM_AT_HL):
            target = getmembyte(*hl) - 1;
            putmembyte(*hl, target);
            cycles = 3;
            break;
        default:
            reg = ptrtoreg8(src_id);
            target = --(*reg);
            cycles = 1;
            break;
    }
    SET_DEC_FLAGS(target);

    return cycles;
}

uint32_t add_hlr16(void)
{
    uint16_t oldval = *hl;
    uint8_t r16 = *ptrtoreg16(REG16_ID(refetchbyte()));
    *hl += r16;

    FLAG_ON_COND(CARRYF, ((*hl) & 0x8000) < (oldval & 0x8000));
    FLAG_ON_COND(HALFCF, (*hl < oldval));
    *f &= ~NEGF;

    return 2;
}

uint32_t add_spe(void)
{
    uint16_t oldval = *sp;

    *sp += fetchbyte();
    //flags
    *f &= ~(NEGF | ZEROF);
    FLAG_ON_COND(CARRYF, ((*sp) & 0x8000) < (oldval & 0x8000));
    FLAG_ON_COND(HALFCF, (*sp < oldval));

    return 2;
}

uint32_t inc_r16(void)
{
    uint16_t *r16 = ptrtoreg16(REG16_ID(refetchbyte()));

    ++(*r16);

    return 2;
}

uint32_t dec_r16(void)
{
    uint16_t *r16 = ptrtoreg16(REG16_ID(refetchbyte()));

    --(*r16);

    return 2;
}

uint32_t rlca(void)
{
    uint8_t carry = (*a & 0x80) >> 7;

    *a <<= 1;
    *a += carry;
    // flags
    *f &= ~(HALFCF | NEGF | ZEROF);
    FLAG_ON_COND(CARRYF, (carry != 0));

    return 1;
}

uint32_t rla(void)
{
    uint8_t old_carry = (*f & CARRYF) >> CARRYB;
    uint8_t new_carry = (*a & 0x80) >> 7;

    *a <<= 1;
    *a += old_carry;
    // flags
    *f &= ~(HALFCF | NEGF | ZEROF);
    FLAG_ON_COND(CARRYF, (new_carry != 0));

    return 1;
}

uint32_t rrca(void)
{
    uint8_t carry = (*a & 1);

    // flags
    *a >>= 1;
    *a += carry;
    *f &= ~(HALFCF | NEGF | ZEROF);
    FLAG_ON_COND(CARRYF, (carry != 0));

    return 1;
}

uint32_t rra(void)
{
    uint8_t old_carry = (*f & CARRYF) >> CARRYB;
    uint8_t new_carry = (*a & 1);

    *a >>= 1;
    *a += old_carry;
    // flags
    *f &= ~(HALFCF | NEGF | ZEROF);
    FLAG_ON_COND(CARRYF, (new_carry != 0));

    return 1;
}

uint32_t rlc_xx(void)
{
    uint32_t cycles = 0;
    uint8_t target = 0;
    uint8_t carry = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t *reg;

    switch(src_id){
        case(MEM_AT_HL):
            target = getmembyte(*hl);
            carry = (target & 0x80) >> 7;
            target <<= 1;
            target += carry;
            putmembyte(*hl, target);
            cycles = 4;
            break;
        default:
            reg = ptrtoreg8(src_id);
            carry = (*reg & 0x80) >> 7;
            *reg <<= 1;
            *reg += carry;
            target = *reg;
            cycles = 2;
            break;
    }
    // flags
    *f &= ~(HALFCF | NEGF);
    FLAG_ON_COND(CARRYF, (carry != 0));
    FLAG_ON_COND(ZEROF, (0 == target));

    return cycles;
}

uint32_t rl_xx(void)
{
    uint32_t cycles = 0;
    uint8_t target = 0;
    uint8_t new_carry = 0;
    uint8_t old_carry = (*f & CARRYF) >> CARRYB;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t *reg;

    switch(src_id){
        case(MEM_AT_HL):
            target = getmembyte(*hl);
            new_carry = (target & 0x80) >> 7;
            target <<= 1;
            target += old_carry;
            putmembyte(*hl, target);
            cycles = 4;
            break;
        default:
            reg = ptrtoreg8(src_id);
            new_carry = (*reg & 0x80) >> 7;
            *reg <<= 1;
            *reg += old_carry;
            target = *reg;
            cycles = 2;
            break;
    }
    // flags
    *f &= ~(HALFCF | NEGF);
    FLAG_ON_COND(CARRYF, (new_carry != 0));
    FLAG_ON_COND(ZEROF, (0 == target));

    return cycles;
}

uint32_t rrc_xx(void)
{
    uint32_t cycles = 0;
    uint8_t target = 0;
    uint8_t carry = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t *reg;

    switch(src_id){
        case(MEM_AT_HL):
            target = getmembyte(*hl);
            carry = target & 1;
            target >>= 1;
            target += carry;
            putmembyte(*hl, target);
            cycles = 4;
            break;
        default:
            reg = ptrtoreg8(src_id);
            carry = *reg & 1;
            *reg >>= 1;
            *reg += carry;
            target = *reg;
            cycles = 2;
            break;
    }
    // flags
    *f &= ~(HALFCF | NEGF);
    FLAG_ON_COND(CARRYF, (carry != 0));
    FLAG_ON_COND(ZEROF, (0 == target));

    return cycles;
}

uint32_t rr_xx(void)
{
    uint32_t cycles = 0;
    uint8_t target = 0;
    uint8_t new_carry = 0;
    uint8_t old_carry = (*f & CARRYF) >> CARRYB;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t *reg;

    switch(src_id){
        case(MEM_AT_HL):
            target = getmembyte(*hl);
            new_carry = target & 1;
            target >>= 1;
            target += old_carry;
            putmembyte(*hl, target);
            cycles = 4;
            break;
        default:
            reg = ptrtoreg8(src_id);
            new_carry = *reg & 1;
            *reg >>= 1;
            *reg += old_carry;
            target = *reg;
            cycles = 2;
            break;
    }
    // flags
    *f &= ~(HALFCF | NEGF);
    FLAG_ON_COND(CARRYF, (new_carry != 0));
    FLAG_ON_COND(ZEROF, (0 == target));

    return cycles;
}

uint32_t sla_xx(void)
{
    uint32_t cycles = 0;
    uint8_t target = 0;
    uint8_t carry = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t *reg;

    switch(src_id){
        case(MEM_AT_HL):
            target = getmembyte(*hl);
            carry = (target & 0x80) >> 7;
            target <<= 1;
            putmembyte(*hl, target);
            cycles = 4;
            break;
        default:
            reg = ptrtoreg8(src_id);
            carry = (*reg & 0x80) >> 7;
            *reg <<= 1;
            target = *reg;
            cycles = 2;
            break;
    }
    // flags
    *f &= ~(HALFCF | NEGF);
    FLAG_ON_COND(CARRYF, (carry != 0));
    FLAG_ON_COND(ZEROF, (0 == target));

    return cycles;
}

uint32_t sra_xx(void)
{
    uint32_t cycles = 0;
    uint8_t target = 0;
    uint8_t carry = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t *reg;

    switch(src_id){
        case(MEM_AT_HL):
            target = getmembyte(*hl);
            carry = target & 1;
            target = (target & 0x80) | (target >> 1); // bit7 unchanged
            putmembyte(*hl, target);
            cycles = 4;
            break;
        default:
            reg = ptrtoreg8(src_id);
            carry = *reg & 1;
            *reg = (*reg & 0x80) | (*reg >> 1); // bit7 unchanged
            target = *reg;
            cycles = 2;
            break;
    }
    // flags
    *f &= ~(HALFCF | NEGF);
    FLAG_ON_COND(CARRYF, (carry != 0));
    FLAG_ON_COND(ZEROF, (0 == target));

    return cycles;
}

uint32_t srl_xx(void)
{
    uint32_t cycles = 0;
    uint8_t target = 0;
    uint8_t carry = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t *reg;

    switch(src_id){
        case(MEM_AT_HL):
            target = getmembyte(*hl);
            carry = target & 1;
            target >>= 1;
            putmembyte(*hl, target);
            cycles = 4;
            break;
        default:
            reg = ptrtoreg8(src_id);
            carry = *reg & 1;
            *reg >>= 1;
            target = *reg;
            cycles = 2;
            break;
    }
    // flags
    *f &= ~(HALFCF | NEGF);
    FLAG_ON_COND(CARRYF, (carry != 0));
    FLAG_ON_COND(ZEROF, (0 == target));

    return cycles;
}

uint32_t swap_xx(void)
{
    uint32_t cycles = 0;
    uint8_t target = 0;
    uint8_t src_id = SRCREG_ID(refetchbyte());
    uint8_t *reg;

    switch(src_id){
        case(MEM_AT_HL):
            target = getmembyte(*hl);
            target = (target & 0xf << 8) | (target >> 8);
            putmembyte(*hl, target);
            cycles = 4;
            break;
        default:
            reg = ptrtoreg8(src_id);
            *reg = (*reg & 0xf << 8) | (*reg >> 8);
            target = *reg;
            cycles = 2;
            break;
    }
    // flags
    *f &= ~(CARRYF | HALFCF | NEGF);
    FLAG_ON_COND(ZEROF, (0 == target));

    return cycles;
}

uint32_t bit_nxx(void)
{
    uint32_t cycles = 0;
    uint8_t target = 0;
    uint8_t opcode = refetchbyte();
    uint8_t src_id = SRCREG_ID(opcode);
    const uint8_t bit = BIT_INDEX(opcode);

    switch(src_id){
        case(MEM_AT_HL):
            target = getmembyte(*hl);
            cycles = 3;
            break;
        default:
            target = *(ptrtoreg8(src_id));
            cycles = 2;
            break;
    }
    //flags
    *f |= HALFCF;
    *f &= ~NEGF;
    FLAG_ON_COND(ZEROF, (0 == (target & (1 << bit))));

    return cycles;
}

uint32_t set_nxx(void)
{
    uint32_t cycles = 0;
    uint8_t target = 0;
    uint8_t opcode = refetchbyte();
    uint8_t src_id = SRCREG_ID(opcode);
    const uint8_t bit = BIT_INDEX(opcode);
    uint8_t *reg;

    switch(src_id){
        case(MEM_AT_HL):
            target = getmembyte(*hl);
            target |= (1 << bit);
            putmembyte(*hl, target);
            cycles = 4;
            break;
        default:
            reg = ptrtoreg8(src_id);
            *reg |= (1 << bit);
            cycles = 2;
            break;
    }
    //no flags affected

    return cycles;
}

uint32_t res_nxx(void)
{
    uint32_t cycles = 0;
    uint8_t target = 0;
    uint8_t opcode = refetchbyte();
    uint8_t src_id = SRCREG_ID(opcode);
    const uint8_t bit = BIT_INDEX(opcode);
    uint8_t *reg;

    switch(src_id){
        case(MEM_AT_HL):
            target = getmembyte(*hl);
            target &= ~(1 << bit);
            putmembyte(*hl, target);
            cycles = 4;
            break;
        default:
            reg = ptrtoreg8(src_id);
            *reg &= ~(1 << bit);
            cycles = 2;
            break;
    }
    //no flags affected

    return cycles;
}

uint32_t jp_nn(void)
{
    *pc = fetchword();

    return 4;
}

uint32_t jpcc_nn(void)
{
    uint32_t cycles = 3;
    const uint8_t opcode = refetchbyte();
    const uint16_t addr = fetchword();
    const uint8_t zeroflag = *f & ZEROF;
    const uint8_t carryflag = *f & CARRYF;
    uint8_t jumpcond = J_COND(opcode);

    if (COND_FLAG_MATCH(zeroflag, carryflag, jumpcond)) {
        *pc = addr;
        ++cycles;
    }

    return cycles;
}

uint32_t jr_e(void)
{
    uint8_t offset = fetchbyte();

    *pc += U8TOS8(offset);

    return 3;
}

uint32_t jrcc_e(void)
{
    uint8_t cycles = 2;
    const uint8_t zeroflag = *f & ZEROF;
    const uint8_t carryflag = *f & CARRYF;
    uint8_t jumpcond = J_COND(refetchbyte());
    uint8_t offset = fetchbyte();

    if (COND_FLAG_MATCH(zeroflag, carryflag, jumpcond)) {
        *pc += U8TOS8(offset);
        ++cycles;
    }

    return cycles;
}

uint32_t jp_hl(void)
{
    *pc += getmemword(*hl);

    return 1;
}

inline uint32_t call_nn(void)
{
    uint16_t addr = fetchword();

    *sp -= 2;
    putmemword(*sp, *pc);
    *pc = addr;

    return 6;
}

uint32_t callcc_nn(void)
{
    uint32_t cycles = 3;
    uint16_t addr = fetchword();
    const uint8_t zeroflag = *f & ZEROF;
    const uint8_t carryflag = *f & CARRYF;
    uint8_t jumpcond = J_COND(refetchbyte());

    if (COND_FLAG_MATCH(zeroflag, carryflag, jumpcond)) {
        *sp -= 2;
        putmemword(*sp, *pc);
        *pc = addr;
        cycles = 6;
    }

    return cycles;
}

uint32_t ret(void)
{
    *pc = getmemword(*sp);
    *sp += 2;

    return 4;
}

uint32_t reti(void)
{
    // TODO: implement IME flag restoration
    return ret();
}

uint32_t ret_cc(void)
{
    const uint8_t zeroflag = *f & ZEROF;
    const uint8_t carryflag = *f & CARRYF;
    uint8_t callcond = J_COND(refetchbyte());

    if (COND_FLAG_MATCH(zeroflag, carryflag, callcond)) {
        ret();
        return 5;
    }

    return 2;
}

uint32_t rst_t(void)
{
    uint16_t addr = (refetchbyte() & 0b111000);

    *sp -= 2;
    putmemword(*sp, *pc);
    *pc = addr;

    return 4;
}

uint32_t daa(void)
{
    const uint8_t carryflag = ((*f & CARRYF) >> CARRYB);
    const uint8_t negflag = ((*f & NEGF) >> NEGB);
    const uint8_t halfcflag = ((*f & HALFCF) >> HALFCB);
    uint8_t a_hi = ((*a & 0xF0) >> 4);
    uint8_t a_lo = (*a & 0x0F);

    if (negflag) {
        if (carryflag) {
            if (halfcflag && (a_hi > 5) && (a_lo > 5)) {
                *a -= 0x66;
            } else if ((a_hi > 6) && !halfcflag && (a_lo < 0xA)) {
                *a -= 0x60;
            }
            *f |= CARRYF;
        } else {
            if (halfcflag && (a_hi < 9) && (a_lo > 5)) {
                *a -= 0x06;
            }
            *f &= ~CARRYF;
        }
    } else {
        if ((halfcflag && (a_lo < 4))
                || (a_lo > 9)) {
            *a += 0x06;
            *f &= ~CARRYF; // XXX: often pointless but checks have a cost too!
        }
        if ((a_hi - halfcflag > 8) ||
                (carryflag && (a_hi - halfcflag < 3))) {
            *a += 0x60;
            *f |= CARRYF;
        }
    }
    *f &= ~HALFCF;
    FLAG_ON_COND(ZEROF, (0 == *a));

    return 1;
}

uint32_t cpl(void)
{
    *a = ~*a;
    *f |= (HALFCF | NEGF);

    return 1;
}

uint32_t nop(void)
{
    // no operation

    return 1;
}

uint32_t halt(void)
{
    // TODO Halt:
    //  - system clock is stopped to this status;
    //  - oscillator circuit and LCD continue to operate.

    return 1;
}

uint32_t stop(void)
{
    fetchbyte(); // just skip next byte
    // TODO Stop:
    //  - System clock is stopped to this status.
    //  - Oscillator circuit and LCD are also stopped.
    // Required pre-conditions:
    //  - All interrupt-enable (IE) flags are reset.
    //  - Input to P10-P13 is LOW for all.

    return 1;
}
