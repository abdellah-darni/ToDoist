CC ?= gcc
TARGET := todoist

DEBUG ?= 0
ARGS  ?=

SRC_DIR   := src
OBJ_DIR   := build
BIN_DIR   := bin
DATA_DIR  := data
LOG_DIR   := logs
INCLUDE_DIR := include

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.d,$(SRCS))

TARGET_PATH := $(BIN_DIR)/$(TARGET)

COMMON_CFLAGS := -std=c11 -Wall -Wextra -I$(INCLUDE_DIR) -MMD -MP

# Libraries
LIBS := -lsqlite3 -lncurses -lmenu -lpanel -lform

# Build-type
ifeq ($(DEBUG),1)
	CFLAGS := $(COMMON_CFLAGS) -g -O0 -DDEBUG
	BUILD_TYPE := debug
else
	CFLAGS := $(COMMON_CFLAGS) -O2 -DNDEBUG
	BUILD_TYPE := release
endif

# AddressSanitizer
ASAN_CFLAGS := $(COMMON_CFLAGS) -g -O1 -fsanitize=address -fno-omit-frame-pointer -DDEBUG -fno-optimize-sibling-calls
ASAN_LDFLAGS := -fsanitize=address

# Linker flags
LDFLAGS ?=

.PHONY: all debug release asan run-asan clean clean-all run gdb gdb-report lldb lldb-report valgrind init_db help print-srcs print-objs print-deps

# Default target
all: $(TARGET_PATH)
	@printf "Built %s version: %s\n" "$(BUILD_TYPE)" "$(TARGET_PATH)"


$(TARGET_PATH): $(OBJS) | $(BIN_DIR)
	@echo "Linking $@..."
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(DATA_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Directory creation rules
$(OBJ_DIR):
	@echo "Creating build directory..."
	@mkdir -p $(OBJ_DIR)

$(DATA_DIR):
	@echo "Creating data directory..."
	@mkdir -p $(DATA_DIR)

$(BIN_DIR):
	@echo "Creating bin directory..."
	@mkdir -p $(BIN_DIR)

$(LOG_DIR):
	echo "Creating logs directory..."
	@mkdir -p $(LOG_DIR)


debug:
	@echo "Building debug version..."
	$(MAKE) DEBUG=1 clean all

release:
	@echo "Building release version..."
	$(MAKE) DEBUG=0 clean all

run: all
	@echo "Running $(TARGET_PATH) $(ARGS)"
	@echo "----------------------------------------"
	./$(TARGET_PATH) $(ARGS)

# Debugging and Sanitizing Tools
asan: clean
	@echo "Building with AddressSanitizer..."
	$(MAKE) DEBUG=1 CFLAGS="$(ASAN_CFLAGS)" LDFLAGS="$(ASAN_LDFLAGS)" all
	@echo ""
	@echo "AddressSanitizer build complete."
	@echo "Run with 'make run-asan' to log errors to $(LOG_DIR)/asan.log"

run-asan: asan | $(LOG_DIR)
	@echo "Running with ASAN, logging to $(LOG_DIR)/asan.log..."
	@echo "----------------------------------------"
	ASAN_OPTIONS=log_path=$(LOG_DIR)/asan.log ./$(TARGET_PATH) $(ARGS)

gdb: debug
	@echo "Starting GDB debugger (interactive)..."
	gdb --args ./$(TARGET_PATH) $(ARGS)

gdb-report: debug | $(LOG_DIR)
	@echo "Generating GDB crash report to $(LOG_DIR)/gdb-report.log..."
	gdb -batch -ex 'run' -ex 'bt' --args ./$(TARGET_PATH) $(ARGS) > $(LOG_DIR)/gdb-report.log 2>&1

lldb: debug
	@echo "Starting LLDB debugger (interactive)..."
	lldb -- ./$(TARGET_PATH) $(ARGS)

lldb-report: debug | $(LOG_DIR)
	@echo "Generating LLDB crash report to $(LOG_DIR)/lldb-report.log..."
	lldb -b -o "run" -o "thread backtrace" -- ./$(TARGET_PATH) $(ARGS) > $(LOG_DIR)/lldb-report.log 2>&1

valgrind: debug | $(LOG_DIR)
	@echo "Running Valgrind, logging to $(LOG_DIR)/valgrind.log..."
	@echo "This may take a while..."
	@echo "----------------------------------------"
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --log-file=$(LOG_DIR)/valgrind.log ./$(TARGET_PATH) $(ARGS)

# Database initialization/reset
init_db: | $(DATA_DIR)
	@echo "Initializing database..."
	@if [ -f $(DATA_DIR)/tasks.db ]; then \
		echo "Removing existing database..."; \
		rm -f $(DATA_DIR)/tasks.db; \
	fi; \
	if [ -f $(DATA_DIR)/fill.sql ]; then \
		sqlite3 $(DATA_DIR)/tasks.db < $(DATA_DIR)/fill.sql; \
		echo "Database initialized successfully!"; \
	else \
		echo "Warning: $(DATA_DIR)/fill.sql not found"; \
		echo "Creating empty database..."; \
		touch $(DATA_DIR)/tasks.db; \
	fi

# Clean up
clean:
	@echo "Cleaning build artifacts and logs..."
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(LOG_DIR)
	@echo "Clean complete!"

clean-all: clean
	@echo "Removing database..."
	rm -f $(DATA_DIR)/tasks.db
	@echo "Full clean complete!"

# Print targets
print-srcs:
	@echo "SRCS = $(SRCS)"

print-objs:
	@echo "OBJS = $(OBJS)"

print-deps:
	@echo "DEPS = $(DEPS)"

# Help text
help:
	@echo "Todoist Project Makefile"
	@echo "======================="
	@echo ""
	@echo "Basic targets:"
	@echo "  all        - Build the project (default)"
	@echo "  debug      - Build debug version (with -g -O0 -DDEBUG)"
	@echo "  release    - Build release version (with -O2 -DNDEBUG)"
	@echo "  clean      - Remove build artifacts and logs"
	@echo "  clean-all  - Remove build artifacts, logs, AND database"
	@echo ""
	@echo "Execution:"
	@echo "  run        - Build and run the program"
	@echo "  run ARGS='...' - Run with arguments"
	@echo ""
	@echo "Debugging & Reporting:"
	@echo "  asan         - Build with AddressSanitizer"
	@echo "  run-asan     - Run ASAN build, logging errors to logs/asan.log"
	@echo "  gdb          - Run in GDB (interactive session)"
	@echo "  gdb-report   - Generate a GDB crash report to logs/gdb-report.log"
	@echo "  lldb         - Run in LLDB (interactive session)"
	@echo "  lldb-report  - Generate an LLDB crash report to logs/lldb-report.log"
	@echo "  valgrind     - Run Valgrind, logging memory errors to logs/valgrind.log"
	@echo ""
	@echo "Database:"
	@echo "  init_db    - Initialize/reset the database"
	@echo ""


# Include dependency files
-include $(DEPS)