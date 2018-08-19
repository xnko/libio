# -----------------------------------------------------------------------------
# CMake project wrapper Makefile ----------------------------------------------
# -----------------------------------------------------------------------------
SHELL = /bin/bash
RM    = rm -Rf
MKDIR = mkdir -p

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

.PHONY: all clean install uninstall

all: clean
	cd build && cmake -DCMAKE_BUILD_TYPE=$(build) .. && make

clean:
	$(RM) build && $(MKDIR) build
	$(RM) bin && $(MKDIR) bin

install: all
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 bin/libio.so $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/
	mkdir -p $(DESTDIR)$(PREFIX)/include/io
	install -m 644 include/io.h $(DESTDIR)$(PREFIX)/include/io/

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/lib/libio.so
	$(RM) $(DESTDIR)$(PREFIX)/include/io