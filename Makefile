CC		:= gcc
CFLAGS	:= -Wall -Wextra -g -Iinclude

SRC_DIR	:= src
OBJ_DIR	:= build
BIN_DIR	:= bin
DATA_DIR:= data

SRCS	:= $(wildcard $(SRC_DIR)/*.c)
OBJS	:= $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TARGET	:= $(BIN_DIR)/todoist

LIBS	:= -lsqlite3 -lncurses -lmenu

.PHONY: all run clean init_db

all: $(TARGET)

# linking
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Compiling
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(DATA_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(DATA_DIR):
	mkdir -p $(DATA_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

run: all
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	rm $(DATA_DIR)/tasks.db

init_db: 
	sqlite3 $(DATA_DIR)/tasks.db < $(DATA_DIR)/fill.sql