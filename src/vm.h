#ifndef __VM_H__
#define __VM_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t  u8;
typedef uint16_t u16;

#define U16_MAX 0xFFFF

typedef int8_t  i8;
typedef int16_t i16;

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
    // OP_ST,    // store
    // OP_JSR,   // jump register
    OP_AND = 5, // bitwise and
    // OP_LDR,   // load register
    // OP_STR,   // store register
    // OP_RTI,   // unused
    // OP_NOT,   // bitwise not
    OP_LDI = 10, // load indirect
    // OP_STI,   // store indirect
    // OP_JMP,   // jump
    // OP_RES,   // reserved (unused)
    // OP_LEA,   // load effective address
    // OP_TRAP,  // execute trap
} OpCode;

typedef struct {
    u8  r0;
    u8  r1;
    u16 pc_offset;
    union {
        u8 r2;
        i8 imm_value;
    } opt;
    Bool   imm_mode;
    OpCode op;
} Instr;

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

static u16 sign_extend(u16 x, u16 bit_count) {
    if ((x >> (bit_count - 1)) & 1) {
        x |= (u16)(U16_MAX << bit_count);
    }
    return x;
}

static void set_flags(Register r) {
    if (REG[r] == 0) {
        REG[R_COND] = FL_ZERO;
    } else if (REG[r] >> 15) {
        /* NOTE: A `1` in the left-most bit indicates negative. */
        REG[R_COND] = FL_NEG;
    } else {
        REG[R_COND] = FL_POS;
    }
}

static u16 mem_read(u16 address) {
    return MEM[address];
}

static void set_op(u16* instr, OpCode op) {
    *instr = (u16)(*instr | (op << 12));
}

static OpCode get_op(u16 instr) {
    return instr >> 12;
}

static void set_r0(u16* instr, u8 r0) {
    *instr = (u16)(*instr | (u16)((r0 & 0x7) << 9));
}

static u8 get_r0(u16 instr) {
    return (instr >> 9) & 0x7;
}

static void set_r1(u16* instr, u8 r1) {
    *instr = (u16)(*instr | (u16)((r1 & 0x7) << 6));
}

static u8 get_r1(u16 instr) {
    return (instr >> 6) & 0x7;
}

static void set_r2(u16* instr, u8 r2) {
    *instr = (u16)(*instr | (u16)(r2 & 0x7));
}

static u8 get_r2(u16 instr) {
    return instr & 0x7;
}

static void set_imm_mode_and_value(u16* instr, i8 value) {
    *instr = (u16)(*instr | (1 << 5) | (value & 0x1F));
}

static Bool get_imm_mode(u16 instr) {
    return (instr >> 5) & 0x1;
}

static i8 get_imm_value(u16 instr) {
    return (i8)sign_extend(instr & 0x1F, 5);
}

static void set_pc_offset(u16* instr, u16 pc_offset) {
    *instr = (u16)(*instr | (pc_offset & 0x1FF));
}

static u16 get_pc_offset(u16 instr) {
    return sign_extend(instr & 0x1FF, 9);
}

static void do_op_add(u16 instr) {
    u8 r0 = get_r0(instr);
    u8 r1 = get_r1(instr);
    if (get_imm_mode(instr)) {
        // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
        // | 0   0   0   1 |     R0    |     R1    | 1 |     IMMEDIATE     |
        // +---------------+-----------+-----------+---+-------------------+
        REG[r0] = (u16)(REG[r1] + get_imm_value(instr));
    } else {
        // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
        // | 0   0   0   1 |     R0    |     R1    | 0 |  NULL |     R2    |
        // +---------------+-----------+-----------+---+-------+-----------+
        REG[r0] = (u16)(REG[r1] + REG[get_r2(instr)]);
    }
    set_flags(r0);
}

static u16 get_op_add(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // |               |           |           | 1 |     IMMEDIATE     |
    // | 0   0   0   1 |     R0    |     R1    +---+-------+-----------|
    // |               |           |           | 0 |  NULL |     R2    |
    // +---------------+-----------+-----------+---+-------+-----------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_ADD);
    set_r0(&bin_instr, instr.r0);
    set_r1(&bin_instr, instr.r1);
    if (instr.imm_mode) {
        set_imm_mode_and_value(&bin_instr, instr.opt.imm_value);
    } else {
        set_r2(&bin_instr, instr.opt.r2);
    }
    return bin_instr;
}

static void do_op_and(u16 instr) {
    u8 r0 = get_r0(instr);
    u8 r1 = get_r1(instr);
    if (get_imm_mode(instr)) {
        // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
        // | 0   0   0   1 |     R0    |     R1    | 1 |     IMMEDIATE     |
        // +---------------+-----------+-----------+---+-------------------+
        REG[r0] = (u16)(REG[r1] & get_imm_value(instr));
    } else {
        // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
        // | 0   0   0   1 |     R0    |     R1    | 0 |  NULL |     R2    |
        // +---------------+-----------+-----------+---+-------+-----------+
        REG[r0] = (u16)(REG[r1] & REG[get_r2(instr)]);
    }
    set_flags(r0);
}

static u16 get_op_and(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // |               |           |           | 1 |     IMMEDIATE     |
    // | 0   0   0   1 |     R0    |     R1    +---+-------+-----------|
    // |               |           |           | 0 |  NULL |     R2    |
    // +---------------+-----------+-----------+---+-------+-----------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_AND);
    set_r0(&bin_instr, instr.r0);
    set_r1(&bin_instr, instr.r1);
    if (instr.imm_mode) {
        set_imm_mode_and_value(&bin_instr, instr.opt.imm_value);
    } else {
        set_r2(&bin_instr, instr.opt.r2);
    }
    return bin_instr;
}

static void do_op_load(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   0   1   0 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u8  r0 = get_r0(instr);
    u16 pc_offset = get_pc_offset(instr);
    REG[r0] = mem_read((u16)(REG[R_PC] + pc_offset));
    set_flags(r0);
}

static u16 get_op_load(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   0   1   0 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_LD);
    set_r0(&bin_instr, instr.r0);
    set_pc_offset(&bin_instr, instr.pc_offset);
    return bin_instr;
}

static void do_op_load_indirect(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   0   1   0 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u8  r0 = get_r0(instr);
    u16 pc_offset = get_pc_offset(instr);
    REG[r0] = mem_read(mem_read((u16)(REG[R_PC] + pc_offset)));
    set_flags(r0);
}

static u16 get_op_load_indirect(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   0   1   0 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_LDI);
    set_r0(&bin_instr, instr.r0);
    set_pc_offset(&bin_instr, instr.pc_offset);
    return bin_instr;
}

#define NOT_IMPLEMENTED(op)                                  \
    {                                                        \
        fprintf(stderr, "OpCode:%d not implemented!\n", op); \
        exit(EXIT_FAILURE);                                  \
    }

static u16 get_bin_instr(Instr instr) {
    switch (instr.op) {
    case OP_BR: {
        NOT_IMPLEMENTED(OP_BR);
    }
    case OP_ADD: {
        return get_op_add(instr);
    }
    case OP_LD: {
        return get_op_load(instr);
    }
    case OP_AND: {
        return get_op_and(instr);
    }
    case OP_LDI: {
        return get_op_load_indirect(instr);
    }
    }
    exit(EXIT_FAILURE);
}

static void do_bin_instr(u16 instr) {
    OpCode op = (OpCode)(instr >> 12);
    switch (op) {
    case OP_BR: {
        NOT_IMPLEMENTED(OP_BR);
    }
    case OP_ADD: {
        do_op_add(instr);
        break;
    }
    case OP_LD: {
        do_op_load(instr);
        break;
    }
    case OP_AND: {
        do_op_and(instr);
        break;
    }
    case OP_LDI: {
        do_op_load_indirect(instr);
        break;
    }
    }
}

#endif
