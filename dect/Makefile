#The tcmAgent implements the TR069 protocol

############################################
#
#Define Basic Env Variables
#
###########################################

TOOLCHAIN=
PLATFORM = LINUX_32

ifeq ($(strip $(PLATFORM)),WINCE_32)
CROSS_COMPILE =
endif
ifeq ($(strip $(PLATFORM)), LINUX_32)
#CROSS_COMPILE =
endif
ifeq ($(strip $(PLATFORM)), WXWORKS_32)
CROSS_COMPILE =
endif

export CROSS_COMPILE PLATFORM

LD = $(CROSS_COMPILE)ld
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
CPP = $(CROSS_COMPILE)cpp
AR = $(CROSS_COMPILE)ar
AS = $(CROSS_COMPILE)as

############################################
#
#Define target file
#
############################################
PROG = dectd
AGENTLIB=libtcmDaemon.o
PKG_CFLAGS  := $(shell pkg-config --cflags --libs dbus-1)
DEFS = -DUSE_SSL -DTR111 $(PKG_CFLAGS)
#DEFS =  -DUSE_SSL
CFLAGS = -Wall ${EXTRA_CFLAGS}
#INCLUDES += -I$(TOOLCHAIN_PATH)/usr/include

ifeq ($(strip $(PLATFORM)), WINCE_32)
DEFS += -D__USE_W32_SOCKETS -D_WIN32_IE=0x0400
endif

export DEFS CFLAGS LD CC CXX CPP AR AS PLATFORM
 
LDLIBS = 

INCLUDES = -Iinc -I../../hc_msg -I../../HCAPI  -I./dect_daemon/include -I. -I$(TOOLCHAIN_PATH)/usr/include -I./include -I$(COMMON_PATH)/include -I./ctrl $(PKG_CFLAGS)
export INCLUDES

OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
OBJS += libtcmDaemon.o libtcmcmbs.o  libendpoint.o

ifeq (${LINK_LIB},)
LINK_LIB += -ldl -lcapi -lz -lm -lcmph -L$(COMMON_PATH)/libxml2/lib -lxml2 -lfwupgrade -L../ipkg-install/lib -L$(COMMON_PATH)/lib -L$(TOOLCHAIN_PATH)/arm-linux/lib -lm -lpthread -L$(TOOLCHAIN_PATH)/usr/lib -lssl -lcrypto -lctrl_common_lib -ldbus-1 -Wl,-rpath-link=$(COMMON_PATH)/lib -lstdc++ -lhcapi -lhcmsg -lcjson
endif


#LIBS += src/dect_daemon.o
LIBS += $(wildcard src/*.o)

DIRS += cmbs/app
#DIRS += json
DIRS += src
DIRS += endpoint

#all: cpfile subdirs ${AGENTLIB}
all: cpfile subdirs $(PROG)

cpfile:
	cp src/dect_daemon.h include/dect_daemon.h
	

$(PROG): ${OBJS}
	@echo "link $@ ..."
	$(CC) ${EXTRA_LDFLAGS} $(INCLUDES) -o $@ $(OBJS) $(LINK_LIB)
	cp ${PROG} ${ROOTFS_INSTALL_PATH}/


subdirs: $(patsubst %, _dir_%, $(DIRS))

$(patsubst %, _dir_%,$(DIRS)):
	echo $(DIRS)
	$(MAKE) -C $(patsubst _dir_%, %, $@)

clean:
	-rm -f *.o $(PROG)
	for n in $(DIRS); do $(MAKE) clean -C $$n || exit; done



 

