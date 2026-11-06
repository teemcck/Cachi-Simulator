CC ?= cc
CPPFLAGS ?= -Isrc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET := cachi
DEBUG_TARGET := cachi-debug
TEST_TARGET := cachi-test
SOURCES := src/main.c src/cache.c src/replacement.c src/hierarchy.c src/parse_config.c src/trace.c
LIB_SOURCES := src/cache.c src/replacement.c src/hierarchy.c src/parse_config.c src/trace.c
HEADERS := src/cache.h src/config.h src/hierarchy.h src/parse_config.h src/replacement.h src/trace.h

.PHONY: all debug tests test run clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(SOURCES) $(LDLIBS)

$(DEBUG_TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -O0 -g3 $(LDFLAGS) -o $@ $(SOURCES) $(LDLIBS)

debug: $(DEBUG_TARGET)

$(TEST_TARGET): tests/test_cache.c $(LIB_SOURCES) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_cache.c $(LIB_SOURCES) $(LDLIBS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

tests: test

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET) $(DEBUG_TARGET) $(TEST_TARGET)
