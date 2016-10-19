ifdef CMBSDIR
SUOTABASE:=$(CMBSDIR)/app/suota
else
SUOTABASE:=$(BASE)/suota
endif
####################################################################
# settle includes
includes += -I$(SUOTABASE) 
#linkpath += $(linkxml)

####################################################################
# settle objects

objects += $(OBJDIR)/suota.o
objects += $(OBJDIR)/gmep-us.o
####################################################################
# settle vpath

vpath %.c 	$(SUOTABASE)

