# Simple Makefile for testing HSM adapter
# Usage: make -f test_hsm_adapter.mk

CC = gcc
CFLAGS = -g -O0 -Wall -I. -I../../../../src/include -I.. -I../../../../src/include/hsm -DFRONTEND
LDFLAGS = -lsdf.client

# Source files
SRCS = test_hsm_adapter.c fe-hsm-adapter.c
OBJS = $(SRCS:.c=.o)

# Default target
all: test_hsm_adapter

# Test program
test_hsm_adapter: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f test_hsm_adapter $(OBJS)

# Run tests
run: test_hsm_adapter
	./test_hsm_adapter

.PHONY: all clean run