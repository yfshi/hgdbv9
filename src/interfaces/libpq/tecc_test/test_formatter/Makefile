# Makefile for TECC data formatter simple test
# Authored by <wangxp@highgo.com>, 20260412

# Compiler and flags
CC = gcc
CFLAGS = -std=c90 -Wall -Werror -Wno-unused-function -g
LDFLAGS =

# Test source files
TEST_SRC = simple_test.c
TEST_OBJ = $(TEST_SRC:.c=.o)
TEST_TARGET = test_formatter_simple

# ICU integration test
ICU_TEST_SRC = test_icu_integration.c
ICU_TEST_OBJ = $(ICU_TEST_SRC:.c=.o)
ICU_TEST_TARGET = test_icu_integration

# Default target
all: $(TEST_TARGET) $(ICU_TEST_TARGET)

# Link test programs
$(TEST_TARGET): $(TEST_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

$(ICU_TEST_TARGET): $(ICU_TEST_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# Include directories
INCLUDES = -I../../tecc_formatter -I../../tecc_scheduler -I../../tecc_common -I../../tecc_processor/internal

# ICU flags (if available)
ICU_CFLAGS = $(shell pkg-config --cflags icu-uc icu-i18n 2>/dev/null || echo "")
ICU_LIBS = $(shell pkg-config --libs icu-uc icu-i18n 2>/dev/null || echo "")

# Compile test programs
$(TEST_OBJ): CFLAGS += $(INCLUDES)
$(ICU_TEST_OBJ): CFLAGS += $(INCLUDES) $(ICU_CFLAGS) -DUSE_ICU

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Link ICU test with ICU libraries
$(ICU_TEST_TARGET): LDFLAGS += $(ICU_LIBS)

# Clean up
clean:
	rm -f $(TEST_OBJ) $(TEST_TARGET) $(ICU_TEST_OBJ) $(ICU_TEST_TARGET)

# Run tests
test: $(TEST_TARGET) $(ICU_TEST_TARGET)
	@echo "Running simple formatter test..."
	./$(TEST_TARGET)
	@echo ""
	@echo "Running ICU integration test..."
	./$(ICU_TEST_TARGET)

.PHONY: all clean test