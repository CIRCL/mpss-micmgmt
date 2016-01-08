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

REPOROOTDIR ?= $(CURDIR)
include $(REPOROOTDIR)/mk/definitions.mk

all: mpssinfo mpssflash micsmc

all_oem: micconfig docs_tools_oem

install: install_mpssinfo install_mpssflash install_micsmc \
	install_mpssdebug

install_oem: install_micconfig install_doc_tools_oem

lib: docs_lib
	$(MAKE) $(MFLAGS) -C miclib all

lib_oem: docs_lib_oem
	$(MAKE) $(MFLAGS) -C miclib_oem all

docs_lib:
	$(MAKE) -C doc lib

docs_lib_oem:
	$(MAKE) -C doc_oem lib

docs_tools:
	$(MAKE) -C doc tools

docs_tools_oem:
	$(MAKE) -C doc_oem tools

mpssinfo:
	$(MAKE) $(MFLAGS) -C apps/mpssinfo all 

mpssflash:
	$(MAKE) $(MFLAGS) -C apps/mpssflash all

micsmc:
	$(MAKE) $(MFLAGS) -C apps/micsmc all

micconfig:
	$(MAKE) $(MFLAGS) -C apps/micconfig all

micconfig_ut:
	$(MAKE) $(MFLAGS) -C apps/micconfig ut

install_lib: install_examples
	$(MAKE) $(MFLAGS) -C miclib install
	$(MAKE) -C doc install_lib

install_lib_oem:
	$(MAKE) $(MFLAGS) -C miclib_oem install
	$(MAKE) -C doc_oem install_lib

install_mpssinfo: 
	$(MAKE) $(MFLAGS) -C apps/mpssinfo install

install_mpssflash: 
	$(MAKE) $(MFLAGS) -C apps/mpssflash install

install_micsmc:
	$(MAKE) $(MFLAGS) -C apps/micsmc install

install_micconfig:
	$(MAKE) $(MFLAGS) -C apps/micconfig install

install_micconfig_ut:
	$(MAKE) $(MFLAGS) -C apps/micconfig install_ut

install_examples: 
	$(MAKE) -C examples install

install_mpssdebug: 
	$(MAKE) -C apps/mpssdebug install

install_doc_tools: 
	$(MAKE) -C doc install_tools

install_doc_tools_oem:
	$(MAKE) -C doc_oem install_tools

install_ut: 
	$(MAKE) $(MFLAGS) -C miclib install_ut
	$(MAKE) $(MFLAGS) -C apps/mpssinfo install_ut
	$(MAKE) $(MFLAGS) -C apps/mpssflash install_ut

install_ut_oem: 
	$(MAKE) $(MFLAGS) -C miclib_oem install_ut

install_pywrapper: install_examples_pywrapper
	$(MAKE) -C miclib_py install

install_pywrapper_oem:
	$(MAKE) -C miclib_py install

install_examples_pywrapper:
	$(MAKE) -C examples install_pywrapper

debug:
	$(MAKE) $(MFLAGS) -C miclib debug
	$(MAKE) $(MFLAGS) -C apps/mpssinfo debug
	$(MAKE) $(MFLAGS) -C apps/mpssflash debug

debug_oem:
	$(MAKE) $(MFLAGS) -C miclib_oem debug

clean:
	$(MAKE) $(MFLAGS) -C miclib clean
	$(MAKE) $(MFLAGS) -C apps/mpssinfo clean
	$(MAKE) $(MFLAGS) -C apps/mpssflash clean
	$(MAKE) $(MFLAGS) -C apps/micsmc clean
	$(MAKE) -C doc clean
	$(MAKE) -C miclib_py clean

clean_ut: 
	$(MAKE) $(MFLAGS) -C miclib clean_ut
	$(MAKE) $(MFLAGS) -C apps/mpssinfo clean_ut
	$(MAKE) $(MFLAGS) -C apps/mpssflash clean_ut
	$(MAKE) $(MFLAGS) -C apps/micconfig clean_ut

clean_oem:
	$(MAKE) $(MFLAGS) -C miclib_oem clean
	$(MAKE) $(MFLAGS) -C apps/micconfig clean

.PHONY: all all_oem install install_oem lib lib_oem docs_lib docs_lib_oem \
	docs_tools docs_tools_oem mpssinfo mpssflash micsmc micconfig \
	install_lib install_lib_oem install_mppsinfo install_mpssflash \
	install_micsmc install_micconfig install_examples install_mpssdebug \
	install_doc_tools install_doc_tools_oem install_ut install_pywrapper \
	install_pywrapper_oem install_examples_pywrapper debug clean clean_ut \
	clean_oem

include $(REPOROOTDIR)/mk/destdir.mk
