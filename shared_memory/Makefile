# Makefile for Shared Memory Chat Application
# C17 standard, with warnings and optimizations enabled

CC := gcc
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -O2
LDFLAGS := -lrt -lpthread

# Directories
SRC_DIR := .
COMMON_DIR := common
SERVER_DIR := server
CLIENT_DIR := client
OUTPUT_DIR := output

# Source files
COMMON_SOURCES := $(COMMON_DIR)/shared.h $(COMMON_DIR)/utils.c
SERVER_SOURCES := $(SERVER_DIR)/server.c
CLIENT_SOURCES := $(CLIENT_DIR)/client.c

# Object files
SERVER_OBJS := $(OUTPUT_DIR)/server.o $(OUTPUT_DIR)/utils.o
CLIENT_OBJS := $(OUTPUT_DIR)/client.o $(OUTPUT_DIR)/utils.o

# Executables
SERVER_EXEC := $(OUTPUT_DIR)/server
CLIENT_EXEC := $(OUTPUT_DIR)/client

# Default target
.PHONY: all
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Server executable
$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Client executable
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Server object
$(OUTPUT_DIR)/server.o: $(SERVER_SOURCES) $(COMMON_DIR)/shared.h
	@mkdir -p $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -c $(SERVER_SOURCES) -o $@

# Client object
$(OUTPUT_DIR)/client.o: $(CLIENT_SOURCES) $(COMMON_DIR)/shared.h
	@mkdir -p $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -c $(CLIENT_SOURCES) -o $@

# Utils object
$(OUTPUT_DIR)/utils.o: $(COMMON_DIR)/utils.c $(COMMON_DIR)/shared.h
	@mkdir -p $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -c $(COMMON_DIR)/utils.c -o $@

# Build server only
.PHONY: server
server: $(SERVER_EXEC)

# Build client only
.PHONY: client
client: $(CLIENT_EXEC)

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(OUTPUT_DIR)

.PHONY: help
help:
	@echo "Shared Memory Chat Application - Makefile"
	@echo "Targets:"
	@echo "  all    - Build both server and client"
	@echo "  server - Build server only"
	@echo "  client - Build client only"
	@echo "  clean  - Remove build artifacts"
	@echo "  help   - Display this help message"
