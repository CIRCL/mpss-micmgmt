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

COPY     = cp -f
COPY_DIR = $(COPY) -r

localhtmldir = html
localmandir = man
man1dir = $(DESTDIR)$(mandir)/man1
man7dir = $(DESTDIR)$(mandir)/man7
micsmc-gui-docdir = $(REPOROOTDIR)/apps/micsmc/gui/UserGuide

FLAGS_HTML = --doctype manpage --format xhtml -v -D $(localhtmldir)
FLAGS_MAN = --doctype manpage --format manpage -v -D $(localmandir)

tools: miccheck micflash micinfo micsmc mpssinfo mpssflash

miccheck: $(localhtmldir) $(localmandir)
	a2x $(FLAGS_HTML) miccheck.1.txt
	a2x $(FLAGS_MAN) miccheck.1.txt

micflash: $(localhtmldir) $(localmandir)
	a2x $(FLAGS_HTML) micflash.1.txt
	a2x $(FLAGS_MAN) micflash.1.txt

micinfo: $(localhtmldir) $(localmandir)
	a2x $(FLAGS_HTML) micinfo.1.txt
	a2x $(FLAGS_MAN) micinfo.1.txt

micsmc: $(localhtmldir) $(localmandir)
	a2x $(FLAGS_HTML) micsmc.1.txt
	a2x $(FLAGS_MAN) micsmc.1.txt

mpssinfo: $(localhtmldir) $(localmandir)
	a2x $(FLAGS_HTML) mpssinfo.1.txt
	a2x $(FLAGS_MAN) mpssinfo.1.txt

mpssflash: $(localhtmldir) $(localmandir)
	a2x $(FLAGS_HTML) mpssflash.1.txt
	a2x $(FLAGS_MAN) mpssflash.1.txt

lib: $(localhtmldir) $(localmandir)
	a2x $(FLAGS_HTML) libmicmgmt.7.txt
	a2x $(FLAGS_MAN) libmicmgmt.7.txt

win-lib:
	a2x --doctype manpage --format chunked -v libmicmgmt.7.txt

install_tools: $(DESTDIR)$(docdir) $(man1dir)
	$(INSTALL) $(localmandir)/*.1 $(man1dir)
	$(INSTALL) $(localhtmldir)/*.html $(DESTDIR)$(docdir)
	$(COPY_DIR) $(micsmc-gui-docdir)/* $(DESTDIR)$(docdir)

install_lib: $(DESTDIR)$(docdir) $(man7dir)
	$(INSTALL) $(localmandir)/*.7 $(man7dir)
	$(INSTALL) $(localhtmldir)/*.html $(DESTDIR)$(docdir)

$(man1dir): $(DESTDIR)$(mandir)
	$(INSTALL_d) $@

$(man7dir): $(DESTDIR)$(mandir)
	$(INSTALL_d) $@

$(localhtmldir):
	$(INSTALL_d) $@

$(localmandir):
	$(INSTALL_d) $@

clean:
	rm -rf $(localhtmldir)
	rm -rf $(localmandir)
	rm -rf *.xml

#FIXME add uninstall target

.PHONY: install all tools miccheck micflash micsmc mpssinfo mpssflash \
	lib install_tools install_lib

include $(REPOROOTDIR)/mk/destdir.mk
