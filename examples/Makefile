# Copyright 2012 Intel Corporation.
#
# This file is subject to the Intel Sample Source Code License. A copy
# of the Intel Sample Source Code License is included.

REPOROOTDIR ?= $(CURDIR)/..
include $(REPOROOTDIR)/mk/definitions.mk

exampledir:=$(DESTDIR)$(docdir)/examples

install: $(exampledir) $(DESTDIR)$(docdir)
	$(INSTALL_f) *.c $(exampledir)
	$(INSTALL_f) Makefile_example $(exampledir)/Makefile
	$(INSTALL_f) readme.* $(DESTDIR)$(docdir)
	$(INSTALL_f) COPYING $(DESTDIR)$(docdir)

install_pywrapper: $(exampledir)
	$(INSTALL_f) example*.py $(exampledir)
	$(INSTALL_f) python_readme.* $(DESTDIR)$(docdir)

$(exampledir):
	    $(INSTALL_d) $@

.PHONY: all install install_pywrapper

include $(REPOROOTDIR)/mk/destdir.mk
