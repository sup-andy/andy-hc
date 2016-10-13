HC_CFLAGS = -I../../../HCAPI -I../../../include -I../ -I../../../zwave/zwave_lib -I../../../zwave/zwave_daemon

PKG_CFLAGS  := $(shell pkg-config --cflags --libs )
ADD_CFLAGS := -g -Wall
CFLAGS  := -ldl -lpthread -lcapi $(PKG_CFLAGS) $(ADD_CFLAGS) $(HC_CFLAGS) $(EXTRA_CFLAGS)
.PHONY: all clean

all:

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

