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

REPOROOTDIR ?= $(CURDIR)/..
include $(REPOROOTDIR)/mk/definitions.mk

MPSS_METADATA_PREFIX = $(REPOROOTDIR)/
include mpss-metadata.mk

INCLUDES = -I. -Iinclude -Isrc

OBJS_DIR = objs
LIBS_DIR = libs
SRC_DIR  = src
DOC_DIR  = doc
INC_DIR  = include

MGMT_lib_linker = libmicmgmt.so
MGMT_SHAREDLIB = $(LIBS_DIR)/$(MGMT_lib_linker)
MGMT_lib_major = 0
MGMT_lib_minor = 0.2
MGMT_lib_abi = $(MGMT_lib_linker).$(MGMT_lib_major)
MGMT_lib_real = $(MGMT_lib_linker).$(MGMT_lib_major).$(MGMT_lib_minor)
MGMT_STATICLIB = $(LIBS_DIR)/libmicmgmt_ut.a

EXTRA_CPPFLAGS += -g -Wall -Werror -Wextra -std=c++0x -fPIC
ALL_CPPFLAGS = $(EXTRA_CPPFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(MPSS_METADATA_CFLAGS)

EXTRA_LDFLAGS = $(LIBPATH) -shared -Wl,-soname=$(MGMT_lib_abi) -lscif
EXTRA_LDFLAGS += -Wl,--version-script,micmgmt.ver
ALL_LDFLAGS = $(LDFLAGS) $(EXTRA_LDFLAGS)

MAIN_OBJS = mic_device.o \
	knc_device.o \
	miclib.o \
	host_platform.o \
	miclib_exception.o

MAIN_OBJS:=$(addprefix $(OBJS_DIR)/,$(MAIN_OBJS))
METADATA_OBJ = $(patsubst %.c,%.o,$(MPSS_METADATA_C))

all: $(MGMT_SHAREDLIB)

$(MGMT_SHAREDLIB): $(MAIN_OBJS) $(METADATA_OBJ) $(LIBS_DIR) $(OBJS_DIR)
	$(CXX) $(MAIN_OBJS) $(METADATA_OBJ) $(ALL_LDFLAGS) -o $@

$(MGMT_STATICLIB): $(MAIN_OBJS) $(METADATA_OBJ) $(LIBS_DIR) $(OBJS_DIR)
	$(AR) rcs $(MGMT_STATICLIB) $(MAIN_OBJS) $(METADATA_OBJ)

$(OBJS_DIR):
	mkdir -p $(OBJS_DIR)

$(LIBS_DIR):
	mkdir -p $(LIBS_DIR)

$(OBJS_DIR)/%.o: $(SRC_DIR)/%.cpp $(OBJS_DIR)
	$(CXX) $(ALL_CPPFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.c
	$(CXX) $(ALL_CPPFLAGS) $(INCLUDES) -c $< -o $@

doc_int:
	doxygen $(DOC_DIR)/config/int.cfg

doc_ext:
	doxygen $(DOC_DIR)/config/ext.cfg

doc_clean:
	rm $(DOC_DIR)/html -rf
	rm $(DOC_DIR)/latex -rf
	rm $(DOC_DIR)/man -rf

install: $(MGMT_SHAREDLIB) $(DESTDIR)$(includedir) $(DESTDIR)$(libdir)
	$(INSTALL_x) $(MGMT_SHAREDLIB) $(DESTDIR)$(libdir)/$(MGMT_lib_real)
	pushd $(DESTDIR)$(libdir) && ln -s $(MGMT_lib_real) $(MGMT_lib_abi) && popd
	pushd $(DESTDIR)$(libdir) && ln -s $(MGMT_lib_real) $(MGMT_lib_linker) && popd
	$(INSTALL_f) $(INC_DIR)/miclib.h $(DESTDIR)$(includedir)

install_ut: $(MGMT_STATICLIB) $(DESTDIR)$(includedir) $(DESTDIR)$(libdir)
	$(INSTALL_x) $(MGMT_STATICLIB) $(DESTDIR)$(libdir)
	$(INSTALL_f) $(INC_DIR)/miclib.h $(DESTDIR)$(includedir)
	$(INSTALL_f) $(INC_DIR)/ut_instr_event.h $(DESTDIR)$(includedir)
	$(INSTALL_f) $(SRC_DIR)/*.h $(DESTDIR)$(includedir)

debug: EXTRA_CPPFLAGS += -DDEBUG=1
debug: $(MGMT_STATICLIB)

clean: doc_clean
	- $(RM) $(OBJS_DIR) -rf
	- $(RM) $(LIBS_DIR) -rf

uninstall:
	- $(RM) $(DESTDIR)$(libdir)/$(MGMT_lib_real)
	- $(RM) $(DESTDIR)$(libdir)/$(MGMT_lib_abi)
	- $(RM) $(DESTDIR)$(libdir)/$(MGMT_lib_linker)
	- $(RM) $(DESTDIR)$(includedir)/miclib.h
	- $(RM) $(DESTDIR)$(includedir)/ut_instr_event.h

.PHONY: all clean install doc-int doc-ext doc-clean debug uninstall

include $(REPOROOTDIR)/mk/destdir.mk
