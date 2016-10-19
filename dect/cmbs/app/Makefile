.KEEP_STATE:
.SUFFIXES:
.SUFFIXES: .c .cpp .h .hpp .asm .s .inc .o .elf .d .dep .def .lex .y .ypp

libraries :=
symfiles  :=
objects   :=
linkpath  :=

APPNAME := cmbs_fwup

HOST_OS = UNIX
RM := rm -f
SL := /

################################################################################
#
# Directory
#
# define output,intermediate, object, project and base directory
#
BASE := .
CMBSDIR := ..
PROJDIR := ..

OUTDIR := $(BASE)$(SL)out
OBJDIR := $(BASE)$(SL)obj
LIBDIR := $(PROJDIR)$(SL)lib

################################################################################
#
# DNA C-Option definition
#
#
#  -DHAN_SERVER_DEMO_CISCO
HAN_SERVER_DEMO = 1
ifdef HAN_SERVER_DEMO
coptions := -DHAN_SERVER_DEMO
NR_ULE_DEVICE := 10
else
coptions := 
NR_ULE_DEVICE := 0
endif
coptions += -DCMBS_API -DDNA_TAG -DCHECKSUM_SUPPORT -D_CONSOLE -DNR_ULE_DEVICE=$(NR_ULE_DEVICE) -DCMBS_HOST_RO_FILE_SYSTEM

################################################################################
#
# DNA include
#
#
includes := -I. -I$(PROJDIR)$(SL)include -I$(PROJDIR)$(SL)cmbs -I../../include -I../../../inc

################################################################################

objects := $(OBJDIR)/main.o
#objects := $(OBJDIR)/tcx_cmbs.o
#objects += $(OBJDIR)/tcx_keyb.o
objects += $(OBJDIR)/tcx_log.o
objects += $(OBJDIR)/tcx_eep.o
objects += $(OBJDIR)/tcx_util.o
objects += $(OBJDIR)/keyb_srv.o
objects += $(OBJDIR)/keyb_call.o
objects += $(OBJDIR)/keyb_swup.o
objects += $(OBJDIR)/keyb_facility.o
objects += $(OBJDIR)/keyb_data.o
objects += $(OBJDIR)/keyb_suota.o
objects += $(OBJDIR)/keyb_han.o
objects += $(OBJDIR)/keyb_rtp.o
objects += $(OBJDIR)/cmbs_voipline.o
objects += $(OBJDIR)/cmbs_event.o
objects += $(OBJDIR)/chl18msg.o
ifdef HAN_SERVER_DEMO
objects += $(OBJDIR)/hanServer.o
endif

objects += $(OBJDIR)/tcm_cmbs.o
objects += $(OBJDIR)/tcm_call.o
#objects += $(OBJDIR)/tcx_keyb.o

vpath %.c $(BASE)



CMBS_LIBNAME := cmbs_host_lnx
CMBS_LIB := $(LIBDIR)/lib$(CMBS_LIBNAME).a
linkpath += -L$(LIBDIR)

################################################################################
# framework
include $(PROJDIR)/frame/linux/frame.mak

################################################################################
# CMBS
include $(PROJDIR)$(SL)cmbs$(SL)cmbs.mak

################################################################################
# List Access
include $(BASE)$(SL)ListAccess$(SL)ListAccess.mak

################################################################################
# SUOTA
include $(BASE)$(SL)suota$(SL)suota.mak

################################################################################
# host cmbs application
include $(BASE)$(SL)appcmbs$(SL)happcmbs.mak

# rules
include  linux.mak

#all:  dirs $(CMBS_LIB) $(OUTDIR)/$(APPNAME)
all:  dirs  $(OUTDIR)/$(APPNAME)

dirs: $(OBJDIR) $(OUTDIR) $(LIBDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OUTDIR):
	mkdir -p $(OUTDIR)

$(LIBDIR):
	mkdir -p $(LIBDIR)

#$(CMBS_LIB):	$(cfr_objects) $(cmbs_objects)
#	$(ARCHIEVE) $(cfr_objects) $(cmbs_objects)
objects += $(cfr_objects) $(cmbs_objects) 
TARGET = ../../libtcmcmbs.o
clean:
	$(RM) $(OBJDIR)$(SL)*
#	$(RM) $(OUTDIR)$(SL)$(APPNAME)
	$(RM) $(OUTDIR)$(SL)$(TARGET)


#$(OUTDIR)/$(APPNAME): $(CMBS_LIB) $(objects) 
	#$(LINK)  $(objects) $(LFLAGS) $(linkpath)
$(OUTDIR)/$(APPNAME): $(objects) 
	$(LD) -r -o $(TARGET) $(objects)

################################################################################
#
# END
