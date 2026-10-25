CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g -O0
INCLUDES = -I ./include $(shell pkg-config --cflags sdl2)
SDL_LIBS = $(shell pkg-config --libs sdl2)

CRITERION_CFLAGS = $(shell pkg-config --cflags criterion)
CRITERION_LIBS = $(shell pkg-config --libs criterion)

TARGET := simpletron
SOURCE := src/main.c

# Tests: every tests/*.c plus all sources except main.c (which defines main)
TEST_TARGET := run_tests
TEST_SOURCES := $(wildcard tests/*.c)
LIB_SOURCES := $(filter-out src/main.c,$(wildcard src/*.c))

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDES) $(CPPFLAGS) -o $@ $< $(LDFLAGS) $(SDL_LIBS)

run: $(TARGET)
	./$(TARGET)

$(TEST_TARGET): $(TEST_SOURCES) $(LIB_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) $(CRITERION_CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(SDL_LIBS) $(CRITERION_LIBS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET)
