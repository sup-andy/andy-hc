HC_CFLAGS = -I../../include -I../../HCAPI -I../../hc_msg -I../../database/ -I../zwave_lib
HC_LINK_LIB = -L../../ipkg-install/lib -lhcmsg -lhcapi -lsqlite3 -lzwave

LINK_LIB = ${HC_LINK_LIB} $(EXTRA_LDFLAGS) -lpthread -lrt -lm -Wl,-rpath-link=$(COMMON_PATH)/lib
PKG_CFLAGS  := $(shell pkg-config --cflags --libs)
ADD_CFLAGS := -g -Wall
CFLAGS  := $(PKG_CFLAGS) $(ADD_CFLAGS) $(HC_CFLAGS) -Wall -I. $(IFLAGS) $(EXTRA_CFLAGS)
TARGET = zwaved
.PHONY: all clean ${TARGET}


# generate OBJ list
OBJS = $(addsuffix .o, $(basename $(wildcard *.c)))

all:  ${TARGET}
	@cp ${TARGET} ${ROOTFS_INSTALL_PATH}/
        
${TARGET}: ${OBJS}
	@echo "linking $@ ..."
	$(CC) $(LINK_LIB) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) ${OBJS} 
	$(RM) ${TARGET}

