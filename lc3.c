#include <stdio.h>
#include <stdint.h>
#include <signal.h>
// for windows
#include <Windows.h>
#include <conio.h>  // _kbhit

/* The LC-3 has 65,536 memory locations (the maximum that is addressable by a 16-bit unsigned integer 2^16), each of which stores a 16-bit value.
This means it can store a total of only 128KB! In our program, this memory will be stored in a simple array */
#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX]; // 65536 memory locations

// Registers
// The LC-3 has 10 total registers, each of which is 16 bits.
// The first 8 registers are general purpose, 1 program counter, and 1 condition flag.
enum
{
    R_R0 = 0, 
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC, /* program counter */
    R_COND,
    R_COUNT
};
// Store registers in an array
uint16_t reg[R_COUNT];

// Instruction set
// Tells the CPU to do basic operations like add two numbers
enum
{
    OP_BR = 0, /* branch */
    OP_ADD,    /* add  */
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

// Condition flags
// Allows programs to check logical conditions like if (x > 0) { ... }
enum
{
    FL_POS = 1 << 0, /* P */
    FL_ZRO = 1 << 1, /* Z */
    FL_NEG = 1 << 2, /* N */
};

// Trap codes
// Traps are used to perform I/O operations
enum
{
    TRAP_GETC = 0x20,  /* get character from keyboard, not echoed onto the terminal */
    TRAP_OUT = 0x21,   /* output a character */
    TRAP_PUTS = 0x22,  /* output a word string */
    TRAP_IN = 0x23,    /* get character from keyboard, echoed onto the terminal */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT = 0x25   /* halt the program */
};

// Memory-mapped registers
// These are special memory locations that are used to communicate with the outside world
enum
{
    MR_KBSR = 0xFE00, /* keyboard status */
    MR_KBDR = 0xFE02  /* keyboard data */
};

// Checks if a key has been pressed
int check_key()
{
    return _kbhit();
}

// Sign extends a number to 16 bits
uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1)
    {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

// LC-3 are big-endian. Most modern computers are little-endian.
// This function swaps the endianness of a number
// https://en.wikipedia.org/wiki/Endianness
uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}

// Update flags
// Updates the condition flags
void update_flags(uint16_t r)
{
    if (reg[r] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15) /* a 1 in the left-most bit indicates negative */
    {
        reg[R_COND] = FL_NEG;
    }
    else
    {
        reg[R_COND] = FL_POS;
    }
}

// Read image

void read_image_file(FILE *file)
{
    // The first 16 bits of the .obj file tell us where in memory to place the image
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    // we know the maximum file size so we only need one fread 
    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t *p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    // Swap to little endian
    while (read-- > 0)
    {
        *p = swap16(*p);
        ++p;
    }
}

// Load image
// Uses the read_image function to load a .obj file into memory
int read_image(const char *image_path)
{
    FILE *file = fopen(image_path, "rb");
    if (!file)
    {
        return 0;
    }
    read_image_file(file);
    fclose(file);
    return 1;
}

// Memory access 

void mem_write(uint16_t address, uint16_t val)
{
    memory[address] = val;
}

uint16_t mem_read(uint16_t address)
{
    if (address == MR_KBSR)
    {
        if (check_key())
        {
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }
        else
        {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}


