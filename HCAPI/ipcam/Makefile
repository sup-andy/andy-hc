HC_CFLAGS := -I../../include -I../../hc_msg -I../../database/db_lib 
HC_LINK_LIB = -lhcmsg -lsqlite3 -L../../ipkg-install/lib/ 

LINK_LIB = ${HC_LINK_LIB} $(EXTRA_LDFLAGS) -lpthread -lrt -Wl,-rpath-link=$(COMMON_PATH)/lib
PKG_CFLAGS  := $(shell pkg-config --cflags --libs )
ADD_CFLAGS := -g -Wall
CFLAGS  := -ldl -lpthread -lcapi $(PKG_CFLAGS) $(ADD_CFLAGS) $(HC_CFLAGS) $(EXTRA_CFLAGS)
.PHONY: all clean

all:

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
