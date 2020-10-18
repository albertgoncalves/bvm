#include <string.h>

#include "vm.h"

#define FAIL(test)               \
    {                            \
        printf("!\n" test "\n"); \
        exit(EXIT_FAILURE);      \
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
    if ((instr.op != get_op(bin_instr)) || (!get_neg(bin_instr)) ||
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
    if ((instr.op != get_op(bin_instr)) || (!get_zero(bin_instr)) ||
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
    if ((instr.op != get_op(bin_instr)) || (!get_pos(bin_instr)) ||
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
        (instr.r0_or_nzp != get_r0(bin_instr)) ||
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
        (instr.r0_or_nzp != get_r0(bin_instr)) ||
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
        (instr.r0_or_nzp != get_r0(bin_instr)) ||
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
        (instr.r0_or_nzp != get_r0(bin_instr)) ||
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
        (instr.r0_or_nzp != get_r0(bin_instr)) ||
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
        (instr.r0_or_nzp != get_r0(bin_instr)) ||
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
        (instr.r0_or_nzp != get_r0(bin_instr)) ||
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
        (instr.r0_or_nzp != get_r0(bin_instr)) ||
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
        (instr.r0_or_nzp != get_r0(bin_instr)) ||
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
        (instr.r0_or_nzp != get_r0(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)) ||
        (instr.immediate_or_offset != get_reg_offset_6(bin_instr)))
    {
        FAIL("test_op_store_indirect");
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
        (instr.r0_or_nzp != get_r0(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)) ||
        (-1 != get_reg_offset_6(bin_instr)))
    {
        FAIL("test_op_not");
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
    free(buffer);
    printf("\nDone!\n");
    return EXIT_SUCCESS;
}
