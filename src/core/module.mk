# Makefile for ufa/src/core

LOCAL_DIR := src/core

LOCAL_SRCS := ufafs.c repo_sqlite.c

SOURCES += $(addprefix $(LOCAL_DIR)/, $(LOCAL_SRCS))

PROGRAMS += ufafs

################################################################################



ufafs : src/core/ufafs.o \
        src/core/repo_sqlite.o \
		src/util/logging.o \
		src/util/misc.o \
		src/util/list.o \
		src/util/error.o \
		-lfuse3 \
		-lsqlite3 \
		-lpthread
	@echo creating "$@" executable...
	$(LD) $(CFLAGS) -o"$@" $^

