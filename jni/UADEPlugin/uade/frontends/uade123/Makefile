BINDIR = /usr/local/bin
UADE123NAME = uade123

CC = gcc
CFLAGS = -Wall -O2 -I../../include -I../common   -g 
CLIBS = -lao -lpthread -ldl    -lm

all:	uade123

include ../common/Makefile.common

MODULES = uade123.o chrarray.o playlist.o playloop.o audio.o terminal.o \
	  $(COMMONMODULES)

uade123:	$(MODULES)
	$(CC) $(CFLAGS) -o $@ $(MODULES) $(CLIBS)

clean:	
	rm -f uade123 *.o

install:	uade123
	mkdir -p "$(BINDIR)"
	rm -f "$(BINDIR)/$(UADE123NAME)"
	cp -f "$(UADE123NAME)" "$(BINDIR)/" || true
	chmod og+rx "$(BINDIR)/$(UADE123NAME)"

uninstall:	
	rm -f "$(BINDIR)/$(UADE123NAME)"

audio.o:	audio.c audio.h
	$(CC) $(CFLAGS) -c $<

chrarray.o:	chrarray.c chrarray.h
	$(CC) $(CFLAGS) -c $<

playlist.o:	playlist.c playlist.h uade123.h
	$(CC) $(CFLAGS) -c $<

playloop.o:	playloop.c playloop.h uade123.h ../common/uadecontrol.h ../common/songinfo.h $(PLAYERHEADERS)
	$(CC) $(CFLAGS) -c $<

terminal.o:	terminal.c terminal.h
	$(CC) $(CFLAGS) -c $<

uade123.o:	uade123.c uade123.h ../../include/amigafilter.h ../common/amifilemagic.h ../common/uadecontrol.h $(PLAYERHEADERS)
	$(CC) $(CFLAGS) -c $<
