#include "vm.h"

#include <signal.h>
#include <termios.h>

typedef FILE           File;
typedef struct termios TermIos;
typedef tcflag_t       TcFlag;

static TermIos TERMINAL;

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

static Bool poll_keyboard(void) {
    FdSet file_descriptors;
    FD_ZERO(&file_descriptors);
    FD_SET(STDIN_FILENO, &file_descriptors);
    TimeVal timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &file_descriptors, NULL, NULL, &timeout) != 0;
}

static u16 get_mem_at(u16 address) {
    if (address == KEYBOARD_STATUS) {
        if (poll_keyboard()) {
            MEM[KEYBOARD_STATUS] = (1 << 15);
            MEM[KEYBOARD_DATA] = (u16)getchar();
        } else {
            MEM[KEYBOARD_STATUS] = 0;
        }
    }
    return MEM[address];
}

static void do_op_branch(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   0   0   0 | N | Z | P |             PC_OFFSET             |
    // +---------------+---+---+---+-----------------------------------+
    if (REG[R_COND] & get_r0_or_nzp(instr)) {
        REG[R_PC] = (u16)(REG[R_PC] + get_pc_offset_9(instr));
    }
}

static void do_op_add(u16 instr) {
    u8 r0 = get_r0_or_nzp(instr);
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

static void do_op_load(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   0   1   0 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u8 r0 = get_r0_or_nzp(instr);
    REG[r0] = get_mem_at((u16)(REG[R_PC] + get_pc_offset_9(instr)));
    set_flags(r0);
}

static void do_op_store(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   0   1   1 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    MEM[REG[R_PC] + get_pc_offset_9(instr)] = REG[get_r0_or_nzp(instr)];
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

static void do_op_and(u16 instr) {
    u8 r0 = get_r0_or_nzp(instr);
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

static void do_op_load_register(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   1   1   0 |     R0    |     R1    |         OFFSET        |
    // +---------------+-----------+-----------+-----------------------+
    u8 r0 = get_r0_or_nzp(instr);
    REG[r0] = get_mem_at((u16)(REG[get_r1(instr)] + get_reg_offset_6(instr)));
    set_flags(r0);
}

static void do_op_store_register(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   1   1   1 |     R0    |     R1    |         OFFSET        |
    // +---------------+-----------+-----------+-----------------------+
    MEM[REG[get_r1(instr)] + get_reg_offset_6(instr)] =
        REG[get_r0_or_nzp(instr)];
}

static void do_op_not(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   0   0   1 |     R0    |     R1    |          NULL         |
    // +---------------+-----------+-----------+-----------------------+
    u8 r0 = get_r0_or_nzp(instr);
    REG[r0] = (u16)(~REG[get_r1(instr)]);
    set_flags(r0);
}

static void do_op_load_indirect(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   0   1   0 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u8 r0 = get_r0_or_nzp(instr);
    REG[r0] =
        get_mem_at(get_mem_at((u16)(REG[R_PC] + get_pc_offset_9(instr))));
    set_flags(r0);
}

static void do_op_store_indirect(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   0   1   1 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    MEM[get_mem_at((u16)(REG[R_PC] + get_pc_offset_9(instr)))] =
        REG[get_r0_or_nzp(instr)];
}

static void do_op_jump(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   1   0   0 |    NULL   |     R1    |          NULL         |
    // +---------------+-----------+-----------------------------------+
    REG[R_PC] = REG[get_r1(instr)];
}

static void do_op_load_effective_address(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   1   1   0 |     R0    |             PC_OFFSET             |
    // +---------------+-----------+-----------------------------------+
    u8 r0 = get_r0_or_nzp(instr);
    REG[r0] = (u16)(REG[R_PC] + get_pc_offset_9(instr));
    set_flags(r0);
}

static void do_op_trap(u16 instr) {
    // | 15| 14| 13| 12| 11| 10| 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 1   1   1   1 |      NULL     |              TRAP             |
    // +---------------+---------------+-------------------------------+
    switch (get_trap(instr)) {
    case TRAP_GETC: {
        REG[R_0] = (u16)getchar();
        break;
    }
    case TRAP_OUT: {
        putc((char)REG[R_0], stdout);
        fflush(stdout);
        break;
    }
    case TRAP_PUTS: {
        for (u16* x = MEM + REG[R_0]; *x; ++x) {
            putc((char)*x, stdout);
        }
        fflush(stdout);
        break;
    }
    case TRAP_IN: {
        printf("Enter a character: ");
        char x = (char)getchar();
        putc(x, stdout);
        REG[R_0] = (u16)x;
        break;
    }
    case TRAP_PUTSP: {
        for (u16* x = MEM + REG[R_0]; *x; ++x) {
            char a = (char)((*x) & 0xFF);
            putc(a, stdout);
            char b = (char)((*x) >> 8);
            if (b) {
                putc(b, stdout);
            }
        }
        fflush(stdout);
        break;
    }
    case TRAP_HALT: {
        puts("HALT");
        fflush(stdout);
        STATUS = DEAD;
        break;
    }
    }
}

static void do_bin_instr(u16 instr) {
    switch (get_op(instr)) {
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
    case OP_TRAP: {
        do_op_trap(instr);
        break;
    }
    }
}

static void disable_input_buffering(void) {
    tcgetattr(STDIN_FILENO, &TERMINAL);
    TermIos terminal = TERMINAL;
    terminal.c_lflag = (TcFlag)(terminal.c_lflag & (TcFlag)(~ICANON & ~ECHO));
    tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
}

static void restore_input_buffering(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &TERMINAL);
}

static void handle_interrupt(i32 _) {
    restore_input_buffering();
    printf("\n");
    exit(EXIT_FAILURE);
}

static u16 swap_endian_16(u16 x) {
    return (u16)((x << 8) | (x >> 8));
}

static void set_bytecode(File* file) {
    u16 origin;
    if (fread(&origin, sizeof(u16), 1, file) == 0) {
        exit(EXIT_FAILURE);
    }
    origin = swap_endian_16(origin);
    u16*  index = MEM + origin;
    usize bytes = fread(index, sizeof(u16), (usize)(U16_MAX - origin), file);
    if (bytes == 0) {
        exit(EXIT_FAILURE);
    }
    while (0 < bytes--) {
        *index = swap_endian_16(*index);
        ++index;
    }
}

int main(int n, const char* args[]) {
    {
        if (n < 2) {
            exit(EXIT_FAILURE);
        }
        File* file = fopen(args[1], "rb");
        if (file == NULL) {
            exit(EXIT_FAILURE);
        };
        set_bytecode(file);
        fclose(file);
    }
    {
        signal(SIGINT, handle_interrupt);
        disable_input_buffering();
        REG[R_PC] = PC_START;
        while (STATUS) {
            do_bin_instr(get_mem_at(REG[R_PC]++));
        }
        restore_input_buffering();
    }
    return EXIT_SUCCESS;
}
