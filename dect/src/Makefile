#SRCS = dect_daemon.c
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))

OBIN = dect_daemon.o 

INCLUDES += -Iinclude/ -I../include -I../../inc -I../jason  -I../endpoint -I../cmbs/app -I../cmbs/cmbs

TARGET = ../libtcmDaemon.o

#$(OBIN): $(SRCS)
#	$(CC) -o $(OBIN) $(CFLAGS) $(INCLUDES) $(DEFS) -c $(SRCS)
all:$(OBJS) $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rv ${TARGET} $(OBJS)
%.o:%.c
	$(CC) $(INCLUDES) $(CFLAGS)  $(DEFS) -c -o $@ $<

clean:
	rm -f *.o


