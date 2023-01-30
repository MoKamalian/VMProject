
/*
 * author: amir kamalian
 * date:   23 jan 2O23
 * This program follows the tutorial on how to create a VM: https://www.jmeiners.com/lc3-vm/#-lc3.c-block-99
 * The VM will be the LC-3 (Little Computer 3)
 */


#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/termios.h>
#include <sys/mman.h>

/* maximum memory size for out VM: 65536 address locations */
#define MEMORY_MAX (1 << 16)
u_int16_t memory[MEMORY_MAX];

/* CPU registers (the workbench of the CPU); the LC-3 will have 10
 * 8 general purpose and 1 program counter and 1 for conditional flags */
enum {
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC, /* the program counter: holds the address of the next instruction */
    R_COND,
    R_COUNT /* count of the total number of CPU registers, i.e. 10 total */
};
u_int16_t registers[R_COUNT];

/* opcodes of the LC-3; i.e. the instruction set of the computer
 * each instruction has an opcode (which is the left 4 bits of
 * the instruction) and the other 12 bits are for storing parameters
 * Below are all the opcodes we will be using */
enum {
    OP_BR = 0, /* branch opcode */
    OP_ADD,    /* add opcode */
    OP_LD,     /* load */
    OP_SD,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap */
};

/* condition flags provide information on the most recent executed calculation
 * they signal various situation about the instruction that was executed */
enum {
    FL_POS = 1 << 0,
    FL_ZRO = 1 << 1,
    FL_NEG = 1 << 2,
};




int main() {




    return 0;
}


