# Begin - ReqID:TECC4.0
#
# File: Makefile
# Abstract:
#   Build configuration for TECC processor and parser integration test program.
#
# Authored by <wangxp@highgo.com>, 20260415.
#
# Copyright:
#   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
#
# Identification:
#   src/interfaces/libpq/tecc_test/test_integration/Makefile
#

# Compiler and flags
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -O0 -fPIC \
         -DFRONTEND -DTECC_TEST_INTEGRATION \
         -I../../../../../src/include \
         -I../../../../../src/interfaces/libpq \
         -I../../../../../src/interfaces/libpq/tecc_processor \
         -I../../../../../src/interfaces/libpq/frontend_parser \
         -I../../../../../src/interfaces/libpq/tecc_libpq_integration \
         -I../../../../../src/interfaces/libpq/tecc_cache \
         -I../../../../../src/interfaces/libpq/tecc_common \
         -I./include

# Linker flags
# For basic framework testing, don't link libpq yet
LDFLAGS = -lm

# Parser object files
PARSER_OBJS = ../../frontend_parser/parser.o \
              ../../frontend_parser/tecc_parser.o \
              ../../frontend_parser/tecc_nodes.o \
              ../../frontend_parser/tecc_error.o \
              ../../frontend_parser/tecc_location.o \
              ../../frontend_parser/tecc_perf.o \
              ../../frontend_parser/tecc_semantic.o \
              ../../frontend_parser/feparser_memutils.o \
              ../../frontend_parser/fe_adapter.o

# Source files
SRCS = test_integration_main.c \
       test_sql_statements.c \
       test_debug_logging.c

# Object files
OBJS = $(SRCS:.c=.o)

# Header files
HEADERS = include/test_integration.h

# Target executable
TARGET = test_integration

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJS) $(PARSER_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Run tests
test: all
	@echo "Running integration tests..."
	./$(TARGET) --log-level=INFO

# Run tests with debug output
test-debug: all
	@echo "Running integration tests with debug output..."
	./$(TARGET) --log-level=DEBUG --verbose

# Run tests with tracing enabled
test-trace: all
	@echo "Running integration tests with tracing..."
	./$(TARGET) --log-level=TRACE --trace --verbose

# Run specific test phase
test-phase1: all
	@echo "Running Phase 1 tests (INSERT statements)..."
	./$(TARGET) --log-level=INFO

test-phase2: all
	@echo "Running Phase 2 tests (SELECT statements)..."
	./$(TARGET) --log-level=INFO

test-phase3: all
	@echo "Running Phase 3 tests (WHERE clauses)..."
	./$(TARGET) --log-level=INFO

test-phase4: all
	@echo "Running Phase 4 tests (UPDATE statements)..."
	./$(TARGET) --log-level=INFO

test-phase5: all
	@echo "Running Phase 5 tests (Complex SQL)..."
	./$(TARGET) --log-level=INFO

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build the test program (default)"
	@echo "  clean        - Remove build artifacts"
	@echo "  test         - Build and run tests with INFO level"
	@echo "  test-debug   - Build and run tests with DEBUG level"
	@echo "  test-trace   - Build and run tests with TRACE level and function tracing"
	@echo "  test-phase1  - Run Phase 1 tests (INSERT)"
	@echo "  test-phase2  - Run Phase 2 tests (SELECT)"
	@echo "  test-phase3  - Run Phase 3 tests (WHERE)"
	@echo "  test-phase4  - Run Phase 4 tests (UPDATE)"
	@echo "  test-phase5  - Run Phase 5 tests (Complex SQL)"
	@echo "  help         - Show this help message"

# Dependencies
test_integration_main.o: test_integration_main.c include/test_integration.h
test_sql_statements.o: test_sql_statements.c include/test_integration.h
test_debug_logging.o: test_debug_logging.c include/test_integration.h

.PHONY: all clean test test-debug test-trace test-phase1 test-phase2 test-phase3 test-phase4 test-phase5 help
# End - ReqID:TECC4.0