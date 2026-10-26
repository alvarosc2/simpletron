#include "simpletron_cpu.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define TERMINAL_GREEN 0, 255, 70

void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y)
{
    if (text[0] == '\0')
    {
        return;
    }

    SDL_Color green = {TERMINAL_GREEN, 255};
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, green);
    if (surface == NULL)
    {
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dest = {x, y, surface->w, surface->h};
    SDL_FreeSurface(surface);

    if (texture != NULL)
    {
        SDL_RenderCopy(renderer, texture, NULL, &dest);
        SDL_DestroyTexture(texture);
    }
}

#define TERM_MAX_LINES 256
#define TERM_LINE_LEN 80
#define INPUT_MAX_LEN 12
#define SENTINEL (-99999)
#define WORD_MAX 9999

static char term_lines[TERM_MAX_LINES][TERM_LINE_LEN];
static int term_count = 0;

// Appends a line to the scrollback, discarding the oldest one when full
void term_print(const char *text)
{
    if (term_count == TERM_MAX_LINES)
    {
        memmove(term_lines[0], term_lines[1], sizeof(term_lines[0]) * (TERM_MAX_LINES - 1));
        term_count--;
    }

    snprintf(term_lines[term_count++], TERM_LINE_LEN, "%s", text);
}

// Draws the tail of the scrollback that fits in the window, plus the prompt (if any)
void term_draw(SDL_Renderer *renderer, TTF_Font *font, const char *prompt)
{
    int line_height = TTF_FontLineSkip(font);
    int rows = (WINDOW_HEIGHT * WINDOW_MULTIPLIER - 2 * TERMINAL_MARGIN) / line_height;
    int shown = prompt ? rows - 1 : rows;
    int first = term_count > shown ? term_count - shown : 0;
    int y = TERMINAL_MARGIN;

    for (int i = first; i < term_count; i++, y += line_height)
    {
        draw_text(renderer, font, term_lines[i], TERMINAL_MARGIN, y);
    }

    if (prompt)
    {
        draw_text(renderer, font, prompt, TERMINAL_MARGIN, y);
    }
}

typedef enum
{
    PHASE_LOADING,
    PHASE_RUNNING,
    PHASE_WAITING_INPUT,
    PHASE_FINISHED
} phase_t;

typedef struct
{
    phase_t phase;
    int address; // next memory location to fill while loading
    char input[INPUT_MAX_LEN];
    int length;
    Uint32 last_step;
} machine_t;

void display_intro()
{
    const char *lines[] = {
        "***           Welcome to Simpletron           ***",
        "*** Please enter your program one instruction ***",
        "*** (or data word) at a time. I will type the ***",
        "*** location number and a question mark (?).  ***",
        "*** You then type the word for that location. ***",
        "*** Type the sentinel -99999 to stop entering ***",
        "*** your program.                             ***",
    };

    for (int i = 0; i < (int)(sizeof(lines) / sizeof(lines[0])); i++)
    {
        term_print(lines[i]);
    }

    term_print("");
}

void start_execution(machine_t *machine)
{
    reset_cpu();
    machine->phase = PHASE_RUNNING;
    machine->last_step = SDL_GetTicks();
}

void finish_loading(machine_t *machine)
{
    term_print("*** Program loading completed ***");
    term_print("*** Program execution begins  ***");
    term_print("");
    start_execution(machine);
}

bool parse_word(const char *text, long *value)
{
    char *end;
    *value = strtol(text, &end, 10);
    return end != text && *end == '\0';
}

void submit_loading(machine_t *machine)
{
    char echo[TERM_LINE_LEN];
    snprintf(echo, sizeof(echo), "%02d ? %s", machine->address, machine->input);
    term_print(echo);

    long value;
    if (!parse_word(machine->input, &value))
    {
        term_print("*** Invalid number ***");
    }
    else if (value == SENTINEL)
    {
        finish_loading(machine);
    }
    else if (value < -WORD_MAX || value > WORD_MAX)
    {
        term_print("*** Word must be between -9999 and +9999 ***");
    }
    else
    {
        mem[machine->address++] = (int)value;

        if (machine->address == MEMORY_SIZE)
        {
            term_print("*** Memory is full ***");
            finish_loading(machine);
        }
    }
}

void submit_input(machine_t *machine)
{
    char echo[TERM_LINE_LEN];
    snprintf(echo, sizeof(echo), "%02d > %s", cpu_regs.operand, machine->input);
    term_print(echo);

    long value;
    if (!parse_word(machine->input, &value) || value < -WORD_MAX || value > WORD_MAX)
    {
        term_print("*** Enter a number between -9999 and +9999 ***");
        return;
    }

    mem[cpu_regs.operand] = (int)value;
    machine->phase = PHASE_RUNNING;
}

void machine_submit(machine_t *machine)
{
    if (machine->length == 0)
    {
        return;
    }

    if (machine->phase == PHASE_LOADING)
    {
        submit_loading(machine);
    }
    else
    {
        submit_input(machine);
    }

    machine->length = 0;
    machine->input[0] = '\0';
}

void machine_handle_event(machine_t *machine, const SDL_Event *event)
{
    if (machine->phase != PHASE_LOADING && machine->phase != PHASE_WAITING_INPUT)
    {
        return;
    }

    if (event->type == SDL_TEXTINPUT)
    {
        for (const char *c = event->text.text; *c != '\0'; c++)
        {
            bool allowed = (*c >= '0' && *c <= '9') || *c == '-' || *c == '+';
            if (allowed && machine->length < INPUT_MAX_LEN - 1)
            {
                machine->input[machine->length++] = *c;
                machine->input[machine->length] = '\0';
            }
        }
    }
    else if (event->type == SDL_KEYDOWN)
    {
        SDL_Scancode key = event->key.keysym.scancode;

        if (key == SDL_SCANCODE_BACKSPACE && machine->length > 0)
        {
            machine->input[--machine->length] = '\0';
        }
        else if (key == SDL_SCANCODE_RETURN || key == SDL_SCANCODE_KP_ENTER)
        {
            machine_submit(machine);
        }
    }
}

// Executes one instruction every STEP_DELAY_MS so the registers can be followed on screen
void machine_update(machine_t *machine)
{
    if (machine->phase != PHASE_RUNNING || SDL_GetTicks() - machine->last_step < STEP_DELAY_MS)
    {
        return;
    }

    machine->last_step = SDL_GetTicks();

    char line[TERM_LINE_LEN];
    switch (cpu_step())
    {
    case CPU_NEED_INPUT:
        machine->phase = PHASE_WAITING_INPUT;
        break;
    case CPU_OUTPUT:
        snprintf(line, sizeof(line), "Output: %d", mem[cpu_regs.operand]);
        term_print(line);
        break;
    case CPU_HALTED:
        term_print("");
        term_print("*** Simpletron execution terminated ***");
        machine->phase = PHASE_FINISHED;
        break;
    case CPU_ERROR:
        snprintf(line, sizeof(line), "*** %s ***", cpu_error());
        term_print("");
        term_print(line);
        term_print("*** Simpletron execution abnormally terminated ***");
        machine->phase = PHASE_FINISHED;
        break;
    default:
        break;
    }
}

// Returns the prompt line for the current phase, or NULL when no input is expected
const char *machine_prompt(const machine_t *machine, char *buffer, size_t size)
{
    bool cursor_on = (SDL_GetTicks() / 500) % 2 == 0;

    if (machine->phase == PHASE_LOADING)
    {
        snprintf(buffer, size, "%02d ? %s%s", machine->address, machine->input, cursor_on ? "_" : "");
        return buffer;
    }

    if (machine->phase == PHASE_WAITING_INPUT)
    {
        snprintf(buffer, size, "%02d > %s%s", cpu_regs.operand, machine->input, cursor_on ? "_" : "");
        return buffer;
    }

    return NULL;
}

void draw_panels(SDL_Renderer *renderer, TTF_Font *font)
{
    int x = TERMINAL_PANE_WIDTH;
    int line_height = TTF_FontLineSkip(font);
    int y = TERMINAL_MARGIN;
    char line[TERM_LINE_LEN];

    SDL_SetRenderDrawColor(renderer, 0, 90, 25, 255);
    SDL_RenderDrawLine(renderer, x - TERMINAL_MARGIN, TERMINAL_MARGIN, x - TERMINAL_MARGIN,
                       WINDOW_HEIGHT * WINDOW_MULTIPLIER - TERMINAL_MARGIN);

    draw_text(renderer, font, "REGISTERS", x, y);
    y += line_height;

    snprintf(line, sizeof(line), "accumulator         %+05d", cpu_regs.accumulator);
    draw_text(renderer, font, line, x, y);
    y += line_height;
    snprintf(line, sizeof(line), "instructionCounter  %02d", cpu_regs.instructionCounter);
    draw_text(renderer, font, line, x, y);
    y += line_height;
    snprintf(line, sizeof(line), "instructionRegister %+05d", cpu_regs.instructionRegister);
    draw_text(renderer, font, line, x, y);
    y += line_height;
    snprintf(line, sizeof(line), "operationCode       %02d", cpu_regs.operationCode);
    draw_text(renderer, font, line, x, y);
    y += line_height;
    snprintf(line, sizeof(line), "operand             %02d", cpu_regs.operand);
    draw_text(renderer, font, line, x, y);
    y += 2 * line_height;

    draw_text(renderer, font, "MEMORY", x, y);
    y += line_height;

    // Column header, aligned with the 3-character row label and 6-character cells below
    size_t length = (size_t)snprintf(line, sizeof(line), "   ");
    for (int column = 0; column < 10; column++)
    {
        length += (size_t)snprintf(line + length, sizeof(line) - length, "%6d", column);
    }
    draw_text(renderer, font, line, x, y);
    y += line_height;

    for (int row = 0; row < MEMORY_SIZE / 10; row++, y += line_height)
    {
        length = (size_t)snprintf(line, sizeof(line), "%3d", row * 10);
        for (int column = 0; column < 10; column++)
        {
            length += (size_t)snprintf(line + length, sizeof(line) - length, " %+05d", mem[row * 10 + column]);
        }
        draw_text(renderer, font, line, x, y);
    }
}

int UI()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0 || TTF_Init() != 0)
    {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        EMULATOR_WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH * WINDOW_MULTIPLIER,
        WINDOW_HEIGHT * WINDOW_MULTIPLIER,
        SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED) : NULL;
    TTF_Font *font = TTF_OpenFont(TERMINAL_FONT_PATH, TERMINAL_FONT_SIZE);

    if (window == NULL || renderer == NULL || font == NULL)
    {
        fprintf(stderr, "UI setup failed: %s\n", font == NULL ? TTF_GetError() : SDL_GetError());
        if (font)
            TTF_CloseFont(font);
        if (renderer)
            SDL_DestroyRenderer(renderer);
        if (window)
            SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    machine_t machine = {0};
    bool running = true;

    display_intro();
    SDL_StartTextInput();

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else
            {
                machine_handle_event(&machine, &event);
            }
        }

        machine_update(&machine);

        char prompt[TERM_LINE_LEN];
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        term_draw(renderer, font, machine_prompt(&machine, prompt, sizeof(prompt)));
        draw_panels(renderer, font);
        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    return UI();
}
