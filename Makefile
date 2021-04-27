# Top-level Makefile for User File Attributes
#
# This file includes other makefiles and builds the project in a
# non-recursive manner.

include Makedefs

CFLAGS += -I"include" -I"/usr/include" -I/usr/include/fuse3

# these vars will be modified by the other makefiles
SOURCES :=
PROGRAMS :=

# we need recursive expansion
OBJECTS = $(subst .c,.o,$(SOURCES))
DEPS = $(subst .c,.d,$(SOURCES))


all:

# all makefiles
include src/util/module.mk
include src/tools/module.mk
include src/core/module.mk

ifneq "$(MAKECMDGOALS)" "clean"
	-include $(DEPS)
endif

################################################################################

.PHONY: all clean dist

all : $(PROGRAMS)
	@echo Project built successfully!
 
dist :
	@echo "create a tarball"
	
doc :
	@echo "generating source code documentation"
	

clean :
	@echo "Cleaning..."
	-$(RM) $(DEPS) $(OBJECTS) $(PROGRAMS)


%.o: %.c
	$(call make-depend,$<,$@,$(subst .o,.d,$@))
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	

