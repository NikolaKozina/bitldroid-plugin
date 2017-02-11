CC=gcc
CFLAGS+=-I/home/impulse/usr/bitlbee-3.5.1 -I/home/impulse/usr/bitlbee-3.5.1/lib -I/home/impulse/usr/bitlbee-3.5.1/protocols -I.
CFLAGS+=-pthread -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
CFLAGS+=`pkg-config --cflags --libs glib-2.0`
ANDROIDSMS=androidsms.so
TAGFILES=/usr/include/sys/socket.h /home/impulse/usr/bitlbee-3.4/* /home/impulse/usr/bitlbee-3.4/lib/* /home/impulse/usr/bitlbee-3.4/protocols/*
$(ANDROIDSMS): ircsmsgateway.c
	$(CC) $(CFLAGS) -fPIC -g -shared $< -o $@
	cp androidsms.so /usr/local/lib/bitlbee/
install:
	cp androidsms.so /usr/local/lib/bitlbee/
tags:
	ctags $(TAGFILES) ircsmsgateway.c
	cscope -b
.PHONY: tags
