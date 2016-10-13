
DIRS = zwave_lib zwave_daemon zwave_lib/zwave_tool
#DIRS = zwave_daemon

.PHONY : $(DIRS)
all    : $(DIRS)


$(DIRS):
	$(MAKE) -C $@

clean: 
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done

