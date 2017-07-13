CC=gcc
#CFLAGS+=-I/home/impulse/usr/bitlbee-3.5.1 -I/home/impulse/usr/bitlbee-3.5.1/lib -I/home/impulse/usr/bitlbee-3.5.1/protocols -I.
CFLAGS+=-I/usr/local/include/bitlbee/
#CFLAGS+=-pthread -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
CFLAGS+=`pkg-config --cflags --libs glib-2.0`
TARGETSO=bitldroid.so
TAGFILES=/usr/include/sys/socket.h /home/impulse/usr/bitlbee-3.4/* /home/impulse/usr/bitlbee-3.4/lib/* /home/impulse/usr/bitlbee-3.4/protocols/*
$(TARGETSO): bitldroid.c
	$(CC) $(CFLAGS) -fPIC -g -shared $< -o $@
	cp $(TARGETSO) /usr/local/lib/bitlbee/
install:
	cp $(TARGETSO) /usr/local/lib/bitlbee/
tags:
	ctags $(TAGFILES) bitldroid.c
	cscope -b
.PHONY: tags
