######################################################################
#
# Copyright (C) 2005, University of Karlsruhe
#
# File path:     afterburn-wedge/Mk/config/Makefile
# Description:   Process configuration files.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# $Id: Makefile,v 1.4 2005/07/14 15:11:22 joshua Exp $
#
######################################################################

CMLPATH=	$(SRCDIR)/contrib/cml2
CMLCOMPILE=	$(CMLPATH)/cmlcompile.py
CMLCONFIG=	$(CMLPATH)/cmlconfigure.py
CMLCONFIGTRANS=	$(CMLPATH)/configtrans.py

CONFIG_DIR=	config

CML_INPUT=	$(addprefix $(SRCDIR)/Mk/config/, \
		  rules.cml 	\
		  l4ka.cml	\
		  kaxen.cml	\
		 )

CMLBATCH_PARAMS ?=

$(CONFIG_DIR)/rules.out: $(CML_INPUT)
	@$(CMLCOMPILE) -o $@ $<

##  We not only need to force the configuration system to run every time,
##  but we must give parameters based on the build target even though the
##  same file is generated from every rule.  The trick is to use phony
##  targets, and to ensure that no rule mentions the output file, which
##  is $(CONFIG_DIR)/config.out.
.PHONY: cml-config
cml-config: $(CONFIG_DIR)/rules.out
	@$(CMLCONFIG) $(type) -o $(CONFIG_DIR)/config.out \
	   -i $(CONFIG_DIR)/config.out \
	   $(addprefix -D,$(CMLBATCH_PARAMS)) $(CONFIG_DIR)/rules.out

$(CONFIG_DIR)/config.h $(BUILDDIR)/Makeconf.local: $(CONFIG_DIR)/config.out
	@$(CMLCONFIGTRANS) -h $(CONFIG_DIR)/config.h -s $(CONFIG_DIR)/.config $<
	@echo "Rewriting Makeconf.local ..."
	@awk -F'[_=]' '/^CONFIG_ARCH_[^_]*=y/ { ARCH=$$3 } /^CONFIG_WEDGE_[^_]*=y/ { WEDGE=$$3 } END{ { printf(",s/^ARCH *=.*/ARCH=%s/\n,s/^WEDGE *=.*/WEDGE=%s/\nwq\n", tolower(ARCH), tolower(WEDGE)) }}' $(CONFIG_DIR)/.config | ed -s $(BUILDDIR)/Makeconf.local

.PHONY: batchconfig ttyconfig menuconfig xconfig
menuconfig : type=-c
batchconfig: type=-b
ttyconfig  : type=-t
xconfig    : type=-x

menuconfig : cml-config $(CONFIG_DIR)/config.h $(BUILDDIR)/Makeconf.local
batchconfig: cml-config $(CONFIG_DIR)/config.h $(BUILDDIR)/Makeconf.local
ttyconfig  : cml-config $(CONFIG_DIR)/config.h $(BUILDDIR)/Makeconf.local
xconfig    : cml-config $(CONFIG_DIR)/config.h $(BUILDDIR)/Makeconf.local

