CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g
LDLIBS ?= -lncurses

TARGET := simpletron
SOURCE := src/main.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
