CC := gcc
BUILD := build
LIBDIR := $(BUILD)/lib
BINDIR := $(BUILD)/bin

.PHONY: all no-rpath with-rpath run-debug clean

all: no-rpath with-rpath

$(LIBDIR) $(BINDIR):
	mkdir -p $@

$(LIBDIR)/libplugin.so: plugin.c | $(LIBDIR)
	$(CC) -g -fPIC -shared -o $@ $<

no-rpath: $(LIBDIR)/libplugin.so | $(BINDIR)
	$(CC) -g -o $(BINDIR)/no_rpath main.c -L$(LIBDIR) -lplugin

with-rpath: $(LIBDIR)/libplugin.so | $(BINDIR)
	$(CC) -g -o $(BINDIR)/with_rpath main.c -L$(LIBDIR) -lplugin \
		-Wl,-rpath,'$$ORIGIN/../lib'

run-debug: no-rpath
	LD_DEBUG=libs ./$(BINDIR)/no_rpath

clean:
	rm -rf $(BUILD)
