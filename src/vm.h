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
    // OP_TRAP,  // execute trap
} OpCode;

typedef struct {
    i16    immediate_or_offset;
    u8     r0_or_nzp;
    u8     r1;
    u8     r2;
    Bool   mode;
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

static u16 get_sign_extend(u16 x, u16 bit_count) {
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

static u16 get_mem_at(u16 address) {
    return MEM[address];
}

static void set_op(u16* instr, OpCode op) {
    *instr = (u16)(*instr | (op << 12));
}

static OpCode get_op(u16 instr) {
    return instr >> 12;
}

static void set_r0_or_nzp(u16* instr, u8 r0_or_nzp) {
    *instr = (u16)(*instr | ((r0_or_nzp & 0x7) << 9));
}

static u8 get_r0(u16 instr) {
    return (instr >> 9) & 0x7;
}

static void set_r1(u16* instr, u8 r1) {
    *instr = (u16)(*instr | ((r1 & 0x7) << 6));
}

static u8 get_r1(u16 instr) {
    return (instr >> 6) & 0x7;
}

static void set_r2(u16* instr, u8 r2) {
    *instr = (u16)(*instr | (r2 & 0x7));
}

static u8 get_r2(u16 instr) {
    return instr & 0x7;
}

static void set_immediate(u16* instr, i8 immediate) {
    *instr = (u16)(*instr | (1 << 5) | (immediate & 0x1F));
}

static Bool get_immediate_mode(u16 instr) {
    return (instr >> 5) & 0x1;
}

static i8 get_immediate(u16 instr) {
    return (i8)get_sign_extend(instr & 0x1F, 5);
}

static void set_pc_offset_9(u16* instr, i16 pc_offset) {
    *instr = (u16)(*instr | (pc_offset & 0x1FF));
}

static i16 get_pc_offset_9(u16 instr) {
    return (i16)get_sign_extend(instr & 0x1FF, 9);
}

static void set_relative_mode_and_pc_offset_11(u16* instr, i16 pc_offset) {
    *instr = (u16)(*instr | (1 << 11) | (pc_offset & 0x7FF));
}

static Bool get_relative_mode(u16 instr) {
    return (instr >> 11) & 0x1;
}

static i16 get_pc_offset_11(u16 instr) {
    return (i16)get_sign_extend(instr & 0x7FF, 11);
}

static u8 get_neg(u16 instr) {
    return (instr >> 11) & 0x1;
}

static u8 get_zero(u16 instr) {
    return (instr >> 10) & 0x1;
}

static u8 get_pos(u16 instr) {
    return (instr >> 9) & 0x1;
}

static void set_reg_offset_6(u16* instr, i16 pc_offset) {
    *instr = (u16)(*instr | (pc_offset & 0x3F));
}

static i16 get_reg_offset_6(u16 instr) {
    return (i16)get_sign_extend(instr & 0x3F, 6);
}

static void do_op_branch(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   0   0   0 | N | Z | P |             PC_OFFSET             |
    // +---------------+---+---+---+-----------------------------------+
    u16 r_cond = REG[R_COND];
    if ((get_neg(instr) & r_cond) || (get_zero(instr) & r_cond) ||
        (get_pos(instr) & r_cond))
    {
        REG[R_PC] = (u16)(REG[R_PC] + get_pc_offset_9(instr));
    }
}

static u16 get_op_branch(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   0   0   0 |    NZP    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_BR);
    set_r0_or_nzp(&bin_instr, instr.r0_or_nzp);
    set_pc_offset_9(&bin_instr, instr.immediate_or_offset);
    return bin_instr;
}

static void do_op_add(u16 instr) {
    u8 r0 = get_r0(instr);
    u8 r1 = get_r1(instr);
    if (get_immediate_mode(instr)) {
        // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
        // | 0   0   0   1 |     R0    |     R1    | 1 |     IMMEDIATE     |
        // +---------------+-----------+-----------+---+-------------------+
        REG[r0] = (u16)(REG[r1] + get_immediate(instr));
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
    set_r0_or_nzp(&bin_instr, instr.r0_or_nzp);
    set_r1(&bin_instr, instr.r1);
    if (instr.mode) {
        set_immediate(&bin_instr, (i8)instr.immediate_or_offset);
    } else {
        set_r2(&bin_instr, instr.r2);
    }
    return bin_instr;
}

static void do_op_and(u16 instr) {
    u8 r0 = get_r0(instr);
    u8 r1 = get_r1(instr);
    if (get_immediate_mode(instr)) {
        // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
        // | 0   1   0   1 |     R0    |     R1    | 1 |     IMMEDIATE     |
        // +---------------+-----------+-----------+---+-------------------+
        REG[r0] = (u16)(REG[r1] & get_immediate(instr));
    } else {
        // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
        // | 0   1   0   1 |     R0    |     R1    | 0 |  NULL |     R2    |
        // +---------------+-----------+-----------+---+-------+-----------+
        REG[r0] = (u16)(REG[r1] & REG[get_r2(instr)]);
    }
    set_flags(r0);
}

static u16 get_op_and(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // |               |           |           | 1 |     IMMEDIATE     |
    // | 0   1   0   1 |     R0    |     R1    +---+-------+-----------|
    // |               |           |           | 0 |  NULL |     R2    |
    // +---------------+-----------+-----------+---+-------+-----------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_AND);
    set_r0_or_nzp(&bin_instr, instr.r0_or_nzp);
    set_r1(&bin_instr, instr.r1);
    if (instr.mode) {
        set_immediate(&bin_instr, (i8)instr.immediate_or_offset);
    } else {
        set_r2(&bin_instr, instr.r2);
    }
    return bin_instr;
}

static void do_op_jump_subroutine(u16 instr) {
    REG[R_7] = REG[R_PC];
    if (get_relative_mode(instr)) {
        // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
        // | 0   1   0   0 | 1 |                 PC_OFFSET                 |
        // +---------------+---+-------------------------------------------+
        REG[R_PC] = (u16)(REG[R_PC] + get_pc_offset_11(instr));
    } else {
        // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
        // | 0   1   0   0 | 0 |  NULL |     R1    |          NULL         |
        // +---------------+---+-------+-----------+-----------------------+
        REG[R_PC] = REG[get_r1(instr)];
    }
}

static u16 get_op_jump_subroutine(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // |               | 1 |                 PC_OFFSET                 |
    // | 0   1   0   0 +---+-------+-----------+-----------------------+
    // |               | 0 |  NULL |     R1    |          NULL         |
    // +---------------+---+-------+-----------+-----------------------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_JSR);
    if (instr.mode) {
        set_relative_mode_and_pc_offset_11(&bin_instr,
                                           instr.immediate_or_offset);
    } else {
        set_r1(&bin_instr, instr.r1);
    }
    return bin_instr;
}

static void do_op_load(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   0   1   0 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u8 r0 = get_r0(instr);
    REG[r0] = get_mem_at((u16)(REG[R_PC] + get_pc_offset_9(instr)));
    set_flags(r0);
}

static u16 get_op_load(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   0   1   0 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_LD);
    set_r0_or_nzp(&bin_instr, instr.r0_or_nzp);
    set_pc_offset_9(&bin_instr, instr.immediate_or_offset);
    return bin_instr;
}

static void do_op_load_indirect(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   0   1   0 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u8 r0 = get_r0(instr);
    REG[r0] =
        get_mem_at(get_mem_at((u16)(REG[R_PC] + get_pc_offset_9(instr))));
    set_flags(r0);
}

static u16 get_op_load_indirect(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   0   1   0 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_LDI);
    set_r0_or_nzp(&bin_instr, instr.r0_or_nzp);
    set_pc_offset_9(&bin_instr, instr.immediate_or_offset);
    return bin_instr;
}

static void do_op_jump(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   1   0   0 |    NULL   |     R1    |          NULL         |
    // +---------------+-----------+-----------------------------------+
    REG[R_PC] = REG[get_r1(instr)];
}

static u16 get_op_jump(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   1   0   0 |    NULL   |     R1    |          NULL         |
    // +---------------+-----------+-----------------------------------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_JMP);
    set_r1(&bin_instr, instr.r1);
    return bin_instr;
}

static void do_op_store(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   0   1   1 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    MEM[REG[R_PC] + get_pc_offset_9(instr)] = get_r0(instr);
}

static u16 get_op_store(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   0   1   1 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_ST);
    set_r0_or_nzp(&bin_instr, instr.r0_or_nzp);
    set_pc_offset_9(&bin_instr, instr.immediate_or_offset);
    return bin_instr;
}

static void do_op_load_register(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   1   1   0 |     R0    |     R1    |         OFFSET        |
    // +---------------+-----------+-----------+-----------------------+
    u8 r0 = get_r0(instr);
    REG[r0] = get_mem_at((u16)(REG[get_r1(instr)] + get_reg_offset_6(instr)));
    set_flags(r0);
}

static u16 get_op_load_register(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   1   1   0 |     R0    |     R1    |         OFFSET        |
    // +---------------+-----------+-----------+-----------------------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_LDR);
    set_r0_or_nzp(&bin_instr, instr.r0_or_nzp);
    set_r1(&bin_instr, instr.r1);
    set_reg_offset_6(&bin_instr, instr.immediate_or_offset);
    return bin_instr;
}

static void do_op_store_indirect(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   0   1   1 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    MEM[get_mem_at((u16)(REG[R_PC] + get_pc_offset_9(instr)))] =
        REG[get_r0(instr)];
}

static u16 get_op_store_indirect(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   0   1   1 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_STI);
    set_r0_or_nzp(&bin_instr, instr.r0_or_nzp);
    set_pc_offset_9(&bin_instr, instr.immediate_or_offset);
    return bin_instr;
}

static void do_op_store_register(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   1   1   1 |     R0    |     R1    |         OFFSET        |
    // +---------------+-----------+-----------+-----------------------+
    MEM[REG[get_r1(instr)] + get_reg_offset_6(instr)] = REG[get_r0(instr)];
}

static u16 get_op_store_register(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   1   1   1 |     R0    |     R1    |         OFFSET        |
    // +---------------+-----------+-----------+-----------------------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_STR);
    set_r0_or_nzp(&bin_instr, instr.r0_or_nzp);
    set_r1(&bin_instr, instr.r1);
    set_reg_offset_6(&bin_instr, instr.immediate_or_offset);
    return bin_instr;
}

static void do_op_not(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   0   0   1 |     R0    |     R1    |          NULL         |
    // +---------------+-----------+-----------+-----------------------+
    u8 r0 = get_r0(instr);
    u8 r1 = get_r1(instr);
    REG[r0] = !REG[r1];
    set_flags(r0);
}

static u16 get_op_not(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   0   0   1 |     R0    |     R1    | 1 | 1 | 1 | 1 | 1 | 1 |
    // +---------------+-----------+-----------+---+---+---+---+---+---+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_NOT);
    set_r0_or_nzp(&bin_instr, instr.r0_or_nzp);
    set_r1(&bin_instr, instr.r1);
    set_reg_offset_6(&bin_instr, -1);
    return bin_instr;
}

static void do_op_load_effective_address(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   1   1   0 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u8 r0 = get_r0(instr);
    REG[r0] = (u16)(REG[R_PC] + get_pc_offset_9(instr));
    set_flags(r0);
}

static u16 get_op_load_effective_address(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   1   1   0 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_LEA);
    set_r0_or_nzp(&bin_instr, instr.r0_or_nzp);
    set_pc_offset_9(&bin_instr, instr.immediate_or_offset);
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
        return get_op_branch(instr);
    }
    case OP_ADD: {
        return get_op_add(instr);
    }
    case OP_LD: {
        return get_op_load(instr);
    }
    case OP_ST: {
        return get_op_store(instr);
    }
    case OP_JSR: {
        return get_op_jump_subroutine(instr);
    }
    case OP_AND: {
        return get_op_and(instr);
    }
    case OP_LDR: {
        return get_op_load_register(instr);
    }
    case OP_STR: {
        return get_op_store_register(instr);
    }
    case OP_NOT: {
        return get_op_not(instr);
    }
    case OP_LDI: {
        return get_op_load_indirect(instr);
    }
    case OP_STI: {
        return get_op_store_indirect(instr);
    }
    case OP_JMP: {
        return get_op_jump(instr);
    }
    case OP_LEA: {
        return get_op_load_effective_address(instr);
    }
    }
    exit(EXIT_FAILURE);
}

static void do_bin_instr(u16 instr) {
    OpCode op = (OpCode)(instr >> 12);
    switch (op) {
    case OP_BR: {
        do_op_branch(instr);
        break;
    }
    case OP_ADD: {
        do_op_add(instr);
        break;
    }
    case OP_LD: {
        do_op_load(instr);
        break;
    }
    case OP_ST: {
        do_op_store(instr);
        break;
    }
    case OP_JSR: {
        do_op_jump_subroutine(instr);
        break;
    }
    case OP_AND: {
        do_op_and(instr);
        break;
    }
    case OP_LDR: {
        do_op_load_register(instr);
        break;
    }
    case OP_STR: {
        do_op_store_register(instr);
        break;
    }
    case OP_NOT: {
        do_op_not(instr);
        break;
    }
    case OP_LDI: {
        do_op_load_indirect(instr);
        break;
    }
    case OP_STI: {
        do_op_store_indirect(instr);
        break;
    }
    case OP_JMP: {
        do_op_jump(instr);
        break;
    }
    case OP_LEA: {
        do_op_load_effective_address(instr);
        break;
    }
    }
}

#endif
