#ifndef __VM_H__
#define __VM_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef fd_set         FdSet;
typedef struct timeval TimeVal;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef size_t   usize;

#define U16_MAX 0xFFFF

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;

typedef enum {
    FALSE = 0,
    TRUE,
} Bool;

typedef enum {
    R_0 = 0,
    R_1,
    R_2,
    R_3,
    R_4,
    R_5,
    R_6,
    R_7,
    R_PC,
    R_COND,
    R_SIZE,
} Register;

typedef enum {
    OP_BR = 0, // branch
    OP_ADD,    // add
    OP_LD,     // load
    OP_ST,     // store
    OP_JSR,    // jump register
    OP_AND,    // bitwise and
    OP_LDR,    // load register
    OP_STR,    // store register
    // OP_RTI,  // return from interrupt (unused)
    OP_NOT = 9, // bitwise not
    OP_LDI,     // load indirect
    OP_STI,     // store indirect
    OP_JMP,     // jump
    // OP_RES,   // reserved (unused)
    OP_LEA = 14, // load effective address
    OP_TRAP,     // execute trap
} OpCode;

typedef enum {
    TRAP_GETC = 0x20,  // get char from keyboard, not echoed onto the terminal
    TRAP_OUT = 0x21,   // output a character
    TRAP_PUTS = 0x22,  // output a word string
    TRAP_IN = 0x23,    // get char from keyboard, echoed onto the terminal
    TRAP_PUTSP = 0x24, // output a byte string
    TRAP_HALT = 0x25,  // halt the program
} Trap;

typedef enum {
    KEYBOARD_STATUS = 0xFE00,
    KEYBOARD_DATA = 0xFE02,
} MemoryMap;

typedef enum {
    FL_POS = 1 << 0,
    FL_ZERO = 1 << 1,
    FL_NEG = 1 << 2,
} CondFlag;

typedef enum {
    DEAD = 0,
    ALIVE,
} Status;

static u16    MEM[U16_MAX] = {0};
static u16    REG[R_SIZE] = {0};
static Status STATUS = ALIVE;

#define PC_START 0x3000

static u16 get_sign_extend(u16 x, u16 bit_count) {
    if ((x >> (bit_count - 1)) & 1) {
        x |= (u16)(U16_MAX << bit_count);
    }
    return x;
}

static OpCode get_op(u16 instr) {
    return instr >> 12;
}

static u8 get_r0_or_nzp(u16 instr) {
    return (instr >> 9) & 0x7;
}

static u8 get_r1(u16 instr) {
    return (instr >> 6) & 0x7;
}

static u8 get_r2(u16 instr) {
    return instr & 0x7;
}

static Bool get_immediate_mode(u16 instr) {
    return (instr >> 5) & 0x1;
}

static i8 get_immediate(u16 instr) {
    return (i8)get_sign_extend(instr & 0x1F, 5);
}

static i16 get_pc_offset_9(u16 instr) {
    return (i16)get_sign_extend(instr & 0x1FF, 9);
}

static Bool get_relative_mode(u16 instr) {
    return (instr >> 11) & 0x1;
}

static i16 get_pc_offset_11(u16 instr) {
    return (i16)get_sign_extend(instr & 0x7FF, 11);
}

static i16 get_reg_offset_6(u16 instr) {
    return (i16)get_sign_extend(instr & 0x3F, 6);
}

static Trap get_trap(u16 instr) {
    return (Trap)(instr & 0xFF);
}

#endif
