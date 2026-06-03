
CC = gcc
CFLAGS = -Wall -Wextra -O2 -I.

TARGET = pdp11
SRCS = src/main.c src/cpu.c src/memory.c src/instructions.c \
       src/decode.c src/loader.c src/debug.c src/disasm.c
OBJS = $(SRCS:src/%.c=obj/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

obj/%.o: src/%.c | obj
	$(CC) $(CFLAGS) -c $< -o $@

obj:
	mkdir -p obj

test: $(TARGET)
	$(CC) $(CFLAGS) -o test_instructions test/test_instructions.c src/*.c
	./test_instructions

clean:
	rm -rf obj/ $(TARGET) test_instructions

.PHONY: all clean test