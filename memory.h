/*
 * Copyright (c) 2012, Nadir Sampaoli
 * See the LICENSE for more information
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "cpu.h"

/* RAM sizes */
#define MEM_SIZE 0xFFFF
#define MEM_BANK_SIZE 0x4000
#define MEM_VRAMDATA_SIZE 0x1800
#define MEM_VRAMCODE_SIZE 0x0800
#define MEM_SRAM_SIZE 0x2000
#define MEM_WRAM_SIZE 0x2000
#define MEM_ERAM_SIZE 0x2000
#define MEM_OAM_SIZE 0x00A0
#define MEM_HRAM_SIZE 0x0100

/* RAM Addresses */
#define MEM_FBANK 0x0000        // first fixed ROM bank
#define MEM_START 0x0150        // code entry point
#define MEM_SBANK 0x4000        // second switchable ROM bank
#define MEM_VRAMDATA 0x8000     // Video RAM Character Data
#define MEM_VRAMCODE 0x9800     // Video RAM Character Codes
#define MEM_SRAM 0xA000         // Save RAM
#define MEM_WRAM 0xC000         // Work RAM
#define MEM_ERAM 0xE000         // Echo RAM
#define MEM_OAM 0xFE00          // Object Attribute Memory (40x32 bits)
#define MEM_FFA0 0xFFA0         // UNUSED
#define MEM_HRAM 0xFF00         // High RAM
#define MEM_REGS 0xFF00         // Port/Mode, Control, Sound Registers
#define MEM_HWRAM 0xFF80        // Working & Stack RAM (127 bytes)


extern uint8_t *memory_area[]; //XXX is it good to have this EXTERN?

uint8_t fetchbyte(void);
uint8_t getmembyte(uint16_t addr);
void putmembyte(uint16_t addr, uint8_t byte);

#endif
