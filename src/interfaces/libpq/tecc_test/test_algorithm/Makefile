# Makefile for TECC algorithm test programs
# Authored by <wangxp@highgo.com>, 20260413

# Compiler and flags
CC = gcc
CFLAGS = -std=c90 -Wall -Werror -Wno-unused-function -Wno-deprecated-declarations \
         -I../../../../../src/include \
         -I../../../../../src/include/common/tecc_algorithm \
         -I../../../../../src/interfaces/libpq \
         -I../../../../../src/interfaces/libpq/tecc_processor \
         -I/usr/include/openssl \
         -D_GNU_SOURCE -g

LDFLAGS = -lssl -lcrypto -lm

# Algorithm test source files
ALGORITHM_TEST_SRCS = test_algorithms.c \
                     test_algorithms_extended.c \
                     test_hkdf.c \
                     test_hmac.c \
                     test_openssl_init.c \
                     test_search_pattern.c

ALGORITHM_TEST_OBJS = $(ALGORITHM_TEST_SRCS:.c=.o)
ALGORITHM_TEST_TARGETS = test_algorithms test_algorithms_extended test_hkdf test_hmac test_openssl_init test_search_pattern

# Algorithm module source files
ALGO_SRC_DIR = ../../../../../src/common/tecc_algorithm
ALGO_SRCS = $(ALGO_SRC_DIR)/tecc_algorithm_api.c \
            $(ALGO_SRC_DIR)/tecc_sm4_rnd.c \
            $(ALGO_SRC_DIR)/tecc_det_sm4.c \
            $(ALGO_SRC_DIR)/tecc_sm4_ope.c \
            $(ALGO_SRC_DIR)/tecc_search_sm4.c \
            $(ALGO_SRC_DIR)/tecc_hkdf_sm3.c

ALGO_OBJS = $(ALGO_SRCS:.c=.o)

# Default target
all: $(ALGORITHM_TEST_TARGETS)

# Build individual test programs with proper linking
test_algorithms: test_algorithms.o $(ALGO_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

test_algorithms_extended: test_algorithms_extended.o $(ALGO_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

test_hkdf: test_hkdf.o $(ALGO_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

test_hmac: test_hmac.o $(ALGO_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

test_openssl_init: test_openssl_init.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_search_pattern: test_search_pattern.o $(ALGO_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile test programs
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile algorithm module files
$(ALGO_SRC_DIR)/%.o: $(ALGO_SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Run all algorithm tests
test: all
	@echo "Running algorithm tests..."
	@echo "=== Running test_openssl_init ==="
	./test_openssl_init || exit 1
	@echo ""
	@echo "=== Running test_hkdf ==="
	./test_hkdf || exit 1
	@echo ""
	@echo "=== Running test_hmac ==="
	./test_hmac || exit 1
	@echo ""
	@echo "=== Running test_algorithms ==="
	./test_algorithms || exit 1
	@echo ""
	@echo "=== Running test_algorithms_extended ==="
	./test_algorithms_extended || exit 1
	@echo ""
	@echo "=== Running test_search_pattern ==="
	./test_search_pattern || exit 1
	@echo ""
	@echo "All algorithm tests passed!"

# Clean up
clean:
	rm -f $(ALGORITHM_TEST_OBJS) $(ALGORITHM_TEST_TARGETS)
	rm -f $(ALGO_OBJS)

.PHONY: all test clean