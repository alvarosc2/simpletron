#ifndef SIMPLETRON_REGISTERS_H
#define SIMPLETRON_REGISTERS_H

#include "config.h"

struct regs
{
    int accumulator;
    int instructionCounter;
    int instructionRegister;
    int operationCode;
    int operand;
    int halt;
};

extern struct regs cpu_regs;

#endif