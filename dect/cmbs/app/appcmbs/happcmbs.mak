ifdef CMBSDIR
HAPPBASE:=$(CMBSDIR)/app/appcmbs
else
HAPPBASE:=$(BASE)/appcmbs
endif
####################################################################
# settle includes
includes += -I$(HAPPBASE) 
#linkpath += $(linkxml)

####################################################################
# settle objects

objects += $(OBJDIR)/appcmbs.o
objects += $(OBJDIR)/appsrv.o
objects += $(OBJDIR)/appcall.o
objects += $(OBJDIR)/appswup.o
objects += $(OBJDIR)/appfacility.o
objects += $(OBJDIR)/appdata.o
objects += $(OBJDIR)/applistacc.o
objects += $(OBJDIR)/appla2.o
objects += $(OBJDIR)/appsuota.o
objects += $(OBJDIR)/appmsgparser.o
objects += $(OBJDIR)/apprtp.o
objects += $(OBJDIR)/applog.o
objects += $(OBJDIR)/apphan.o
objects += $(OBJDIR)/apphandemo.o
####################################################################
# settle vpath

vpath %.c 	$(HAPPBASE)
