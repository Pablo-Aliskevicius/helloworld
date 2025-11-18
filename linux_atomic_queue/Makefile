# Makefile for atomic queue program
# Compiles C17 code using gcc with strict standards compliance

CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -Wpedantic -O2
LDFLAGS = -lm -pthread
TARGET = atomic_queue

# Source files
SOURCES = main.c queue.c producer.c consumerA.c consumerB.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = queue.h shared.h

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

# Compile source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Remove everything including generated files
distclean: clean
	rm -f input.txt

# Rebuild from scratch
rebuild: clean all

# Run
run: $(TARGET)
	./$(TARGET) input.txt

# Test for leak memory (requires valgrind)
leaktest: $(TARGET)
	valgrind --leak-check=full ./$(TARGET) input.txt
	

clang: $(TARGET)
    # Should load tests from .clang-tidy file
	# --dump-config .clang-tidy-generated to generate one
	clang-tidy -p . *.c 

.PHONY: all clean distclean rebuild run leaktest
