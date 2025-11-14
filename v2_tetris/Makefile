# Compiler & Flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude -lncurses

# Project structure
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Find all .c in src/
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Output binary name
TARGET = $(BIN_DIR)/tetris

# Default rule
all: $(TARGET)

# Build the binary
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $(TARGET) -lncurses

# Compile each .c → .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Full reset
fclean: clean

re: fclean all

.PHONY: all clean fclean re
