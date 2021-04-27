# Makefile for ufa/src/util

LOCAL_DIR := src/util

LOCAL_SRCS := list.c logging.c misc.c

SOURCES += $(addprefix $(LOCAL_DIR)/, $(LOCAL_SRCS))
