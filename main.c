
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
    OP_ST,     /* store */
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
    FL_POS = 1 << 0, // P
    FL_ZRO = 1 << 1, // Z
    FL_NEG = 1 << 2, // N
};

/* memory mapped registers */
enum {
    MR_KBSR = 0xFE00, /* KEYBOARD STATUS */
    MR_KBDR = 0xFE02  /* KEYBOARD DATA */
};

u_int16_t swap16(u_int16_t x) {
    return (x << 8) | (x >> 8);
};

/* reading an LC-3 program into memory */
void read_image_file(FILE* file) {
    /* origin here tells us where in memory to place the image */
    u_int16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    /* since we know the max file size we only need one fread */
    u_int16_t max_read = MEMORY_MAX - origin;
    u_int16_t* p = memory + origin;
    size_t read = fread(p, sizeof(u_int16_t), max_read, file);

    /* here we swap to little endian */
    while(read-->0) {
        *p = swap16(*p);
        ++p;
    }
};

/* convenience function for reading_image_file which reads an LC-3 program into memory   */
int read_image(const char* image_path) {
    FILE* file = fopen(image_path, "rb");
    if(!file) { return 0; }

    read_image_file(file);
    fclose(file);

    return 1;
};

/* MAC/LINUX */
struct termios original_tio;

void disable_input_buffering() {
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
};

u_int16_t check_key() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    return select(1, &readfds, NULL, NULL, &timeout) != 0;
};

/* interrupt handling */
void handle_interrupt(int signal) {
    restore_input_buffering();
    printf("\n");
    exit(-2);
};

/* for reading of memory mapped registers */
u_int16_t mem_read(u_int16_t address) {
    if(address == MR_KBSR) {
        if(check_key()) {

            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();

        } else {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
};




/* implementation of the instructions */
/* sign extension: when using immediate mode we need to add a 5 bit number to a 16
 * bit number and so we need to extend the 5 bit number to match 16 bits, for
 * positive numbers we just add 0s but negatives we add 1s to preserve the sign */
u_int16_t sign_extend(u_int16_t x, int bit_count) {
    if((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
};

/* updating conditions flag */
void update_flag(u_int16_t r) {
    if(registers[r] == 0) {
        registers[R_COND] = FL_ZRO;
    } else if(registers[r] >> 15) { /* a 1 in the left-most bit indicates negative */
        registers[R_COND] = FL_NEG;
    } else {
        registers[R_COND] = FL_POS;
    }
}


int main(int argc, char** argv) {

    if(argc < 2) {
        /* display the usage tip */
        printf("LC-3 [image-file1]...\n");
        exit(2);
    }

    for(int i=1; i<argc; i++) {
        if(!read_image(argv[i])) {
            printf("Failed to load image: %s\n", argv[i]);
            exit(1);
        }
    }

    signal(SIGINT, handle_interrupt);
    disable_input_buffering();
    /* only one condition flag should be set at any given time, so we set
     * the Z flag */
    registers[R_COND] = FL_ZRO;

    /* the program counter is set to the starting address
     * 0x3000 is the default */
    enum { PC_START = 0x3000 };
    registers[R_PC] = PC_START;

    int running = 1;
    while(running) {
        /* fetch */
        u_int16_t instruction = mem_read(registers[R_PC]);
        u_int16_t opcode = instruction >> 12;

        switch(opcode) {
            case OP_ADD: {
                /* add instruction */
                /* destination register (DR) */
                u_int16_t r0 = (instruction >> 9) & 0x7;
                /* the first operand (SR1) */
                u_int16_t r1 = (instruction >> 6) & 0x7;
                /* if we are in immediate mode */
                u_int16_t imm_flag = (instruction >> 5) & 0x1;

                if(imm_flag) {
                    u_int16_t imm5 = sign_extend(instruction & 0x1F, 5);
                    registers[r0] = registers[r1] + imm5;
                } else {
                    u_int16_t r2 = instruction & 0x7;
                    registers[r0] = registers[r1] + registers[r2]; /* add the two values in the two registers */
                }

                update_flag(r0);
            }
                break;
            case OP_AND:
                /* and instruction */
            case OP_NOT:
                /* not instruction */
                break;
            case OP_BR:
                /* branch op code */
                break;
            case OP_JMP:
                /* perform jump */
                break;
            case OP_JSR:
                /* jump register */
                break;
            case OP_LD:
                /* load */
                break;
            case OP_LDI:
                /* load indirect */
                break;
            case OP_LDR:
                /* load register */
                break;
            case OP_LEA:
                /* load affective address */
                break;
            case OP_ST:
                /* store */
                break;
            case OP_STI:
                /* store indirect */
                break;
            case OP_STR:
                /* store register */
                break;
            case OP_TRAP:
                /* trap signal */
                break;
            case OP_RES:
            case OP_RTI:
            default:
                /* reserved and unused default to bad opcode */
                break;
        }

    }

    restore_input_buffering();
    return 0;
}


