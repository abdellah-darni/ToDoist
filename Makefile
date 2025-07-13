CC		:= gcc
CFLAGS	:= -Wall -Wextra -g -Iinclude

SRC_DIR	:= src
OBJ_DIR	:= build
BIN_DIR	:= bin

SRCS	:= $(wildcard $(SRC_DIR)/*.c)
OBJS	:= $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TARGET	:= $(BIN_DIR)/todoist

.PHONY: all run clean 

all: $(TARGET)

# linking
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Compiling
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

run: all
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
