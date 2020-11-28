.POSIX:
ALL_WARNING = -Wall -Wextra -pedantic
ALL_LDFLAGS = -lxcb -lxcb-keysyms $(LDFLAGS)
ALL_CFLAGS = $(CPPFLAGS) $(CFLAGS) -std=c99 $(ALL_WARNING)
PREFIX = /usr/local
LDLIBS = -lm
BINDIR = $(PREFIX)/bin

all: xbar
install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -f xbar $(DESTDIR)$(BINDIR)
	chmod 755 $(DESTDIR)$(BINDIR)/xbar
xbar: xbar.o
	$(CC) $(ALL_LDFLAGS) -o xbar xbar.o $(LDLIBS)
xbar.o: xbar.c config.h
	$(CC) $(ALL_CFLAGS) -c xbar.c
clean:
	rm -f xbar *.o
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/xbar
.PHONY: all install uninstall clean
