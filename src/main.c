#include "vm.h"

int main(void) {
    printf("\n"
           "sizeof(Bool)     : %zu\n"
           "sizeof(Register) : %zu\n"
           "sizeof(OpCode)   : %zu\n"
           "sizeof(Instr)    : %zu\n"
           "sizeof(Status)   : %zu\n"
           "sizeof(MEM)      : %zu\n"
           "sizeof(REG)      : %zu\n",
           sizeof(Bool),
           sizeof(Register),
           sizeof(OpCode),
           sizeof(Instr),
           sizeof(Status),
           sizeof(MEM),
           sizeof(REG));
    STATUS = DEAD;
    REG[R_PC] = PC_START;
    while (STATUS) {
        do_bin_instr(mem_read(REG[R_PC]++));
        STATUS = DEAD;
    }
    printf("Done!\n");
    return EXIT_SUCCESS;
}
