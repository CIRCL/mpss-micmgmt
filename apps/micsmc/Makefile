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

EXTRA_CPPFLAGS = -Wall -Werror -Wextra -std=c++0x
ALL_CPPFLAGS = $(CPPFLAGS) $(CXXFLAGS) $(EXTRA_CPPFLAGS) $(MPSS_METADATA_CFLAGS)

EXTRA_LDFLAGS = -lscif -lmicmgmt -lpthread
ALL_LDFLAGS = $(LDFLAGS) $(EXTRA_LDFLAGS)

MAIN_SRCS = cmd_line_args.cpp cmd_config.cpp mic_device_features.cpp \
		mic_device_class.cpp micmgmt_exception.cpp micsmc_exception.cpp \
		host_platform.cpp linux_platform.cpp \
		micsmc_helper.cpp micsmc_cli.cpp main.cpp

INCDIR = -I.
HEADERS=
MAIN_OBJS = $(MAIN_SRCS:.cpp=.o) 
MAIN_EXEC = micsmc

all: $(MAIN_EXEC)

micsmc: $(MAIN_OBJS) $(MPSS_METADATA_C)
	$(CXX) $(ALL_CPPFLAGS) $^ $(ALL_LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(ALL_CPPFLAGS) $(INCDIR) -c $< -o $@

install: $(MAIN_EXEC) $(DESTDIR)$(bindir)
	$(INSTALL) $(MAIN_EXEC) $(DESTDIR)$(bindir)

clean:
	- $(RM) $(MAIN_OBJS) $(MAIN_EXEC)

uninstall:
	- $(RM) $(DESTDIR)$(bindir)/$(MAIN_EXEC)

.PHONY: all clean install

include $(REPOROOTDIR)/mk/destdir.mk
