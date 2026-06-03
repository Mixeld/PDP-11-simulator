CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET = asm
SRCS = src/assembler.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

.PHONY: all clean