# Makefile for TECC key manager test programs
# Authored by <wanghaoyan@highgo.com>, 20260413

# Compiler and flags
CC = gcc
CFLAGS = -std=c99 -Wall -Wno-unused-function -Wno-deprecated-declarations \
         -I../../../../../src/include \
         -I../../../../../src/interfaces/libpq \
         -I../../../../../src/common/tecc_key \
         -I. \
         -D_GNU_SOURCE -g -DFRONTEND -DTECC_KEY_TEST -DNO_PG_CONFIG

# Key manager test source files
KEY_MANAGER_TEST_SRCS = test_key_manager_basic.c \
                        test_key_manager_integration.c \
                        test_key_manager_performance.c \
                        test_key_manager_error.c \
                        test_minimal.c \
                        test_simple_standalone.c

KEY_MANAGER_TEST_OBJS = $(KEY_MANAGER_TEST_SRCS:.c=.o)
KEY_MANAGER_TEST_TARGETS = $(KEY_MANAGER_TEST_SRCS:.c=)

# Key manager source files
KEY_MANAGER_SRC_DIR = ../../../../../src/common/tecc_key
KEY_MANAGER_SRCS = $(KEY_MANAGER_SRC_DIR)/high_level_api.c \
                   $(KEY_MANAGER_SRC_DIR)/key_manager.c \
                   $(KEY_MANAGER_SRC_DIR)/kms_adapter.c \
                   $(KEY_MANAGER_SRC_DIR)/server_interface.c \
                   $(KEY_MANAGER_SRC_DIR)/utils.c \
                   $(KEY_MANAGER_SRC_DIR)/config.c

# Adapter for PostgreSQL functions in test environment
ADAPTER_SRCS = fe_memutils_adapter.c

KEY_MANAGER_OBJS = $(KEY_MANAGER_SRCS:.c=.o) $(ADAPTER_SRCS:.c=.o)

# Default target
all: $(KEY_MANAGER_TEST_TARGETS)

# Build individual test programs
test_key_manager_basic: test_key_manager_basic.o $(KEY_MANAGER_OBJS)
	$(CC) -o $@ $^ -lpthread

test_key_manager_integration: test_key_manager_integration.o $(KEY_MANAGER_OBJS)
	$(CC) -o $@ $^ -lpthread

test_key_manager_performance: test_key_manager_performance.o $(KEY_MANAGER_OBJS)
	$(CC) -o $@ $^ -lpthread

test_key_manager_error: test_key_manager_error.o $(KEY_MANAGER_OBJS)
	$(CC) -o $@ $^ -lpthread

test_minimal: test_minimal.o
	$(CC) -o $@ $^ -lpthread

test_simple_standalone: test_simple_standalone.o
	$(CC) -o $@ $^ -lpthread

# Compile test programs
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile key manager source files
high_level_api.o: $(KEY_MANAGER_SRC_DIR)/high_level_api.c
	$(CC) $(CFLAGS) -c -o $@ $<

key_manager.o: $(KEY_MANAGER_SRC_DIR)/key_manager.c
	$(CC) $(CFLAGS) -c -o $@ $<

kms_adapter.o: $(KEY_MANAGER_SRC_DIR)/kms_adapter.c
	$(CC) $(CFLAGS) -c -o $@ $<

server_interface.o: $(KEY_MANAGER_SRC_DIR)/server_interface.c
	$(CC) $(CFLAGS) -c -o $@ $<

utils.o: $(KEY_MANAGER_SRC_DIR)/utils.c
	$(CC) $(CFLAGS) -c -o $@ $<

config.o: $(KEY_MANAGER_SRC_DIR)/config.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile adapter files
fe_memutils_adapter.o: fe_memutils_adapter.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Run all key manager tests
test: all
	@echo "Running key manager tests..."
	@echo ""
	@for test in $(KEY_MANAGER_TEST_TARGETS); do \
		echo "=== Running $$test ==="; \
		./$$test || exit 1; \
		echo ""; \
	done
	@echo "All key manager tests passed!"

# Clean up
clean:
	rm -f $(KEY_MANAGER_TEST_OBJS) $(KEY_MANAGER_TEST_TARGETS)
	rm -f $(KEY_MANAGER_OBJS)

.PHONY: all test clean