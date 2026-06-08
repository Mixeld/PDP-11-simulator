CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET = asm
SRCS = src/assembler.c

LDA_FILES = $(wildcard *.lda)
LST_FILES = $(wildcard *.lst)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
	rm -f $(LDA_FILES)
	rm -f $(LST_FILES)
	@echo "Удалены: asm, *.lda, *.lst"

.PHONY: all clean