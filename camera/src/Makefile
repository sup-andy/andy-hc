HC_CFLAGS = -I../../HCAPI -I../../hc_msg -I../../database/ -I../../include
HC_LINK_LIB = -lhcapi -lhcmsg -lsqlite3 -lssl -lcapi -L../../ipkg-install/lib

LINK_LIB = ${HC_LINK_LIB} $(EXTRA_LDFLAGS) -lcurl -lxml2 -lpthread -lrt -lm -Wl,-rpath-link=$(COMMON_PATH)/lib
PKG_CFLAGS  := $(shell pkg-config --cflags --libs)
ADD_CFLAGS := -g -Wall -DWLAN_RECOVER_NETWORK
CFLAGS  := $(PKG_CFLAGS) $(ADD_CFLAGS) $(HC_CFLAGS) -Wall -I. $(IFLAGS) $(EXTRA_CFLAGS)
TARGET = camerad
.PHONY: all clean ${TARGET}


# generate OBJ list
OBJS = $(addsuffix .o, $(basename $(wildcard *.c)))

all:  ${TARGET} 
	@cp ${TARGET} ${ROOTFS_INSTALL_PATH}/
        
${TARGET}: ${OBJS} 
	@echo "linking $@ ..."
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LINK_LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) ${OBJS} 
	$(RM) ${TARGET}

