CC := gcc
BUILD := build
LIBDIR := $(BUILD)/lib
BINDIR := $(BUILD)/bin

.PHONY: all run-fixed inspect clean

all: $(BINDIR)/missing_path $(BINDIR)/fixed_path $(BINDIR)/address_case

$(LIBDIR) $(BINDIR):
	mkdir -p $@

$(LIBDIR)/libservice.so: service.c | $(LIBDIR)
	$(CC) -g -fPIC -shared -o $@ $<

$(BINDIR)/missing_path: main.c $(LIBDIR)/libservice.so | $(BINDIR)
	$(CC) -g -o $@ main.c -L$(LIBDIR) -lservice

$(BINDIR)/fixed_path: main.c $(LIBDIR)/libservice.so | $(BINDIR)
	$(CC) -g -o $@ main.c -L$(LIBDIR) -lservice -Wl,-rpath,'$$ORIGIN/../lib'

$(BINDIR)/address_case: address.c | $(BINDIR)
	$(CC) -g -O0 -no-pie -o $@ $<

run-fixed: $(BINDIR)/fixed_path
	./$(BINDIR)/fixed_path

inspect: all
	readelf -d $(BINDIR)/missing_path | grep -E 'NEEDED|RPATH|RUNPATH'
	nm -n $(BINDIR)/address_case | grep suspicious_operation

clean:
	rm -rf $(BUILD)
