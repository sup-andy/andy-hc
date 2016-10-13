HC_CFLAGS := -I../../HCAPI -I../../include -I../ 
HC_CFLAGS += -I../../zwave/zwave_lib -I../../zwave/zwave_daemon 
HC_CFLAGS += -I../../lprf 
HC_CFLAGS += -DEZSP_HOST -DPHY_NULL \
    -DPLATFORM_HEADER=\"../../em35x-ezsp/hal/micro/unix/compiler/gcc.h\" \
    -I../../em35x-ezsp/app/builder/zigbeed \
    -I../../em35x-ezsp/app/em35x-testCli \
    -I../../em35x-ezsp \
    -I../../em35x-ezsp/stack \
    -I../../em35x-ezsp/app/framework/include \

PKG_CFLAGS  := $(shell pkg-config --cflags --libs )
ADD_CFLAGS := -g -Wall
CFLAGS  := $(PKG_CFLAGS) $(ADD_CFLAGS) $(HC_CFLAGS) $(EXTRA_CFLAGS)
LDFLAGS  := -shared -fPIC -ldl -lpthread $(PKG_CFLAGS) $(ADD_CFLAGS) $(HC_CFLAGS) $(EXTRA_CFLAGS)
TARGET = libsqlite3.so 

SRCS = sqlite3.c sql_api.c
SRCS += $(wildcard zigbee/*.c)
SRCS += $(wildcard zwave/*.c)
SRCS += $(wildcard dect/*.c)
SRCS += $(wildcard ipcam/*.c)
SRCS += $(wildcard lprf/*.c)

SQL_OBJS = $(patsubst %.c,%.o,$(SRCS)) 

.PHONY: all clean

all: ${TARGET}
	@cp ${TARGET} ${LIB_INSTALL_PATH}/
	@cp ${TARGET} ${COMMON_LIB_DIR}/

${TARGET}: ${SQL_OBJS}
	$(CC) $(LDFLAGS) $(HC_LINK_LIB) $(SQL_OBJS) -o $@ 

%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) ${TARGET}
	$(RM) ${SQL_OBJS}

