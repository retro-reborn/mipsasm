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

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR) $(BINDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR) $(BINDIR)

# Include dependency files
-include $(DEPS)
