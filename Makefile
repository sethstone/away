# Makefile for Away

# files and paths
PREFIX = /usr/local
BINARY = away
SRC    = $(BINARY).c

# compiler
CC     = gcc
CFLAGS = -O2 -Wall -D_REENTRANT
LIBS   = -lpthread -ldl -lpam -lpam_misc

# alias away
all: $(BINARY)

# away
$(BINARY): $(SRC)
	$(CC) $(CFLAGS) $(LIBS) $(SRC) -o $(BINARY)
#	@strip $(BINARY)

# make install
install: $(BINARY)
	cp $(BINARY) $(PREFIX)/bin/
	cp doc/away.1 $(PREFIX)/man/man1/
	cp doc/awayrc.5 $(PREFIX)/man/man5/
	cp data/away.pam /etc/pam.d/away

# Clean up
clean:
	rm -f $(BINARY) *.o core
