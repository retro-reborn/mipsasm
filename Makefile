CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2
LDFLAGS =
SRCDIR = src
BUILDDIR = build
BINDIR = bin
TARGET = $(BINDIR)/mipsasm
TEST_DIR = tests

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))
DEPS = $(OBJECTS:.o=.d)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR) $(BINDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR) $(BINDIR)
	find . -type f -name '*.bin' -delete

TEST_FILES = $(wildcard $(TEST_DIR)/*.asm)
TEST_BINS = $(patsubst $(TEST_DIR)/%.asm, $(TEST_DIR)/%.bin, $(TEST_FILES))

test: $(TARGET) $(TEST_BINS)
	@echo "All tests completed."
	@for test in $(TEST_BINS); do \
		bin_base=$$(basename $$test); \
		test_name=$${bin_base%.bin}; \
		expected_file=$(TEST_DIR)/expected_$$test_name.bin; \
		if [ -f $$expected_file ]; then \
			echo "Validating $$test..."; \
			if ! diff -q $$test $$expected_file > /dev/null 2>&1; then \
				echo "Test $$test_name failed: Output does not match expected output"; \
				exit 1; \
			fi; \
		else \
			echo "Test $$test_name passed: No expected output file to compare against"; \
		fi; \
	done
	@echo "All tests passed!"

$(TEST_DIR)/%.bin: $(TEST_DIR)/%.asm $(TARGET)
	@echo "Assembling $<..."
	@$(TARGET) $< $@ || (echo "Failed to assemble $<" && exit 1)

.PHONY: test clean-tests

# Include dependency files
-include $(DEPS)
