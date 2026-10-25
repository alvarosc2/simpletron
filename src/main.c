#include <ncurses.h>
#include <stdio.h>

#define MEMLEN 100

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

struct regs
{
    int accumulator;
    int instructionCounter;
    int instructionRegister;
    int operationCode;
    int operand;
    int halt;
} cpu_regs;

int mem[MEMLEN] = {0};

void display_intro()
{
    char *linea1 = "***           Welcome to Simpletron           ***";
    char *linea2 = "*** Please enter your program one instruction ***";
    char *linea3 = "*** (or data word) at a time. I will type the ***";
    char *linea4 = "*** location number and a question mark (?).  ***";
    char *linea5 = "*** You then type the word for that location. ***";
    char *linea6 = "*** Type the sentinel -99999 to stop entering ***";
    char *linea7 = "*** your program.                             ***";

    printf("%s\n", linea1);
    printf("%s\n", linea2);
    printf("%s\n", linea3);
    printf("%s\n", linea4);
    printf("%s\n", linea5);
    printf("%s\n", linea6);
    printf("%s\n\n", linea7);
}

void display_registers()
{
    printf("REGISTERS: \n");
    printf("accumulator %+4.4d\n", cpu_regs.accumulator);
    printf("instructionCounter %2d\n", cpu_regs.instructionCounter);
    printf("instructionRegister %+4.4d\n", cpu_regs.instructionRegister);
    printf("operationCode %2d\n", cpu_regs.operationCode);
    printf("operand %2d\n", cpu_regs.operand);
    printf("haltSignal %2d\n\n", cpu_regs.halt);
}

void display_memory()
{
    printf("MEMORY: \n");

    // Table Header
    for (int h = 0; h < 10; h++)
    {
        printf("%6d", h);
    }

    printf("\n");

    // Table data
    for (int i = 0; i < 10; i++)
    {
        printf("%3d", 10 * i);

        for (int j = 0; j < 10; j++)
        {
            printf("%+6.4d", mem[i * 10 + j]);
        }
        printf("\n");
    }
}

void line_editor()
{
    int memory_address = 0;
    char *prompt = "?";

    int opcode = 0;

    while (true)
    {
        printf("%2.2d %s ", memory_address, prompt);
        scanf("%d", &opcode);

        if (opcode == -99999)
        {
            break;
        }

        mem[memory_address] = opcode;
        memory_address++;
    }

    char *program_loaded_msg = "*** Program loading completed ***";
    char *start_executing_msg = "*** Program execution begins  ***";
    printf("%s\n", program_loaded_msg);
    printf("%s\n\n", start_executing_msg);
}

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
}

void decode()
{
    cpu_regs.operationCode = cpu_regs.instructionRegister / 100;
    cpu_regs.operand = cpu_regs.instructionRegister % 100;
}

void execute()
{
    switch (cpu_regs.operationCode)
    {
    case READ:
        printf("Input:\n");
        printf("> ");
        scanf("%d", &mem[cpu_regs.operand]);
        break;
    case WRITE:
        printf("Output:\n");
        printf("%d\n", mem[cpu_regs.operand]);
        break;
    case LOAD:
        cpu_regs.accumulator = cpu_regs.operand;
        break;
    case STORE:
        mem[cpu_regs.operand] = cpu_regs.accumulator;
        break;
    case ADD:
        cpu_regs.accumulator += cpu_regs.operand;
        break;
    case SUBSTRACT:
        cpu_regs.accumulator -= cpu_regs.operand;
        break;
    case DIVIDE:
        cpu_regs.accumulator /= cpu_regs.operand;
        break;
    case MULTIPLY:
        cpu_regs.accumulator *= cpu_regs.operand;
        break;
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
        break;
    default:
        break;
    }

    cpu_regs.instructionCounter++;
}

void execution_cycle()
{
    reset_cpu();

    while (cpu_regs.halt != HALT)
    {
        fetch();
        decode();
        execute();
    }
}

void UI()
{
    initscr();            // entra en modo curses
    cbreak();             // teclas sin esperar Enter
    noecho();             // no imprime lo que tecleas
    keypad(stdscr, TRUE); // flechas, F1..F12

    WINDOW *regs = newwin(8, 30, 0, 0); // alto, ancho, y, x

    box(regs, 0, 0);
    mvwprintw(regs, 0, 2, " SIMPLETRON ");
    mvwprintw(regs, 1, 2, "accumulator: %+05d", 0);

    refresh();      // refresca stdscr primero
    wrefresh(regs); // luego cada ventana

    getch(); // espera una tecla
    delwin(regs);
    endwin(); // restaura la terminal (¡siempre!)
}

int main(int argc, char *argv[])
{
    display_intro();
    line_editor();
    execution_cycle();
    display_registers();
    display_memory();

    return 0;
}