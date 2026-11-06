# Makefile for testing SWXA HSM adapter
# Usage: make -f test_swxa_adapter.mk

CC = gcc
CFLAGS = -g -O0 -Wall -I. -I../../../../src/include -I.. -I../../../../src/include/hsm -DFRONTEND
LDFLAGS = -L/usr/local/lib -lswsds -Wl,-rpath,/usr/local/lib

SRCS = test_swxa_adapter.c fe-swxa-adapter.c
OBJS = $(SRCS:.c=.o)

all: test_swxa_adapter

test_swxa_adapter: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f test_swxa_adapter $(OBJS)

run: test_swxa_adapter
	./test_swxa_adapter

.PHONY: all clean run
