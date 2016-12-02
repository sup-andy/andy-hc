RM = @rm -f

LINK_LIB = -lctrl_common_lib -ldbus-1 -lcapi -lssl -lcrypto -lcjson -lz -lpthread -lplist -lusbmuxd -limobiledevice $(EXTRA_LDFLAGS) -Wl,-rpath-link=$(COMMON_PATH)/lib
###LINK_LIB = -lctrl_common_lib -ldbus-1 $(EXTRA_LDFLAGS) -Wl,-rpath-link=$(COMMON_PATH)/lib

TARGET = sp_ble_ctrl

POWER_SAVING_MGR_CFLAGS := -Wall -I. $(IFLAGS) $(EXTRA_CFLAGS)

# generate OBJ list
OBJS = $(addsuffix .o, $(basename $(wildcard *.c)))

all:  ${TARGET}
        
${TARGET}: ${OBJS}
	@echo "linking $@ ..."
	$(CC) $(POWER_SAVING_MGR_CFLAGS) -o $@ $(OBJS) $(LINK_LIB)

%.o: %.c
	$(CC) $(POWER_SAVING_MGR_CFLAGS) -c -o $@ $<

clean:
	$(RM) ${OBJS} 
	$(RM) ${TARGET} 

