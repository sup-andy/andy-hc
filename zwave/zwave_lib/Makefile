HC_CFLAGS = -I../../include -I../../HCAPI -I../../hc_msg -I../../database/ -I../zwave_daemon
HC_LINK_LIB = 

PKG_CFLAGS  := $(shell pkg-config --cflags --libs)
ADD_CFLAGS := -g -Wall -lssl ${HC_LINK_LIB} $(EXTRA_LDFLAGS) -lpthread -lrt -Wl,-rpath-link=$(COMMON_PATH)/lib
CFLAGS  := -shared -fPIC $(PKG_CFLAGS) $(ADD_CFLAGS) ${CFLAGS} $(HC_CFLAGS) -I. $(IFLAGS) $(EXTRA_CFLAGS)
TARGET = libzwave.so 
.PHONY: all clean

SRCS = $(wildcard $(shell pwd)/*.c)

all: $(TARGET)
	@cp ${TARGET} ${LIB_INSTALL_PATH}/
	@cp ${TARGET} ${COMMON_LIB_DIR}/

$(TARGET): ${SRCS}
	$(CC) $(CFLAGS) ${SRCS} -o $@

clean:
	$(RM) $(TARGET)

