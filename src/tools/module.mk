# Makefile for ufa/src/tools

LOCAL_DIR := src/tools

LOCAL_SRCS := ufafind.c ufaset.c ufatag.c

SOURCES += $(addprefix $(LOCAL_DIR)/, $(LOCAL_SRCS))

PROGRAMS += ufafind ufaset ufatag


################################################################################

	
ufafind : $(LOCAL_DIR)/ufafind.o \
		src/util/logging.o \
		src/util/misc.o \
		src/util/list.o
	@echo creating "$@" executable...
	$(LD) $(CFLAGS) -o"$@" $^ $(LDFLAGS)

ufaset : $(LOCAL_DIR)/ufaset.o \
	src/util/logging.o
	@echo creating "$@" executable...
	$(LD) $(CFLAGS) -o"$@" $^ $(LDFLAGS)

ufatag : $(LOCAL_DIR)/ufatag.o \
	src/util/logging.o \
	src/util/misc.o \
	src/util/list.o \
	src/core/repo_sqlite.o \
	-lsqlite3
	@echo creating "$@" executable...
	$(LD) $(CFLAGS) -o"$@" $^ $(LDFLAGS)



