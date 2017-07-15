CC=gcc
CFLAGS+=`pkg-config --cflags --libs bitlbee`
TARGETSO=bitldroid.so
BITLBEESOURCE?=/home/impulse/usr/bitlbee-3.5.1
PLUGINDIR?=/usr/local/lib/bitlbee
TAGFILES=/usr/include/sys/socket.h $(BITLBEESOURCE)/* $(BITLBEESOURCE)/lib/* $(BITLBEESOURCE)/protocols/*
$(TARGETSO): bitldroid.c
	$(CC) $(CFLAGS) -fPIC -g -shared $< -o $@
install:
	cp $(TARGETSO) $(PLUGINDIR)/
tags:
	ctags $(TAGFILES) bitldroid.c
	cscope -b
.PHONY: tags
