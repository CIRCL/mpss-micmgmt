# Copyright 2010-2013 Intel Corporation.
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, version 2.1.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# Disclaimer: The codes contained in these modules may be specific
# to the Intel Software Development Platform codenamed Knights Ferry,
# and the Intel product codenamed Knights Corner, and are not backward
# compatible with other Intel products. Additionally, Intel will NOT
# support the codes or instruction set in future products.
#
# Intel offers no warranty of any kind regarding the code. This code is
# licensed on an "AS IS" basis and Intel is not obligated to provide
# any support, assistance, installation, training, or other services
# of any kind. Intel is also not obligated to provide any updates,
# enhancements or extensions. Intel specifically disclaims any warranty
# of merchantability, non-infringement, fitness for any particular
# purpose, and any other warranty.
#
# Further, Intel disclaims all liability of any kind, including but
# not limited to liability for infringement of any proprietary rights,
# relating to the use of the code, even if Intel is notified of the
# possibility of such liability. Except as expressly stated in an Intel
# license agreement provided with this code and agreed upon with Intel,
# no license, express or implied, by estoppel or otherwise, to any
# intellectual property rights is granted herein.

REPOROOTDIR ?= $(CURDIR)/../..
include $(REPOROOTDIR)/mk/definitions.mk

MPSS_METADATA_PREFIX = $(REPOROOTDIR)/
include mpss-metadata.mk

EXTRA_CFLAGS += -Wall -Werror -Wextra
ALL_CFLAGS = $(CFLAGS) $(EXTRA_CFLAGS) $(MPSS_METADATA_CFLAGS)

EXTRA_LDFLAGS = $(LIBPATH) -lscif
ALL_LDFLAGS = $(LDFLAGS) $(EXTRA_LDFLAGS)

INCDIR = -I.
MAIN_SRC=flash1.c helper.c
HEADERS=helper.h
MAIN_OBJS=$(MAIN_SRC:.c=.o)
MAIN_EXEC=flash1 micflash mpssflash
TEST_EXEC=flash1_ut micflash_ut mpssflash_ut

all: EXTRA_LDFLAGS += -lmicmgmt
all: $(MAIN_EXEC)

flash1: mpssmain.o $(MAIN_OBJS) $(MPSS_METADATA_C)
	$(CC) $(ALL_CFLAGS) $^ $(ALL_LDFLAGS) -o $@

flash1_ut: mpssmain.o $(MAIN_OBJS) $(MPSS_METADATA_C)
	$(CXX) $(ALL_CFLAGS) $^ $(ALL_LDFLAGS) -o $@

micflash: micflash.o $(MAIN_OBJS) $(MPSS_METADATA_C)
	$(CC) $(ALL_CFLAGS) $^ $(ALL_LDFLAGS) -o $@

micflash_ut: micflash.o $(MAIN_OBJS) $(MPSS_METADATA_C)
	$(CXX) $(ALL_CFLAGS) $^ $(ALL_LDFLAGS) -o $@

mpssflash: mpssflash.o $(MAIN_OBJS) $(MPSS_METADATA_C)
	$(CC) $(ALL_CFLAGS) $^ $(ALL_LDFLAGS) -o $@

mpssflash_ut: mpssflash.o $(MAIN_OBJS) $(MPSS_METADATA_C)
	$(CXX) $(ALL_CFLAGS) $^ $(ALL_LDFLAGS) -o $@

.c.o: $(HEADERS)
	$(CC) $(ALL_CFLAGS) $(INCDIR) -c $< -o $@

install: $(MAIN_EXEC) $(DESTDIR)$(bindir)
	$(INSTALL_x) $(MAIN_EXEC) $(DESTDIR)$(bindir)

debug: INCDIR += -I$(REPOROOTDIR)/miclib/src -I$(REPOROOTDIR)/miclib/include
debug: EXTRA_LDFLAGS += $(REPOROOTDIR)/miclib/libs/libmicmgmt_ut.a
debug: EXTRA_CFLAGS += -DDEBUG=1
debug: $(TEST_EXEC)

install_ut: $(TEST_EXEC) $(DESTDIR)$(bindir)
	$(INSTALL_x) $(TEST_EXEC) $(DESTDIR)$(bindir)

clean:
	- $(RM) $(MAIN_OBJS) $(MAIN_EXEC)

uninstall:
	- $(RM) $(MAIN_OBJS) $(TEST_EXEC) $(MAIN_EXEC)
	- $(RM) $(DESTDIR)$(bindir)/flash1
	- $(RM) $(DESTDIR)$(bindir)/micflash
	- $(RM) $(DESTDIR)$(bindir)/mpssflash
	- $(RM) $(DESTDIR)$(bindir)/flash1_ut
	- $(RM) $(DESTDIR)$(bindir)/micflash_ut
	- $(RM) $(DESTDIR)$(bindir)/mpssflash_ut

.PHONY: all install install_ut clean

include $(REPOROOTDIR)/mk/destdir.mk
