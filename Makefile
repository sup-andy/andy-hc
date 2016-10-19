
DIRS = hc_msg database HCAPI hc_dispatcher zwave lprf test dect camera em35x-ezsp alljoyn_app

.PHONY : $(DIRS)
all    : $(DIRS)


$(DIRS):
	$(MAKE) -C $@

clean: 
	-rm -f ipkg-install/bin/*
	-rm -f ipkg-install/lib/*
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done

