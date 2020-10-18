#include "vm.h"

#define FAIL(test)               \
    {                            \
        printf("!\n" test "\n"); \
        exit(EXIT_FAILURE);      \
    }

static void test_op_add(void) {
    Instr instr = {0};
    instr.op = OP_ADD;
    instr.r0 = 3;
    instr.r1 = 5;
    instr.imm_mode = FALSE;
    instr.opt.r2 = 7;
    u16 bin_instr = get_bin_instr(instr);
    if ((instr.op != get_op(bin_instr)) || (instr.r0 != get_r0(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)) ||
        (instr.imm_mode != get_imm_mode(bin_instr)) ||
        (instr.opt.r2 != get_r2(bin_instr)))
    {
        FAIL("test_op_add");
    }
    printf(".");
}

static void test_op_add_imm_mode(void) {
    Instr instr = {0};
    instr.op = OP_ADD;
    instr.r0 = 5;
    instr.r1 = 3;
    instr.imm_mode = TRUE;
    instr.opt.imm_value = -15;
    u16 bin_instr = get_bin_instr(instr);
    if ((instr.op != get_op(bin_instr)) || (instr.r0 != get_r0(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)) ||
        (instr.imm_mode != get_imm_mode(bin_instr)) ||
        (instr.opt.imm_value != get_imm_value(bin_instr)))
    {
        FAIL("test_op_add_imm_mode");
    }
    printf(".");
}

static void test_op_and(void) {
    Instr instr = {0};
    instr.op = OP_AND;
    instr.r0 = 2;
    instr.r1 = 1;
    instr.imm_mode = FALSE;
    instr.opt.r2 = 4;
    u16 bin_instr = get_bin_instr(instr);
    if ((instr.op != get_op(bin_instr)) || (instr.r0 != get_r0(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)) ||
        (instr.imm_mode != get_imm_mode(bin_instr)) ||
        (instr.opt.r2 != get_r2(bin_instr)))
    {
        FAIL("test_op_and");
    }
    printf(".");
}

static void test_op_and_imm_mode(void) {
    Instr instr = {0};
    instr.op = OP_AND;
    instr.r0 = 2;
    instr.r1 = 3;
    instr.imm_mode = TRUE;
    instr.opt.imm_value = 14;
    u16 bin_instr = get_bin_instr(instr);
    if ((instr.op != get_op(bin_instr)) || (instr.r0 != get_r0(bin_instr)) ||
        (instr.r1 != get_r1(bin_instr)) ||
        (instr.imm_mode != get_imm_mode(bin_instr)) ||
        (instr.opt.imm_value != get_imm_value(bin_instr)))
    {
        FAIL("test_op_and_imm_mode");
    }
    printf(".");
}

static void test_op_load(void) {
    Instr instr = {0};
    instr.op = OP_LD;
    instr.r0 = 2;
    instr.pc_offset = 66;
    u16 bin_instr = get_bin_instr(instr);
    if ((instr.op != get_op(bin_instr)) || (instr.r0 != get_r0(bin_instr)) ||
        (instr.pc_offset != get_pc_offset(bin_instr)))
    {
        FAIL("test_op_load");
    }
    printf(".");
}

static void test_op_load_indirect(void) {
    Instr instr = {0};
    instr.op = OP_LDI;
    instr.r0 = 6;
    instr.pc_offset = 100;
    u16 bin_instr = get_bin_instr(instr);
    if ((instr.op != get_op(bin_instr)) || (instr.r0 != get_r0(bin_instr)) ||
        (instr.pc_offset != get_pc_offset(bin_instr)))
    {
        FAIL("test_op_load_indirect");
    }
    printf(".");
}

int main(void) {
    test_op_add();
    test_op_add_imm_mode();
    test_op_and();
    test_op_and_imm_mode();
    test_op_load();
    test_op_load_indirect();
    printf("\nDone!\n");
    return EXIT_SUCCESS;
}