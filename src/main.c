#include "vm.h"

#include <signal.h>
#include <termios.h>

typedef FILE           File;
typedef struct termios TermIos;

static TermIos TERMINAL;

static void disable_input_buffering(void) {
    tcgetattr(STDIN_FILENO, &TERMINAL);
    TermIos terminal = TERMINAL;
    terminal.c_lflag =
        (tcflag_t)(terminal.c_lflag & (tcflag_t)(~ICANON & ~ECHO));
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
