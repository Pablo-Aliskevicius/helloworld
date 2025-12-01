# Makefile for atomic queue program
# Compiles C17 code using gcc with strict standards compliance
#
# Binaries are placed in:
#   release/atomic_queue
#   debug/atomic_queue_dbg
# Object files are placed under:
#   release/int/*.o
#   debug/int/*.o

CC := gcc
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -O2
CFLAGS_DEBUG := $(CFLAGS) -g -DDEBUG -O0
LDFLAGS := -lm -pthread

BUILD_DIR_RELEASE := release
BUILD_DIR_DEBUG := debug
OBJ_DIR_RELEASE := $(BUILD_DIR_RELEASE)/int
OBJ_DIR_DEBUG := $(BUILD_DIR_DEBUG)/int

TARGET := atomic_queue
TARGET_DBG := atomic_queue_dbg

# Source files
SOURCES := main.c queue.c producer.c consumerA.c consumerB.c
HEADERS := queue.h shared.h

# Object files for each build type
OBJ_RELEASE = $(addprefix $(OBJ_DIR_RELEASE)/, $(SOURCES:.c=.o))
OBJ_DEBUG = $(addprefix $(OBJ_DIR_DEBUG)/, $(SOURCES:.c=.o))

# Recompile all targets if the compiler changes
.EXTRA_PREREQS = $(CC)

# Default target -> release build
all: release

# Build targets
release: $(BUILD_DIR_RELEASE)/$(TARGET)

debug: $(BUILD_DIR_DEBUG)/$(TARGET_DBG)

# Link release executable
$(BUILD_DIR_RELEASE)/$(TARGET): $(OBJ_RELEASE)
	@mkdir -p $(BUILD_DIR_RELEASE)
	$(CC) $(CFLAGS) -o $@ $(OBJ_RELEASE) $(LDFLAGS)

# Link debug executable
$(BUILD_DIR_DEBUG)/$(TARGET_DBG): $(OBJ_DEBUG)
	@mkdir -p $(BUILD_DIR_DEBUG)
	$(CC) $(CFLAGS_DEBUG) -o $@ $(OBJ_DEBUG) $(LDFLAGS)

# Compile .c -> release object
$(OBJ_DIR_RELEASE)/%.o: %.c $(HEADERS)
	@mkdir -p $(OBJ_DIR_RELEASE)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile .c -> debug object
$(OBJ_DIR_DEBUG)/%.o: %.c $(HEADERS)
	@mkdir -p $(OBJ_DIR_DEBUG)
	$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR_RELEASE) $(OBJ_DIR_DEBUG) $(BUILD_DIR_RELEASE)/$(TARGET) $(BUILD_DIR_DEBUG)/$(TARGET_DBG)

# Remove everything including generated files
distclean: clean
	rm -f input.txt

# Rebuild from scratch
rebuild: clean all

# Run (release)
test:run 
run: release
	./$(BUILD_DIR_RELEASE)/$(TARGET) input.txt

# Run debug executable
run_dbg: debug
	./$(BUILD_DIR_DEBUG)/$(TARGET_DBG) input.txt

# Test for leak memory (requires valgrind)
leaktest: release
	valgrind --leak-check=full ./$(BUILD_DIR_RELEASE)/$(TARGET) input.txt

# Static analysis
clang: release
	clang-tidy -p . *.c 

# Targets that are not files
.PHONY: all release debug clean distclean rebuild run run_dbg leaktest clang

# In order to generate a Makefile dependency file
#  cc -MM *.c > Makefile.deps
-include Makefile.deps