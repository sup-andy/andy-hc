HC_CFLAGS = -I../include -I../HCAPI
HC_LINK_LIB =

PKG_CFLAGS  := $(shell pkg-config --cflags --libs)
ADD_CFLAGS := -g -Wall ${HC_LINK_LIB} $(EXTRA_LDFLAGS) -lpthread -lrt -Wl,-rpath-link=$(COMMON_PATH)/lib
CFLAGS  := -shared -fPIC $(PKG_CFLAGS) $(ADD_CFLAGS) $(CFLAGS) ${HC_CFLAGS} $(EXTRA_CFLAGS)
TARGET = libhcmsg.so 
.PHONY: all clean

SRCS = hc_msg.c hc_util.c

all: $(TARGET)
	cp ${TARGET} ${LIB_INSTALL_PATH}/
	cp ${TARGET} ${COMMON_LIB_DIR}/

$(TARGET): ${SRCS}
	$(CC) $(CFLAGS) ${SRCS} -o $@ 

clean:
	$(RM) $(TARGET)

