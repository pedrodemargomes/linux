# SPDX-License-Identifier: GPL-2.0

.PHONY: clean

TARGETS = hamt_test
OFILES = $(SHARED_OFILES)
DEPS = ../../../include/linux/hamt.h ../../../lib/hamt.c

targets: $(TARGETS)

include ../shared/shared.mk

ifeq ($(DEBUG), 1)
	CFLAGS += -g
endif

$(TARGETS):	$(OFILES)

hamt_test.o:  ../../../lib/hamt_test.c

clean:
	$(RM) $(TARGETS) *.o radix-tree.c idr.c generated/*
