# WebSocket by CyrIng
# Copyright (C) 2015-2021 CYRIL INGENIERIE
# Licenses: GPL2

CC ?= cc
WARNING = -Wall
SRCDIR = src
OBJDIR = obj
BINDIR = bin

.PHONY: all
all: makedir WebSocket

.PHONY: makedir
makedir:
	mkdir -p $(BINDIR) $(OBJDIR)

.PHONY: removedir
removedir:
	rmdir $(BINDIR) $(OBJDIR)

.PHONY: removeobj
removeobj:
	rm -f $(OBJDIR)/WebSocket.o

.PHONY: removebin
removebin:
	rm -f $(BINDIR)/WebSocket

.PHONY: clean
clean: removebin removeobj removedir

$(OBJDIR)/WebSocket.o: $(SRCDIR)/WebSocket.c
	$(CC) $(WARNING) -c $(SRCDIR)/WebSocket.c -o $(OBJDIR)/WebSocket.o

WebSocket: $(OBJDIR)/WebSocket.o
	$(CC) $(WARNING) $(SRCDIR)/WebSocket.c -o $(BINDIR)/WebSocket -lwebsockets
