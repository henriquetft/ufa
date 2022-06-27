# Makefile for ufa/src/tools

LOCAL_DIR := src/tools

LOCAL_SRCS := ufafind.c ufaset.c ufatag.c

SOURCES += $(addprefix $(LOCAL_DIR)/, $(LOCAL_SRCS))

PROGRAMS += ufafind ufaset ufatag


################################################################################

	
ufafind : $(LOCAL_DIR)/ufafind.o \
	src/util/logging.o \
	src/util/misc.o \
	src/util/list.o \
	src/core/repo_sqlite.o \
	src/util/error.o \
	-lsqlite3
	@echo creating "$@" executable...
	$(LD) $(CFLAGS) -o"$@" $^ $(LDFLAGS)

ufaattr : $(LOCAL_DIR)/ufaattr.o \
	src/util/logging.o
	@echo creating "$@" executable...
	$(LD) $(CFLAGS) -o"$@" $^ $(LDFLAGS)

ufatag : $(LOCAL_DIR)/ufatag.o \
	src/util/logging.o \
	src/util/misc.o \
	src/util/list.o \
	src/core/repo_sqlite.o \
	src/util/error.o \
	-lsqlite3
	@echo creating "$@" executable...
	$(LD) $(CFLAGS) -o"$@" $^ $(LDFLAGS)



