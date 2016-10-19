ifdef CMBSDIR 
CMBSBASE:=$(CMBSDIR)/cmbs
includes += -I$(CMBSDIR)/include 
else
CMBSBASE:=$(PROJDIR)/cmbs
endif

includes += -I$(CMBSBASE)
cmbs_objects:=
####################################################################
# settle includes

#includes += -I$(FRAMEBASE) -I$(FRAMEBASE)/vone 
####################################################################
# settle objects

cmbs_objects += $(OBJDIR)/cmbs_int.o
cmbs_objects += $(OBJDIR)/cmbs_api.o
cmbs_objects += $(OBJDIR)/cmbs_dsr.o
cmbs_objects += $(OBJDIR)/cmbs_dee.o
cmbs_objects += $(OBJDIR)/cmbs_dem.o
cmbs_objects += $(OBJDIR)/cmbs_ie.o
cmbs_objects += $(OBJDIR)/cmbs_cmd.o
cmbs_objects += $(OBJDIR)/cmbs_suota.o
cmbs_objects += $(OBJDIR)/cmbs_rtp.o
cmbs_objects += $(OBJDIR)/cmbs_util.o
cmbs_objects += $(OBJDIR)/cmbs_fifo.o
ifneq ($(DCX_CHIP),DCX78)
cmbs_objects += $(OBJDIR)/cmbs_dbg.o
endif
cmbs_objects += $(OBJDIR)/cmbs_han.o
cmbs_objects += $(OBJDIR)/cmbs_han_ie.o

####################################################################
# settle vpath

vpath %.c 	$(CMBSBASE)
