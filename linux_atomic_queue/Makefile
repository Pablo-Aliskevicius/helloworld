# Makefile for atomic queue program
# Compiles C17 code using gcc with strict standards compliance

CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -Wpedantic -O2
debug: CFLAGS += -g -DDEBUG -O0
LDFLAGS = -lm -pthread
TARGET = atomic_queue

# Source files
SOURCES = main.c queue.c producer.c consumerA.c consumerB.c
OBJECTS = $(SOURCES:.c=.o)
DEBUG_OBJECTS = $(SOURCES:%.c=%_dbg.o)
HEADERS = queue.h shared.h

# Default target
all: $(TARGET)

debug: $(TARGET)_dbg

# Build executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

$(TARGET)_dbg: $(DEBUG_OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET)_dbg $(DEBUG_OBJECTS) $(LDFLAGS)

# Compile source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

%_dbg.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET) $(DEBUG_OBJECTS) $(TARGET)_dbg

# Remove everything including generated files
distclean: clean
	rm -f input.txt

# Rebuild from scratch
rebuild: clean all

# Run (or test, in this case is the same)
test: run
run: $(TARGET)
	./$(TARGET) input.txt

run_dbg: $(TARGET)_dbg
	./$(TARGET)_dbg input.txt

# Test for leak memory (requires valgrind)
leaktest: $(TARGET)
	valgrind --leak-check=full ./$(TARGET) input.txt
	
# Should load tests from ../clang-tidy file
# `clang-tidy -p . *.c --dump-config .clang-tidy-generated` to generate one
clang: $(TARGET)
	clang-tidy -p . *.c 

# Targets that are not files
.PHONY: all clean distclean rebuild run test leaktest

# In order to generate a Makefile dependency file
#  cc -MM *.c > Makefile.deps
-include Makefile.deps