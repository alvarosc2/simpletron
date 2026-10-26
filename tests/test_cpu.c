#include <criterion/criterion.h>
#include <string.h>
#include "simpletron_cpu.h"

static void load(const int *words, int count)
{
    memset(mem, 0, sizeof(mem));
    memcpy(mem, words, sizeof(int) * (size_t)count);
    reset_cpu();
}

// Runs until HALT/ERROR, feeding `inputs` to READ and collecting WRITE outputs
static cpu_status_t run(const int *inputs, int *outputs, int *out_count)
{
    int in = 0;
    *out_count = 0;

    for (int steps = 0; steps < 1000; steps++)
    {
        cpu_status_t status = cpu_step();

        if (status == CPU_NEED_INPUT)
            mem[cpu_regs.operand] = inputs[in++];
        else if (status == CPU_OUTPUT)
            outputs[(*out_count)++] = mem[cpu_regs.operand];
        else if (status == CPU_HALTED || status == CPU_ERROR)
            return status;
    }

    return CPU_OK;
}

Test(cpu, adds_two_numbers)
{
    int program[] = {1007, 1008, 2007, 3008, 2109, 1109, 4300};
    int inputs[] = {5, 5}, outputs[4], count;

    load(program, 7);
    cr_assert_eq(run(inputs, outputs, &count), CPU_HALTED);
    cr_assert_eq(count, 1);
    cr_assert_eq(outputs[0], 10);
}

Test(cpu, divide_by_zero_is_an_error)
{
    int program[] = {2005, 3205, 4300, 0, 0, 0};
    int outputs[1], count;

    load(program, 6);
    cr_assert_eq(run(NULL, outputs, &count), CPU_ERROR);
    cr_assert_str_eq(cpu_error(), "Attempt to divide by zero");
}

Test(cpu, invalid_opcode_is_an_error)
{
    int program[] = {9900};
    int outputs[1], count;

    load(program, 1);
    cr_assert_eq(run(NULL, outputs, &count), CPU_ERROR);
}

Test(cpu, accumulator_overflow_is_an_error)
{
    int program[] = {2003, 3303, 4300, 9999};
    int outputs[1], count;

    load(program, 4);
    cr_assert_eq(run(NULL, outputs, &count), CPU_ERROR);
}

Test(cpu, branchzero_jumps_when_accumulator_is_zero)
{
    int program[] = {4202, 4300, 4300};

    load(program, 3);
    cr_assert_eq(cpu_step(), CPU_OK);
    cr_assert_eq(cpu_regs.instructionCounter, 2);
}
