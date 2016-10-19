CTARGET :=-D_REENTRANT

#--------------------------------------------------------------------
# "C" compiler flags
# -------------------------------
CMODE :=-W -Wno-pointer-sign -Wall

#--------------------------------------------------------------------
# the target "C" compiler:
# ---------------------------------------

CC=$(CROSS)gcc -g $(CTARGET) $(CMODE) $(includes) $(coptions)
CPP=$(CROSS)c++ $(CTARGET) $(CMODE) $(includes) $(coptions)
ARCHIEVE=$(CROSS)ar rcs $(CMBS_LIB)
LINK=$(CROSS)gcc -g -gdwarf-2

################################################################################
# Linker
LIBS += -l$(CMBS_LIBNAME)
LIBS += -lpthread

LFLAGS=$(LINKCMD) $(LIBS) -o $@
LFLAGS+= -lrt

################################################################################
# Rules

$(OBJDIR)/%.o:	%.c
	$(CC)  $(includes) $(CFLAGS) $(coptions) -c $< -o$(OBJDIR)/$*.o

$(OBJDIR)/%.o:	%.cpp
	$(CPP) -c $< -o$(OBJDIR)/$*.o

