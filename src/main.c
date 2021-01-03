#include "vm.h"

i32 main(i32 n, const char** args) {
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
