#include <string.h>

#include "pre_vm.h"

typedef struct {
    i16    immediate_or_offset;
    u8     r0_or_nzp;
    u8     r1;
    u8     r2;
    Trap   trap;
    Bool   mode;
    OpCode op;
} Instr;

#define FAIL(test)               \
    {                            \
        printf("!\n" test "\n"); \
        exit(EXIT_FAILURE);      \
    }

static void set_op(u16* instr, OpCode op) {
    *instr = (u16)(*instr | (op << 12));
}

static void set_r0_or_nzp(u16* instr, u8 r0_or_nzp) {
    *instr = (u16)(*instr | ((r0_or_nzp & 0x7) << 9));
}

static void set_r1(u16* instr, u8 r1) {
    *instr = (u16)(*instr | ((r1 & 0x7) << 6));
}

static void set_r2(u16* instr, u8 r2) {
    *instr = (u16)(*instr | (r2 & 0x7));
}

static void set_immediate(u16* instr, i8 immediate) {
    *instr = (u16)(*instr | (1 << 5) | (immediate & 0x1F));
}

static void set_pc_offset_9(u16* instr, i16 pc_offset) {
    *instr = (u16)(*instr | (pc_offset & 0x1FF));
}

static void set_relative_mode_and_pc_offset_11(u16* instr, i16 pc_offset) {
    *instr = (u16)(*instr | (1 << 11) | (pc_offset & 0x7FF));
}

static void set_reg_offset_6(u16* instr, i16 pc_offset) {
    *instr = (u16)(*instr | (pc_offset & 0x3F));
}

static void set_trap(u16* instr, Trap trap) {
    *instr = (u16)(*instr | (trap & 0xFF));
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

static u16 get_op_trap(Instr instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   1   1   1 |      NULL     |              TRAP             |
    // +---------------+---------------+-------------------------------+
    u16 bin_instr = 0;
    set_op(&bin_instr, OP_TRAP);
    set_trap(&bin_instr, instr.trap);
    return bin_instr;
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
    case OP_TRAP: {
        return get_op_trap(instr);
    }
    }
    exit(EXIT_FAILURE);
}

static void set_u16_to_string(char* buffer, u16 x) {
    u8 i = 15;
    u8 j = 0;
    for (;; --i) {
        if (!((j + 1) % 5)) {
            buffer[j++] = ' ';
        }
        buffer[j++] = (char)(((x >> i) & 0x1) + (u16)'0');
        if (i == 0) {
            break;
        }
    }
}

static void test_op_branch_neg(char* buffer) {
    Instr instr = {0};
    instr.op = OP_BR;
    instr.r0_or_nzp = FL_NEG;
    instr.immediate_or_offset = -10;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "0000 1001 1111 0110")) {
        FAIL("test_op_branch_neg (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) || (!((bin_instr >> 11) & 0x1)) ||
        (instr.immediate_or_offset != get_pc_offset_9(bin_instr)))
    {
        FAIL("test_op_branch_neg");
    }
    printf(".");
}

static void test_op_branch_zero(char* buffer) {
    Instr instr = {0};
    instr.op = OP_BR;
    instr.r0_or_nzp = FL_ZERO;
    instr.immediate_or_offset = -51;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "0000 0101 1100 1101")) {
        FAIL("test_op_branch_zero (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) || (!((bin_instr >> 10) & 0x1)) ||
        (instr.immediate_or_offset != get_pc_offset_9(bin_instr)))
    {
        FAIL("test_op_branch_zero");
    }
    printf(".");
}

static void test_op_branch_pos(char* buffer) {
    Instr instr = {0};
    instr.op = OP_BR;
    instr.r0_or_nzp = FL_POS;
    instr.immediate_or_offset = -27;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "0000 0011 1110 0101")) {
        FAIL("test_op_branch_pos (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) || (!((bin_instr >> 9) & 0x1)) ||
        (instr.immediate_or_offset != get_pc_offset_9(bin_instr)))
    {
        FAIL("test_op_branch_pos");
    }
    printf(".");
}

static void test_op_add(char* buffer) {
    Instr instr = {0};
    instr.op = OP_ADD;
    instr.r0_or_nzp = 3;
    instr.r1 = 5;
    instr.mode = FALSE;
    instr.r2 = 7;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "0001 0111 0100 0111")) {
        FAIL("test_op_add (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.r0_or_nzp != get_r0_or_nzp(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)) ||
        (instr.mode != get_immediate_mode(bin_instr)) ||
        (instr.r2 != get_r2(bin_instr)))
    {
        FAIL("test_op_add");
    }
    printf(".");
}

static void test_op_add_immediate_mode(char* buffer) {
    Instr instr = {0};
    instr.op = OP_ADD;
    instr.r0_or_nzp = 5;
    instr.r1 = 3;
    instr.mode = TRUE;
    instr.immediate_or_offset = -9;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "0001 1010 1111 0111")) {
        FAIL("test_op_add_immediate_mode (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.r0_or_nzp != get_r0_or_nzp(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)) ||
        (instr.mode != get_immediate_mode(bin_instr)) ||
        (instr.immediate_or_offset != get_immediate(bin_instr)))
    {
        FAIL("test_op_add_immediate_mode");
    }
    printf(".");
}

static void test_op_and(char* buffer) {
    Instr instr = {0};
    instr.op = OP_AND;
    instr.r0_or_nzp = 2;
    instr.r1 = 1;
    instr.mode = FALSE;
    instr.r2 = 4;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "0101 0100 0100 0100")) {
        FAIL("test_op_and (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.r0_or_nzp != get_r0_or_nzp(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)) ||
        (instr.mode != get_immediate_mode(bin_instr)) ||
        (instr.r2 != get_r2(bin_instr)))
    {
        FAIL("test_op_and");
    }
    printf(".");
}

static void test_op_and_immediate_mode(char* buffer) {
    Instr instr = {0};
    instr.op = OP_AND;
    instr.r0_or_nzp = 2;
    instr.r1 = 3;
    instr.mode = TRUE;
    instr.immediate_or_offset = 14;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "0101 0100 1110 1110")) {
        FAIL("test_op_and_immediate_mode (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.r0_or_nzp != get_r0_or_nzp(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)) ||
        (instr.mode != get_immediate_mode(bin_instr)) ||
        (instr.immediate_or_offset != get_immediate(bin_instr)))
    {
        FAIL("test_op_and_immediate_mode");
    }
    printf(".");
}

static void test_op_load(char* buffer) {
    Instr instr = {0};
    instr.op = OP_LD;
    instr.r0_or_nzp = 2;
    instr.immediate_or_offset = 66;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "0010 0100 0100 0010")) {
        FAIL("test_op_load (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.r0_or_nzp != get_r0_or_nzp(bin_instr)) ||
        (instr.immediate_or_offset != get_pc_offset_9(bin_instr)))
    {
        FAIL("test_op_load");
    }
    printf(".");
}

static void test_op_load_indirect(char* buffer) {
    Instr instr = {0};
    instr.op = OP_LDI;
    instr.r0_or_nzp = 6;
    instr.immediate_or_offset = 100;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "1010 1100 0110 0100")) {
        FAIL("test_op_load_indirect (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.r0_or_nzp != get_r0_or_nzp(bin_instr)) ||
        (instr.immediate_or_offset != get_pc_offset_9(bin_instr)))
    {
        FAIL("test_op_load_indirect");
    }
    printf(".");
}

static void test_op_jump(char* buffer) {
    Instr instr = {0};
    instr.op = OP_JMP;
    instr.r1 = 5;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "1100 0001 0100 0000")) {
        FAIL("test_op_jump (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) || (instr.r1 != get_r1(bin_instr))) {
        FAIL("test_op_jump");
    }
    printf(".");
}

static void test_op_jump_subroutine(char* buffer) {
    Instr instr = {0};
    instr.op = OP_JSR;
    instr.mode = FALSE;
    instr.r1 = 4;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "0100 0001 0000 0000")) {
        FAIL("test_op_jump_subroutine (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.mode != get_relative_mode(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)))
    {
        FAIL("test_op_jump_subroutine");
    }
    printf(".");
}

static void test_op_jump_subroutine_relative(char* buffer) {
    Instr instr = {0};
    instr.op = OP_JSR;
    instr.mode = TRUE;
    instr.immediate_or_offset = -500;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "0100 1110 0000 1100")) {
        FAIL("test_op_jump_subroutine_relative (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.mode != get_relative_mode(bin_instr)) ||
        (instr.immediate_or_offset != get_pc_offset_11(bin_instr)))
    {
        FAIL("test_op_jump_subroutine_relative");
    }
    printf(".");
}

static void test_op_store(char* buffer) {
    Instr instr = {0};
    instr.op = OP_ST;
    instr.r0_or_nzp = 6;
    instr.immediate_or_offset = -45;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "0011 1101 1101 0011")) {
        FAIL("test_op_store (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.r0_or_nzp != get_r0_or_nzp(bin_instr)) ||
        (instr.immediate_or_offset != get_pc_offset_9(bin_instr)))
    {
        FAIL("test_op_store");
    }
    printf(".");
}

static void test_op_load_register(char* buffer) {
    Instr instr = {0};
    instr.op = OP_LDR;
    instr.r0_or_nzp = 3;
    instr.r1 = 5;
    instr.immediate_or_offset = -31;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "0110 0111 0110 0001")) {
        FAIL("test_op_load_register (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.r0_or_nzp != get_r0_or_nzp(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)) ||
        (instr.immediate_or_offset != get_reg_offset_6(bin_instr)))
    {
        FAIL("test_op_load_register");
    }
    printf(".");
}

static void test_op_store_indirect(char* buffer) {
    Instr instr = {0};
    instr.op = OP_STI;
    instr.r0_or_nzp = 1;
    instr.immediate_or_offset = -250;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "1011 0011 0000 0110")) {
        FAIL("test_op_store_indirect (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.r0_or_nzp != get_r0_or_nzp(bin_instr)) ||
        (instr.immediate_or_offset != get_pc_offset_9(bin_instr)))
    {
        FAIL("test_op_store_indirect");
    }
    printf(".");
}

static void test_op_store_register(char* buffer) {
    Instr instr = {0};
    instr.op = OP_STR;
    instr.r0_or_nzp = 2;
    instr.r1 = 3;
    instr.immediate_or_offset = -29;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "0111 0100 1110 0011")) {
        FAIL("test_op_store_register (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.r0_or_nzp != get_r0_or_nzp(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)) ||
        (instr.immediate_or_offset != get_reg_offset_6(bin_instr)))
    {
        FAIL("test_op_store_register");
    }
    printf(".");
}

static void test_op_not(char* buffer) {
    Instr instr = {0};
    instr.op = OP_NOT;
    instr.r0_or_nzp = 5;
    instr.r1 = 4;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "1001 1011 0011 1111")) {
        FAIL("test_op_not (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.r0_or_nzp != get_r0_or_nzp(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)) || (-1 != get_reg_offset_6(bin_instr)))
    {
        FAIL("test_op_not");
    }
    printf(".");
}

static void test_op_load_effective_address(char* buffer) {
    Instr instr = {0};
    instr.op = OP_LEA;
    instr.r0_or_nzp = 1;
    instr.immediate_or_offset = -195;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "1110 0011 0011 1101")) {
        FAIL("test_op_load_effective_address (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) ||
        (instr.r0_or_nzp != get_r0_or_nzp(bin_instr)) ||
        (instr.immediate_or_offset != get_pc_offset_9(bin_instr)))
    {
        FAIL("test_op_load_effective_address");
    }
    printf(".");
}

static void test_op_trap_halt(char* buffer) {
    Instr instr = {0};
    instr.op = OP_TRAP;
    instr.trap = TRAP_HALT;
    u16 bin_instr = get_bin_instr(instr);
    set_u16_to_string(buffer, bin_instr);
    if (strcmp(buffer, "1111 0000 0010 0101")) {
        FAIL("test_op_trap_halt (strcmp)");
    }
    if ((instr.op != get_op(bin_instr)) || (instr.trap != get_trap(bin_instr)))
    {
        FAIL("test_op_trap_halt");
    }
    printf(".");
}

int main(void) {
    char* buffer = calloc(20, sizeof(char));
    if (buffer == NULL) {
        exit(EXIT_FAILURE);
    }
    test_op_branch_neg(buffer);
    test_op_branch_zero(buffer);
    test_op_branch_pos(buffer);
    test_op_add(buffer);
    test_op_add_immediate_mode(buffer);
    test_op_and(buffer);
    test_op_and_immediate_mode(buffer);
    test_op_load(buffer);
    test_op_load_indirect(buffer);
    test_op_jump(buffer);
    test_op_jump_subroutine(buffer);
    test_op_jump_subroutine_relative(buffer);
    test_op_store(buffer);
    test_op_load_register(buffer);
    test_op_store_indirect(buffer);
    test_op_store_register(buffer);
    test_op_not(buffer);
    test_op_load_effective_address(buffer);
    test_op_trap_halt(buffer);
    free(buffer);
    printf("\nDone!\n");
    return EXIT_SUCCESS;
}
