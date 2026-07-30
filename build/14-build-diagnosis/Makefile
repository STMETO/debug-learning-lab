CC      := gcc
WARN    := -Wall -Wextra -Wpedantic
BUILD   := build
LIBDIR  := $(BUILD)/lib
BINDIR  := $(BUILD)/bin

.PHONY: help undefined-reference multiple-definition missing-header \
        no-debug with-debug shared-lib shared-no-rpath shared-rpath \
        run-no-rpath run-rpath clean

help:
	@echo "Expected failures: undefined-reference multiple-definition missing-header run-no-rpath"
	@echo "Working targets: no-debug with-debug shared-lib shared-no-rpath shared-rpath run-rpath"

$(LIBDIR) $(BINDIR):
	mkdir -p $@

undefined-reference: | $(BINDIR)
	$(CC) $(WARN) -g -o $(BINDIR)/undefined cases/undefined_main.c

multiple-definition: | $(BINDIR)
	$(CC) $(WARN) -g -o $(BINDIR)/duplicate \
		cases/duplicate_main.c cases/duplicate_a.c cases/duplicate_b.c

missing-header:
	$(CC) $(WARN) -g -c cases/missing_header.c

no-debug: | $(BINDIR)
	$(CC) $(WARN) -O2 -o $(BINDIR)/no_debug cases/symbol_demo.c

with-debug: | $(BINDIR)
	$(CC) $(WARN) -g -O0 -o $(BINDIR)/with_debug cases/symbol_demo.c

shared-lib: | $(LIBDIR)
	$(CC) $(WARN) -fPIC -shared -o $(LIBDIR)/libgreeting.so cases/greeting.c

shared-no-rpath: shared-lib | $(BINDIR)
	$(CC) $(WARN) -o $(BINDIR)/no_rpath cases/greeting_main.c -L$(LIBDIR) -lgreeting

shared-rpath: shared-lib | $(BINDIR)
	$(CC) $(WARN) -o $(BINDIR)/with_rpath cases/greeting_main.c \
		-L$(LIBDIR) -lgreeting -Wl,-rpath,'$$ORIGIN/../lib'

run-no-rpath: shared-no-rpath
	./$(BINDIR)/no_rpath

run-rpath: shared-rpath
	./$(BINDIR)/with_rpath

clean:
	rm -rf $(BUILD) missing_header.o
