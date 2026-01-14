#	$OpenBSD$

PROG=	macsetautoboot

MAN=    macsetautoboot.8

# Try to detect if we're on a BSD system
UNAME_S := $(shell uname -s)

# On BSD systems, use bsd.prog.mk
ifeq ($(findstring BSD,$(UNAME_S)),BSD)
.include <bsd.prog.mk>
else ifeq ($(UNAME_S),OpenBSD)
.include <bsd.prog.mk>
else
# On other systems (Linux, etc.), use a simple Makefile
CC ?= cc
CFLAGS ?= -Wall -O2

all: $(PROG)

$(PROG): macsetautoboot.c
	$(CC) $(CFLAGS) -o $(PROG) macsetautoboot.c

install: $(PROG)
	install -m 755 $(PROG) /usr/local/sbin/$(PROG)
	install -m 644 $(MAN) /usr/local/share/man/man8/$(MAN)

clean:
	rm -f $(PROG) *.o

.PHONY: all install clean
endif
