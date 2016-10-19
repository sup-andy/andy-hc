#SRCS = dect_daemon.c
SRCS = $(wildcard *.c)

#OBJS =%(SRCS:%.c =%.o)
OBJS = $(patsubst %.c, %.o, $(SRCS))

TARGET = ../libendpoint.o

INCLUDES += -Iinclude/ -I../include -I../../inc -I../jason  -I../cmbs/app -I../cmbs/cmbs -I../src



$(TARGET): $(OBJS)
	#$(CC) -o $(OBIN) $(CFLAGS) $(INCLUDES) $(DEFS) -c $(SRCS)
	#$(LD) $(OBJS) -o $(TARGET) $(CFLAGS) $(INCLUDES) $(DEFS)
	$(AR) rv ${TARGET} $(OBJS)

%.o:%.c
	$(CC) $(CFLAGS)  $(INCLUDES) $(DEFS) -c -o $@ $<

clean:
	rm -f *.o


