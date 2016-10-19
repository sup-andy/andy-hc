HC_CFLAGS := -I../include -I../hc_msg -I../database/db_lib
HC_CFLAGS += -I../zwave/zwave_lib -I../zwave/zwave_daemon 
HC_CFLAGS += -I../lprf 
HC_CFLAGS += -DEZSP_HOST -DPHY_NULL \
    -DPLATFORM_HEADER=\"../em35x-ezsp/hal/micro/unix/compiler/gcc.h\" \
    -I../em35x-ezsp/app/builder/zigbeed \
    -I../em35x-ezsp/app/em35x-testCli \
    -I../em35x-ezsp \
    -I../em35x-ezsp/stack \
    -I../em35x-ezsp/app/framework/include \
    
HC_LINK_LIB = -lhcmsg -lsqlite3 -L../ipkg-install/lib/ 

LINK_LIB = ${HC_LINK_LIB} $(EXTRA_LDFLAGS) -lpthread -lrt -Wl,-rpath-link=$(COMMON_PATH)/lib
PKG_CFLAGS  := $(shell pkg-config --cflags --libs )
ADD_CFLAGS := -g -Wall
CFLAGS  := $(PKG_CFLAGS) $(ADD_CFLAGS) $(HC_CFLAGS) -Wall -I. $(IFLAGS) $(EXTRA_CFLAGS)
LDFLAGS  := -shared -fPIC $(PKG_CFLAGS) $(ADD_CFLAGS) $(HC_CFLAGS) $(EXTRA_CFLAGS)
TARGET = libhcapi.so 
.PHONY: all clean

SRCS := hcapi_db.c hcapi.c
SRCS += $(wildcard zigbee/*.c)
SRCS += $(wildcard zwave/*.c)
SRCS += $(wildcard dect/*.c)
SRCS += $(wildcard ipcam/*.c)
SRCS += $(wildcard lprf/*.c)

HC_OBJS = $(patsubst %.c,%.o,$(SRCS)) 

all: ${TARGET}
	@cp ${TARGET} ${LIB_INSTALL_PATH}/
	@cp ${TARGET} ${COMMON_LIB_DIR}/

${TARGET}: ${HC_OBJS}
	$(CC) $(CFLAGS) $(LDFLAGS) $(HC_OBJS) $(LINK_LIB) -o $@ 

%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) ${HC_OBJS}
	$(RM) ${TARGET}
