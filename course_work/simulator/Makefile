CC        = gcc
CFLAGS    = -Wall -Wextra -std=c11 -g

SRC_DIR    = src
DEV_DIR    = src/devices
BUILD_DIR  = build
TARGET     = $(BUILD_DIR)/pdp11

SRCS  = $(wildcard $(SRC_DIR)/*.c)
SRCS += $(wildcard $(DEV_DIR)/*.c)

OBJS  = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))
OBJS += $(patsubst $(DEV_DIR)/%.c, $(BUILD_DIR)/%.o, $(wildcard $(DEV_DIR)/*.c))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/%.o: $(DEV_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

rebuild: clean all

.PHONY: all clean run rebuild