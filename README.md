BitlDroid-plugin - BitlBee plugin for SMS messaging on Android phones
=========

- Use in conjunction with BitlDroid-app
- Android phone and BitlBee server must be on the same network
- *NO ENCRYPTION/AUTHENTICATION* - All messages are sent in the clear and anyone on the network can connect to your phone and send SMS messages

Building
--------

Building the plugin requires BitlBee and its headers to be installed.

By default, the Makefile is setup for a local install (/usr/local). If you installed BitlBee from source, you can simply do:

```
make
make install
```

If you installed BitlBee from a repo, you will likely need to set the PLUGINDIR environment variable to match where BitlBee's plugins go (usually /usr/lib/bitlbee/)

```
make
PLUGINDIR=/usr/lib/bitlbee make install
```
