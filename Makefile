CC ?= cc
AR ?= ar
PREFIX ?= /usr/local
CFLAGS ?= -O2 -std=c99 -Wall -Wextra -pedantic
CPPFLAGS += -Iinclude
LDFLAGS ?=
HTSLIB_CFLAGS ?= $(shell pkg-config --cflags htslib 2>/dev/null)
HTSLIB_LIBS ?= $(shell pkg-config --libs htslib 2>/dev/null || printf '%s' '-lhts')

BUILD := build
.PHONY: all clean lib examples bcat biocat test c-test install

all: lib

$(BUILD):
	mkdir -p $(BUILD)

lib: $(BUILD)/libbiosyntax.so $(BUILD)/libbiosyntax.a

examples: bcat

bcat: $(BUILD)/bcat

$(BUILD)/bcat: examples/bcat.c src/biosyntax.c include/biosyntax.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/bcat.c src/biosyntax.c -o $@

biocat: $(BUILD)/biocat

$(BUILD)/biocat: examples/biocat.c src/biosyntax.c include/biosyntax.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(HTSLIB_CFLAGS) $(CFLAGS) examples/biocat.c src/biosyntax.c $(HTSLIB_LIBS) -o $@

$(BUILD)/test_biosyntax: tests/test_biosyntax.c src/biosyntax.c include/biosyntax.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_biosyntax.c src/biosyntax.c -o $@

$(BUILD)/libbiosyntax.so: src/biosyntax.c include/biosyntax.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared src/biosyntax.c -o $@ $(LDFLAGS)

$(BUILD)/libbiosyntax.a: $(BUILD)/biosyntax.o
	$(AR) rcs $@ $<

$(BUILD)/biosyntax.o: src/biosyntax.c include/biosyntax.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/biosyntax.c -o $@

c-test: $(BUILD)/test_biosyntax
	$(BUILD)/test_biosyntax

test: c-test

install: lib
	install -d $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/lib
	install -m 0644 include/biosyntax.h $(DESTDIR)$(PREFIX)/include/biosyntax.h
	install -m 0755 $(BUILD)/libbiosyntax.so $(DESTDIR)$(PREFIX)/lib/libbiosyntax.so
	install -m 0644 $(BUILD)/libbiosyntax.a $(DESTDIR)$(PREFIX)/lib/libbiosyntax.a

clean:
	rm -rf $(BUILD)
