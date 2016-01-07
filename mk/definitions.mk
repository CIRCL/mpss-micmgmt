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

REPOROOTDIR:=$(realpath $(REPOROOTDIR))

prefix ?= /usr
exec_prefix ?= $(prefix)
datarootdir ?= $(prefix)/share
datadir ?= $(datarootdir)
mandir ?= $(datarootdir)/man
bindir ?= $(exec_prefix)/bin
libexecdir ?= $(exec_prefix)/libexec
libdir ?= $(exec_prefix)/lib64
sysconfdir ?= $(prefix)/etc
includedir ?= $(exec_prefix)/include
srcdir ?= $(prefix)/src
mpss_test_data = $(datadir)/mpss

ifndef $(docdir)
	docdir = $(datarootdir)/doc/micmgmt
else
	docdir = $(addsuffix /micmgmt, $(docdir))
endif

INSTALL = install
INSTALL_d = $(INSTALL) -d
INSTALL_x = $(INSTALL)
INSTALL_f = $(INSTALL) -m644
CXX ?= g++ 
CP ?= cp
CC ?= cc

DESTDIR ?= $(REPOROOTDIR)/build-tmp

DESTDIRS = $(addprefix $(DESTDIR), $(exec_prefix) $(docdir) $(bindir) \
                        $(datadir) $(mpss_test_data) $(sysconfdir) $(includedir) $(libdir) \
                        $(srcdir) $(mandir))

