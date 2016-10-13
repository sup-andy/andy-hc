
#DIRS = src
DIRS = src

.PHONY : $(DIRS)
all    : $(DIRS)


$(DIRS):
	$(MAKE) -C $@

clean: 
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done

