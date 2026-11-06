# Makefile for TECC processor test programs
# Authored by <wangxp@highgo.com>, 20260412

# Compiler and flags
CC = gcc
CFLAGS = -std=c90 -Wall -Wno-unused-function -Wno-deprecated-declarations \
         -I../../../../../src/include \
         -I../../../../../src/interfaces/libpq \
         -I../../../../../src/interfaces/libpq/tecc_processor \
         -I../../../../../src/include/common/tecc_algorithm \
         -I/usr/include/openssl \
         -D_GNU_SOURCE -g -DFRONTEND

LDFLAGS = -lssl -lcrypto

# Processor test source files
PROCESSOR_TEST_SRCS = test_raw_value.c \
                     test_column_ref_data.c \
                     test_func_hardcoded_values.c \
                     test_processor_error_handling.c

PROCESSOR_TEST_OBJS = $(PROCESSOR_TEST_SRCS:.c=.o)
PROCESSOR_TEST_TARGETS = $(PROCESSOR_TEST_SRCS:.c=)

# Algorithm module source files (for linking)
ALGO_SRC_DIR = ../../../../../src/common/tecc_algorithm
ALGO_SRCS = $(ALGO_SRC_DIR)/tecc_algorithm_api.c \
            $(ALGO_SRC_DIR)/tecc_sm4_rnd.c \
            $(ALGO_SRC_DIR)/tecc_det_sm4.c \
            $(ALGO_SRC_DIR)/tecc_sm4_ope.c \
            $(ALGO_SRC_DIR)/tecc_search_sm4.c \
            $(ALGO_SRC_DIR)/tecc_hkdf_sm3.c

ALGO_OBJS = $(ALGO_SRCS:.c=.o)

# Note: For standalone test compilation, we don't link with processor module
# Processor module should be tested through integration tests

# Default target
all: $(PROCESSOR_TEST_TARGETS)

# Processor module object files needed for linking
PROCESSOR_OBJS = raw_value.o \
                 raw_values_list.o \
                 column_ref_data.o \
                 func_hardcoded_values.o \
                 tecc_processor_internal_test.o

# Special flags for compiling processor modules in test environment
PROCESSOR_CFLAGS = $(CFLAGS) -DTECC_TEST_ENVIRONMENT -I. -include test_functions.h -Dtecc_processor_palloc=test_palloc -Dtecc_processor_palloc0=test_palloc0 -Dtecc_processor_pfree=test_pfree -Dtecc_processor_pstrdup=test_pstrdup

# Build individual test programs with proper linking
test_raw_value: test_raw_value.o $(PROCESSOR_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

test_column_ref_data: test_column_ref_data.o $(PROCESSOR_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

test_func_hardcoded_values: test_func_hardcoded_values.o $(PROCESSOR_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

test_processor_error_handling: test_processor_error_handling.o $(PROCESSOR_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile test programs
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile processor modules with test environment flags
raw_value.o: ../../tecc_processor/raw_value.c
	$(CC) $(PROCESSOR_CFLAGS) -c -o $@ $<

raw_values_list.o: ../../tecc_processor/raw_values_list.c
	$(CC) $(PROCESSOR_CFLAGS) -c -o $@ $<

column_ref_data.o: ../../tecc_processor/column_ref_data.c
	$(CC) $(PROCESSOR_CFLAGS) -c -o $@ $<

func_hardcoded_values.o: ../../tecc_processor/func_hardcoded_values.c
	$(CC) $(PROCESSOR_CFLAGS) -c -o $@ $<

tecc_processor_internal_test.o: tecc_processor_internal_test.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Run all processor tests
test: all
	@echo "Running processor tests..."
	@for test in $(PROCESSOR_TEST_TARGETS); do \
		echo "=== Running $$test ==="; \
		./$$test || exit 1; \
		echo ""; \
	done
	@echo "All processor tests passed!"

# Clean up
clean:
	rm -f $(PROCESSOR_TEST_OBJS) $(PROCESSOR_TEST_TARGETS)
	rm -f $(PROCESSOR_OBJS) $(ALGO_OBJS) tecc_processor_internal_test.o

.PHONY: all test clean