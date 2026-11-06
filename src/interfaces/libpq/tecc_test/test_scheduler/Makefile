# Begin - ReqID:TECC4.0
#
# Makefile for SEARCH algorithm operator type extension tests
#

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c90 -g

# Source files
SRCS = test_simple_integration.c
OBJS = $(SRCS:.c=.o)
TARGET = test_simple_integration

# Build rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Test run
test: $(TARGET)
	./$(TARGET)

# Clean
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all test clean

# End - ReqID:TECC4.0