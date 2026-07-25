CC        = gcc
CFLAGS    = -Wall -Wextra -std=c11 -g `pkg-config --cflags gtk+-3.0`
LDFLAGS   = `pkg-config --libs gtk+-3.0` -lpthread

SRC_DIR   = src
DEV_DIR   = src/devices
GUI_DIR   = gui
BUILD_DIR = build
TARGET    = $(BUILD_DIR)/pdp11_gui

SRC_FILES = $(filter-out $(SRC_DIR)/main.c, $(wildcard $(SRC_DIR)/*.c))
DEV_FILES = $(wildcard $(DEV_DIR)/*.c)
GUI_FILES = $(wildcard $(GUI_DIR)/*.c)

SRC_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/src_%.o, $(SRC_FILES))
DEV_OBJS = $(patsubst $(DEV_DIR)/%.c, $(BUILD_DIR)/dev_%.o, $(DEV_FILES))
GUI_OBJS = $(patsubst $(GUI_DIR)/%.c, $(BUILD_DIR)/gui_%.o, $(GUI_FILES))

OBJS = $(SRC_OBJS) $(DEV_OBJS) $(GUI_OBJS)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/src_%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(GUI_DIR) -c $< -o $@

$(BUILD_DIR)/dev_%.o: $(DEV_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(GUI_DIR) -c $< -o $@

$(BUILD_DIR)/gui_%.o: $(GUI_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(GUI_DIR) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

rebuild: clean all

.PHONY: all clean run rebuild