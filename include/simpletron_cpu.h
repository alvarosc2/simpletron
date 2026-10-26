#ifndef SIMPLETRON_CPU_H
#define SIMPLETRON_CPU_H

#include "simpletron_memory.h"
#include "simpletron_registers.h"

// input/output operations
#define READ 10
#define WRITE 11

// load/store operations
#define LOAD 20
#define STORE 21

// arithmetic operations
#define ADD 30
#define SUBSTRACT 31
#define DIVIDE 32
#define MULTIPLY 33

// transfer-of-control operations
#define BRANCH 40
#define BRANCHNEG 41
#define BRANCHZERO 42
#define HALT 43

typedef enum
{
    CPU_OK,
    CPU_NEED_INPUT, // READ: the caller must store a word in mem[cpu_regs.operand]
    CPU_OUTPUT,     // WRITE: the caller must show mem[cpu_regs.operand]
    CPU_HALTED,
    CPU_ERROR
} cpu_status_t;

void reset_cpu();
void fetch();
void decode();
cpu_status_t execute();
cpu_status_t cpu_step();
const char *cpu_error();

#endif