HC_CFLAGS = -I../include -I../HCAPI -I../hc_msg 
HC_LINK_LIB = -lhcmsg -lhcapi -L../ipkg-install/lib

LINK_LIB = ${HC_LINK_LIB} $(EXTRA_LDFLAGS) -lpthread -lrt -Wl,-rpath-link=$(COMMON_PATH)/lib
PKG_CFLAGS  := $(shell pkg-config --cflags --libs)
ADD_CFLAGS := -g -Wall
CFLAGS  := $(PKG_CFLAGS) $(ADD_CFLAGS) $(HC_CFLAGS) -Wall -I. $(IFLAGS) $(EXTRA_CFLAGS)
TARGET = dispatcher 
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

