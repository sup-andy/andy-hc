FRAMEBASE:=$(PROJDIR)/frame

cfr_objects:=
####################################################################
# settle includes

includes += -I$(FRAMEBASE) -I$(FRAMEBASE)/linux 

ifdef CMBS_COMA
includes += -I$(LIBCOMA)/inc/ -I$(LIBOSL)/inc
endif

####################################################################
# settle objects

cfr_objects += $(OBJDIR)/cfr_uart.o
cfr_objects += $(OBJDIR)/cfr_cmbs.o
cfr_objects += $(OBJDIR)/cfr_ie.o

ifdef CMBS_COMA
cfr_objects += $(OBJDIR)/cfr_coma.o
endif

####################################################################
# settle vpath

vpath %.c 	$(FRAMEBASE)
# linux
vpath %.c   $(FRAMEBASE)/linux
