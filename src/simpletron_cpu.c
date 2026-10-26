#include <stdio.h>
#include "simpletron_cpu.h"

void reset_cpu()
{
    cpu_regs.accumulator = 0;
    cpu_regs.instructionCounter = 0;
    cpu_regs.instructionRegister = 0;
    cpu_regs.operand = 0;
    cpu_regs.operationCode = 0;
    cpu_regs.halt = 0;
}

void fetch()
{
    cpu_regs.instructionRegister = mem[cpu_regs.instructionCounter];
    cpu_regs.instructionCounter++;
}

void decode()
{
    cpu_regs.operationCode = cpu_regs.instructionRegister / 100;
    cpu_regs.operand = cpu_regs.instructionRegister % 100;
}

static const char *last_error = NULL;

const char *cpu_error()
{
    return last_error;
}

static cpu_status_t fail(const char *message)
{
    last_error = message;
    cpu_regs.halt = HALT;
    return CPU_ERROR;
}

// Simpletron words are limited to -9999..+9999
static cpu_status_t set_accumulator(long value)
{
    if (value < -9999 || value > 9999)
    {
        return fail("Accumulator overflow");
    }

    cpu_regs.accumulator = (int)value;
    return CPU_OK;
}

cpu_status_t execute()
{
    int word = mem[cpu_regs.operand];

    switch (cpu_regs.operationCode)
    {
    case READ:
        return CPU_NEED_INPUT;
    case WRITE:
        return CPU_OUTPUT;
    case LOAD:
        cpu_regs.accumulator = word;
        break;
    case STORE:
        mem[cpu_regs.operand] = cpu_regs.accumulator;
        break;
    case ADD:
        return set_accumulator((long)cpu_regs.accumulator + word);
    case SUBSTRACT:
        return set_accumulator((long)cpu_regs.accumulator - word);
    case DIVIDE:
        if (word == 0)
        {
            return fail("Attempt to divide by zero");
        }

        return set_accumulator(cpu_regs.accumulator / word);
    case MULTIPLY:
        return set_accumulator((long)cpu_regs.accumulator * word);
    case BRANCH:
        cpu_regs.instructionCounter = cpu_regs.operand;
        break;
    case BRANCHNEG:
        if (cpu_regs.accumulator < 0)
        {
            cpu_regs.instructionCounter = cpu_regs.operand;
        }

        break;
    case BRANCHZERO:
        if (cpu_regs.accumulator == 0)
        {
            cpu_regs.instructionCounter = cpu_regs.operand;
        }

        break;
    case HALT:
        cpu_regs.halt = HALT;
        return CPU_HALTED;
    default:
        return fail("Invalid operation code");
    }

    return CPU_OK;
}

// Runs one fetch-decode-execute cycle
cpu_status_t cpu_step()
{
    if (cpu_regs.instructionCounter < 0 || cpu_regs.instructionCounter >= MEMORY_SIZE)
    {
        return fail("Instruction counter out of range");
    }

    fetch();
    decode();

    // A negative word decodes to a negative operand, which would index outside memory
    if (cpu_regs.instructionRegister < 0)
    {
        return fail("Invalid operation code");
    }

    return execute();
}