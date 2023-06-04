# Compiler
CC := gcc

# Compiler flags
CFLAGS := -g -Wall

# Libraries
LIBS := -lpthread -lrt

# Source files
SRCS := bounded_buffer.c unbounded_buffer.c parse_configuration.c main.c

# Object files
OBJS := $(SRCS:.c=.o)

# Target executable
TARGET := ex3.out

# Default target
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

# Rule to build object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)
