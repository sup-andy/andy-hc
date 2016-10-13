HC_CFLAGS := -I../../../HCAPI -I../../../include -I../ 
HC_CFLAGS += -DEZSP_HOST -DPHY_NULL \
    -DPLATFORM_HEADER=\"../../../em35x-ezsp/hal/micro/unix/compiler/gcc.h\" \
    -I../../../em35x-ezsp/app/builder/zigbeed \
    -I../../../em35x-ezsp/app/em35x-testCli \
    -I../../../em35x-ezsp \
    -I../../../em35x-ezsp/stack \
    -I../../../em35x-ezsp/app/framework/include \
  
PKG_CFLAGS  := $(shell pkg-config --cflags --libs )
ADD_CFLAGS := -g -Wall
CFLAGS  := -ldl -lpthread -lcapi $(PKG_CFLAGS) $(ADD_CFLAGS) $(HC_CFLAGS) $(EXTRA_CFLAGS)
.PHONY: all clean

all:

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

