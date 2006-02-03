######################################################################
#                
# Copyright (C) 2005, University of Karlsruhe
#                
# File path:     patches/Makefile
# Description:   Rules to apply patches, and rules to build all 
#                prerequisites.
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
# $Id: Makefile,v 1.63 2005/12/16 09:18:32 joshua Exp $
#                
######################################################################

PARENT_DIR = $(dir $(CURDIR))

######################################################################
#
#  Command line parameters.  Nearly all paths must be absolute, since the 
#  build process changes directories, which invalidates relative
#  paths.
#
######################################################################

# Local overrides.
-include Makeconf.local

#  The directory for unpacking the source archives.  By default, put it
#  within the afterburner directory structure.
SRCDIR ?= $(PARENT_DIR)extern

#  The directory for storing the archives.
ARCHIVE_DIR ?= $(CURDIR)/archives

BUILD_DIR ?= $(PARENT_DIR)build
BOOT_DIR ?= $(PARENT_DIR)boot
INSTALL_ROOT ?= $(PARENT_DIR)usr

L4KA_CVS_ROOT ?= :pserver:guest@cvs.l4ka.org:/public-cvs
RHYPE_CVS_ROOT ?= :pserver:anonymous@kcvs.watson.ibm.com:/u/kitchawa/cvsroot

#  The directory used *within* the GRUB menu.lst for locating 
#  the boot elements.
GRUB_BOOT ?= (nd)/tftpboot/afterburn
#  The virtual machine size in kilobytes.
VMSIZE ?= 65536
#  To use VGA console output, set VGA=1 on the command line.
ifdef VGA
  LINUX_BOOT_PARAMS ?= console=ttyS0,115200 console=tty
else
  LINUX_BOOT_PARAMS ?= console=tty console=ttyS0,115200
endif

#  To see all issued commands, set DEBUG=1 on the command line,
#  or Q=
ifdef DEBUG
Q = 
endif

Q ?= @

#  To support cross-compilation, and features specific to a platform,
#  determine the host platform and architecture.
#  Valid platform values: Darwin, Linux
HOST_PLATFORM ?= $(shell uname -s)
#  Valid architecture values: i386, powerpc
UNAME_ARCH_Linux = uname -m | sed 's/i.86/i386/'
UNAME_ARCH_Darwin = uname -p | sed 's/i.86/i386/'
HOST_ARCH ?= $(shell $(UNAME_ARCH_$(HOST_PLATFORM)))
TARGET_ARCH ?= i386

TOOLPREFIX_i386 = i386-elf-
TOOLPREFIX_powerpc = powerpc-elf-

ifneq ($(HOST_ARCH),$(TARGET_ARCH))
## Cross compiling.
TOOLPREFIX ?= $(TOOLPREFIX_$(TARGET_ARCH))
endif
ifeq ($(HOST_PLATFORM),Darwin)
## We're on Darwin, and we need ELF support, so cross compile.
TOOLPREFIX ?= $(TOOLPREFIX_$(TARGET_ARCH))
endif

######################################################################
#
#  Source tree versions.  We manually maintain the versions of our 
#  source trees here.
#
######################################################################
VERSION_linux-2.6 = 2.6.9
#VERSION_linux-2.6 = 2.6.11
#VERSION_linux-2.6 = 2.6.12.3
VERSION_linux-2.6-vmi = 2.6.12.3
VERSION_linux-2.4 = 2.4.31
VERSION_linux-2.4 = 2.4.28
VERSION_binutils = 2.15.91.0.2
VERSION_xen-2 = 2.0.2
#VERSION_xen-2 = 2.0.7
VERSION_xen-3 = 3.0.0
VERSION_newlib = 1.13.0
VERSION_idl4 = 1.0.2
VERSION_qemu = 0.7.0
VERSION_gdb = 6.3
VERSION_xnu = 792.1.5
VERSION_module-init-tools = 3.2-pre7
VERSION_genromfs = 0.5.1
#VERSION_python = 2.4.2
VERSION_python = 2.3.5
VERSION_hg = 0.7

VERSION_i386_gcc = 3.4.4
VERSION_powerpc_gcc = 3.3.6
VERSION_gcc = $(VERSION_$(TARGET_ARCH)_gcc)

######################################################################
#
#  Tar ball information.
#
######################################################################

TARBALLS += linux-2.6 binutils xen-2 gcc newlib idl4 qemu qemu-floppy gdb \
  module-init-tools
CVS_LIST += pistachio marzipan

TARBALL_linux-2.6 = linux-$(VERSION_linux-2.6).tar.bz2
URL_linux-2.6 = http://www.kernel.org/pub/linux/kernel/v2.6/$(TARBALL_linux-2.6)
URL_$(TARBALL_linux-2.6) = $(URL_linux-2.6)
UNPACK_linux-2.6 = linux-$(VERSION_linux-2.6)

TARBALL_linux-2.6-vmi = linux-$(VERSION_linux-2.6-vmi).tar.bz2
URL_linux-2.6-vmi = http://www.kernel.org/pub/linux/kernel/v2.6/$(TARBALL_linux-2.6-vmi)
URL_$(TARBALL_linux-2.6-vmi) = $(URL_linux-2.6-vmi)
UNPACK_linux-2.6-vmi = linux-$(VERSION_linux-2.6-vmi)-vmi
PREUNPACK_linux-2.6-vmi = linux-$(VERSION_linux-2.6-vmi)


TARBALL_linux-2.4 = linux-$(VERSION_linux-2.4).tar.bz2
URL_linux-2.4 = http://kernel.org/pub/linux/kernel/v2.4/$(TARBALL_linux-2.4)
URL_$(TARBALL_linux-2.4) = $(URL_linux-2.4)
UNPACK_linux-2.4 = linux-$(VERSION_linux-2.4)

TARBALL_binutils = binutils-$(VERSION_binutils).tar.bz2
URL_binutils = http://www.kernel.org/pub/linux/devel/binutils/$(TARBALL_binutils)
URL_$(TARBALL_binutils) = $(URL_binutils)
UNPACK_binutils = binutils-$(VERSION_binutils)

TARBALL_xen-2 = xen-$(VERSION_xen-2)-src.tgz
URL_xen-2 = http://www.cl.cam.ac.uk/Research/SRG/netos/xen/downloads/$(TARBALL_xen-2)
URL_$(TARBALL_xen-2) = $(URL_xen-2)
PREUNPACK_xen-2 = xen-2.0
UNPACK_xen-2 = xen-$(VERSION_xen-2)

TARBALL_xen-3 = xen-$(VERSION_xen-3)-src.tgz
URL_xen-3 = http://www.cl.cam.ac.uk/Research/SRG/netos/xen/downloads/$(TARBALL_xen-3)
URL_$(TARBALL_xen-3) = $(URL_xen-3)
PREUNPACK_xen-3 = xen-unstable
UNPACK_xen-3 = xen-$(VERSION_xen-3)

TARBALL_gcc = gcc-$(VERSION_gcc).tar.bz2
UNPACK_gcc = gcc-$(VERSION_gcc)
URL_gcc = http://mirrors.usc.edu/pub/gnu/gcc/gcc-$(VERSION_gcc)/$(TARBALL_gcc)
URL_$(TARBALL_gcc) = $(URL_gcc)

TARBALL_newlib = newlib-$(VERSION_newlib).tar.gz
UNPACK_newlib = newlib-$(VERSION_newlib)
URL_newlib = ftp://sources.redhat.com/pub/newlib/$(TARBALL_newlib)
URL_$(TARBALL_newlib) = $(URL_newlib)

TARBALL_idl4 = idl4-$(VERSION_idl4).tgz
UNPACK_idl4 = idl4-$(VERSION_idl4)
URL_idl4 = http://l4ka.org/projects/idl4/$(TARBALL_idl4)
URL_$(TARBALL_idl4) = $(URL_idl4)

TARBALL_qemu = qemu-$(VERSION_qemu).tar.gz
UNPACK_qemu = qemu-$(VERSION_qemu)
URL_qemu = http://fabrice.bellard.free.fr/qemu/$(TARBALL_qemu)
URL_$(TARBALL_qemu) = $(URL_qemu)

TARBALL_qemu-floppy = qemu-afterburner-floppy.img.gz
URL_qemu-floppy = http://l4ka.org/download/tools/$(TARBALL_qemu-floppy)
URL_$(TARBALL_qemu-floppy) = $(URL_qemu-floppy)

TARBALL_gdb = gdb-$(VERSION_gdb).tar.bz2
UNPACK_gdb = gdb-$(VERSION_gdb)
URL_gdb = http://ftp.gnu.org/gnu/gdb/$(TARBALL_gdb)
URL_$(TARBALL_gdb) = $(URL_gdb)

TARBALL_module-init-tools = module-init-tools-$(VERSION_module-init-tools).tar.gz
UNPACK_module-init-tools = module-init-tools-$(VERSION_module-init-tools)
URL_module-init-tools = http://www.kernel.org/pub/linux/utils/kernel/module-init-tools/$(TARBALL_module-init-tools)
URL_$(TARBALL_module-init-tools) = $(URL_module-init-tools)

TARBALL_genromfs = genromfs-$(VERSION_genromfs).tar.gz
UNPACK_genromfs = genromfs-$(VERSION_genromfs)
URL_genromfs = http://www.ibiblio.org/pub/Linux/system/recovery/$(TARBALL_genromfs)
URL_$(TARBALL_genromfs) = $(URL_genromfs)

TARBALL_xnu = xnu-$(VERSION_xnu).tar.gz
UNPACK_xnu = xnu-$(VERSION_xnu)

TARBALL_python = Python-$(VERSION_python).tgz
UNPACK_python = Python-$(VERSION_python)
URL_python = http://python.org/ftp/python/$(VERSION_python)/$(TARBALL_python)
URL_$(TARBALL_python) = $(URL_python)

TARBALL_hg = mercurial-$(VERSION_hg).tar.gz
UNPACK_hg = mercurial-$(VERSION_hg)
URL_hg = http://www.selenic.com/mercurial/release/$(TARBALL_hg)
URL_$(TARBALL_hg) = $(URL_hg)

######################################################################
#
# Build rules.
#
######################################################################

PATH := $(INSTALL_ROOT)/bin:$(PATH)
LD_LIBRARY_PATH := $(INSTALL_ROOT)/lib:$(LD_LIBRARY_PATH)
export LD_LIBRARY_PATH
AFTERBURN_BINUTILS := $(INSTALL_ROOT)/bin
export AFTERBURN_BINUTILS

all: help

clean: help-clean

help::
	@echo
	@echo "Use 'make world' to build everything."
	@echo "Use 'make retrieve-all' to first download all packages, rather"
	@echo "than download on demand."
	@echo
	@echo "Help targets:"
	@echo "  help-patch"
	@echo "  help-build"
	@echo "  help-boot"
	@echo "  help-clean"
	@echo
	@echo "All retrieved source archives are stored in '$(ARCHIVE_DIR)'."
	@echo "Source packages are unarchived into '$(SRCDIR)'."
	@echo "Most packages are built in '$(BUILD_DIR)'."
	@echo "Utilities are installed into '$(INSTALL_ROOT)'."
	@echo "The boot directory is '$(BOOT_DIR)'."
	@echo
	@echo "By default, the first serial port is used for console output."
	@echo "Set VGA=1 on the command line to choose the VGA console."
	@echo
	@echo "Set DEBUG=1 on the command line to see each command."
	@echo
	@echo "     Host architecture: $(HOST_ARCH)"
	@echo "         Host platform: $(HOST_PLATFORM)"
	@echo "   Target architecture: $(TARGET_ARCH)"
	@echo "Cross-compilation type: $(TOOLPREFIX)"
	@echo

help-patch::
	@echo
	@echo "Patch targets:"

help-build::
	@echo
	@echo "Use 'make world' to build everything."
	@echo "Use 'make build-runtime' to build the runtime components."
	@echo "Use 'make build-utils' to build the support utilities."
	@echo "Use 'make build-vmi' to build everything related to VMI."
	@echo "Use 'make build-console' to build all targets that have "
	@echo "    a hard-coded console selection."
	@echo
	@echo "Build targets:"

help-boot::
	@echo
	@echo "Boot targets:"

help-clean::
	@echo
	@echo "clean-world    - clean all build targets."
	@echo "clean-runtime  - clean all runtime build targets."
	@echo "clean-utils    - clean all build utilities."
	@echo "clean-console  - clean all runtime build targets that have"
	@echo "                 a hard-coded console selection."
	@echo
	@echo "Clean targets:"

.PHONY: world build-utils build-runtime clean-world clean-runtime clean-console
world: build-utils build-runtime
build-vmi::
build-utils:: build-utils-$(TARGET_ARCH)
build-runtime:: build-runtime-$(TARGET_ARCH)
build-console::

clean-world: clean-utils clean-runtime
clean-utils:: clean-utils-$(TARGET_ARCH)
clean-runtime:: clean-runtime-$(TARGET_ARCH)
clean-console::

## The architecture-specific rules for build-utils, build-runtime, etc.
build-utils-$(TOOLPREFIX)::
build-utils-i386 build-utils-powerpc::
build-runtime-i386 build-runtime-powerpc::
clean-utils-i386 clean-utils-powerpc::
clean-runtime-i386 clean-utils-powerpc::

$(ARCHIVE_DIR)/%:
	@echo Retrieving the remote archive for $(@F)
	$(Q) mkdir -p $(ARCHIVE_DIR)
	$(Q) trap "curl --output $@ $(URL_$(@F)) && exit 0" ERR && wget -O $@ $(URL_$(@F))

retrieve-all: \
  $(foreach name,$(TARBALLS),$(ARCHIVE_DIR)/$(TARBALL_$(name))) \
  $(foreach name,$(CVS_LIST),receipts/cvs-$(name))

-include Makefile.patch
-include Makefile.l4ka
-include Makefile.local

######################################################################
#
# Rules to apply patches.
#
######################################################################

help-patch::
	@echo "  patch-binutils"
	@echo "  patch-linux-2.6"
	@echo "  patch-xen-2.0"
	@echo "  patch-qemu"
	@echo "  patch-idl4"
	@echo "  patch-pistachio"

PATCH = patch -Np1

.PHONY: patch-binutils patch-linux-2.6 patch-xen-2.0 patch-qemu patch-idl4 patch-pistachio patch-linux-2.4
patch-binutils: receipts/patch-binutils-$(VERSION_binutils)
patch-linux-2.6: receipts/patch-linux-$(VERSION_linux-2.6)
patch-linux-2.6-vmi: receipts/patch-linux-$(VERSION_linux-2.6-vmi)
patch-linux-2.4: receipts/patch-linux-$(VERSION_linux-2.4)
patch-xen-2.0: receipts/patch-xen-$(VERSION_xen-2)
patch-xen-3.0: receipts/patch-xen-$(VERSION_xen-3)
patch-qemu: receipts/patch-qemu-$(VERSION_qemu)
patch-idl4: receipts/patch-idl4-$(VERSION_idl4)
patch-pistachio: receipts/patch-pistachio

PATCH_FILE-binutils = $(CURDIR)/binutils/binutils-$(VERSION_binutils)-afterburn.patch
PATCH_FILE-linux-2.6 = $(CURDIR)/linux/linux-$(VERSION_linux-2.6)-big-afterburn.patch
PATCH_FILE-linux-2.6-vmi = $(CURDIR)/linux/linux-$(VERSION_linux-2.6-vmi)-vmi.patch
PATCH_FILE-linux-2.4 = $(CURDIR)/linux/linux-$(VERSION_linux-2.4)-big-afterburn.patch
PATCH_FILE-qemu = $(CURDIR)/qemu/qemu-$(VERSION_qemu).patch
PATCH_FILE-xen-2 = $(CURDIR)/xen/xen-$(VERSION_xen-2)-afterburn.patch
PATCH_FILE-xen-3 = $(CURDIR)/xen/xen-$(VERSION_xen-3)-afterburn.patch
PATCH_FILE-idl4 = $(CURDIR)/idl4/idl4-$(VERSION_idl4).patch
PATCH_FILE-pistachio = $(CURDIR)/pistachio/pistachio-kernel-perfmon.patch

receipts/patch-binutils-$(VERSION_binutils): type=binutils
receipts/patch-binutils-$(VERSION_binutils): untar=jxf
receipts/patch-binutils-$(VERSION_binutils): $(ARCHIVE_DIR)/$(TARBALL_binutils) $(PATCH_FILE-binutils)

receipts/patch-linux-$(VERSION_linux-2.6): type=linux-2.6
receipts/patch-linux-$(VERSION_linux-2.6): untar=jxf
receipts/patch-linux-$(VERSION_linux-2.6): $(ARCHIVE_DIR)/$(TARBALL_linux-2.6) $(PATCH_FILE-linux-2.6)

receipts/patch-linux-$(VERSION_linux-2.6-vmi)-vmi: type=linux-2.6-vmi
receipts/patch-linux-$(VERSION_linux-2.6-vmi)-vmi: untar=jxf
receipts/patch-linux-$(VERSION_linux-2.6-vmi)-vmi: $(ARCHIVE_DIR)/$(TARBALL_linux-2.6-vmi) $(PATCH_FILE-linux-2.6-vmi)

receipts/patch-linux-$(VERSION_linux-2.4): type=linux-2.4
receipts/patch-linux-$(VERSION_linux-2.4): untar=jxf
receipts/patch-linux-$(VERSION_linux-2.4): $(ARCHIVE_DIR)/$(TARBALL_linux-2.4) $(PATCH_FILE-linux-2.4)

receipts/patch-qemu-$(VERSION_qemu): type=qemu
receipts/patch-qemu-$(VERSION_qemu): untar=zxf
receipts/patch-qemu-$(VERSION_qemu): $(ARCHIVE_DIR)/$(TARBALL_qemu) $(PATCH_FILE-qemu)

receipts/patch-idl4-$(VERSION_idl4): type=idl4
receipts/patch-idl4-$(VERSION_idl4): untar=zxf
receipts/patch-idl4-$(VERSION_idl4): $(ARCHIVE_DIR)/$(TARBALL_idl4) $(PATCH_FILE-idl4)

receipts/patch-xen-$(VERSION_xen-2): type=xen-2
receipts/patch-xen-$(VERSION_xen-2): untar=zxf
receipts/patch-xen-$(VERSION_xen-2): $(ARCHIVE_DIR)/$(TARBALL_xen-2) $(PATCH_FILE-xen-2)

receipts/patch-xen-$(VERSION_xen-3): type=xen-3
receipts/patch-xen-$(VERSION_xen-3): untar=zxf
receipts/patch-xen-$(VERSION_xen-3): $(ARCHIVE_DIR)/$(TARBALL_xen-3) $(PATCH_FILE-xen-3)


receipts/patch-%:
	@echo Extracting the archive $(TARBALL_$(type)) to apply a new patch.
	@if [ -d $(SRCDIR)/$(UNPACK_$(type)) ] ; then \
	  echo $(SRCDIR)/$(UNPACK_$(type)) already exists. ; \
	  echo Please remove it first. ; \
	  exit 1 ; \
	fi
	$(Q) mkdir -p $(SRCDIR) receipts
	$(Q) (cd $(SRCDIR) && tar $(untar) $(ARCHIVE_DIR)/$(TARBALL_$(type)))
	$(Q) if [ -n "$(PREUNPACK_$(type))" ] ; then cd $(SRCDIR) && mv $(PREUNPACK_$(type)) $(UNPACK_$(type)) ; fi
	@echo Applying the patch.
	$(Q) (cd $(SRCDIR)/$(UNPACK_$(type)) && $(PATCH) < $(PATCH_FILE-$(type)))
	$(Q) touch $@
	@echo The newly patched project: $(SRCDIR)/$(UNPACK_$(type))

receipts/patch-pistachio: receipts/cvs-pistachio $(PATCH_FILE-pistachio)
	@echo Applying the patch.
	$(Q) (cd $(SRCDIR)/pistachio && $(PATCH) < $(PATCH_FILE-pistachio))
	$(Q) touch $@
	@echo The newly patched project: $(SRCDIR)/pistachio

######################################################################
# Build binutils
######################################################################

help-build::
	@echo "  build-binutils"
help-clean::
	@echo "  clean-binutils"
build-utils:: build-binutils
clean-utils:: clean-binutils

build-binutils: receipts/install-binutils-$(VERSION_binutils)

ifdef TOOLPREFIX
OPTS_binutils = --target=$(TARGET_ARCH)-elf --program-prefix=$(TOOLPREFIX)
endif
ifeq ($(HOST_PLATFORM),Darwin)
ENV_binutils = env CC="cc -no-cpp-precomp"
endif

receipts/install-binutils-$(VERSION_binutils): receipts/patch-binutils-$(VERSION_binutils)
	@echo Building binutils in $(BUILD_DIR)/$(UNPACK_binutils)
	$(Q) mkdir -p $(BUILD_DIR)/$(UNPACK_binutils) receipts $(INSTALL_ROOT)
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_binutils) && $(ENV_binutils) $(SRCDIR)/$(UNPACK_binutils)/configure --prefix=$(INSTALL_ROOT) --disable-nls $(OPTS_binutils))
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_binutils) && make all)
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_binutils) && make install)
	$(Q) touch $@

clean-binutils:
	-$(Q) (cd $(BUILD_DIR) && rm -rf $(UNPACK_binutils))
	-$(Q) rm -f receipts/install-binutils-$(VERSION_binutils)

######################################################################
# Build gcc
######################################################################

help-build::
	@echo "  build-gcc"
help-clean::
	@echo "  clean-gcc"
build-utils-$(TOOLPREFIX):: build-gcc
clean-utils-$(TOOLPREFIX):: clean-gcc

build-gcc: receipts/install-gcc-$(VERSION_gcc)

receipts/extract-gcc-$(VERSION_gcc): $(ARCHIVE_DIR)/$(TARBALL_gcc)
	@echo Extracting $(<F) in $(SRCDIR).
	$(Q) mkdir -p $(SRCDIR) receipts
	$(Q) (cd $(SRCDIR) && tar jxf $<)
	$(Q) touch $@

receipts/extract-newlib-$(VERSION_newlib): $(ARCHIVE_DIR)/$(TARBALL_newlib) receipts/extract-gcc-$(VERSION_gcc)
	@echo Extracting $(<F) in $(SRCDIR).
	$(Q) mkdir -p $(SRCDIR) receipts
	$(Q) (cd $(SRCDIR) && tar zxf $<)
	$(Q) ln -s $(SRCDIR)/$(UNPACK_newlib)/newlib $(SRCDIR)/$(UNPACK_gcc)/.
	$(Q) touch $@

ifdef TOOLPREFIX
  DEPS_install_gcc = receipts/extract-newlib-$(VERSION_newlib)
  OPTS_gcc = --target=$(TARGET_ARCH)-elf --program-prefix=$(TOOLPREFIX) --with-newlib
endif
ifeq ($(HOST_PLATFORM),Darwin)
  ENV_gcc = env CC="cc -no-cpp-precomp"
endif

receipts/install-gcc-$(VERSION_gcc): receipts/extract-gcc-$(VERSION_gcc) $(DEPS_install_gcc)
	@echo Building gcc in $(BUILD_DIR)/$(UNPACK_gcc)
	$(Q) mkdir -p $(BUILD_DIR)/$(UNPACK_gcc) receipts $(INSTALL_ROOT)
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_gcc) && $(ENV_gcc) $(SRCDIR)/$(UNPACK_gcc)/configure --prefix=$(INSTALL_ROOT) --with-gnu-as --with-gnu-ld --enable-languages="c,c++" $(OPTS_gcc))
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_gcc) && make all)
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_gcc) && make install)
	$(Q) touch $@

clean-gcc:
	-$(Q) (cd $(BUILD_DIR) && rm -rf $(UNPACK_gcc))
	-$(Q) rm -f receipts/install-gcc-$(VERSION_gcc)


######################################################################
# Build static module-init-tools.  They do not require libc.
######################################################################

help-build::
	@echo "  build-module-init-tools"
help-clean::
	@echo "  clean-module-init-tools"
#build-utils-i386:: build-module-init-tools
#clean-utils-i386:: clean-module-init-tools

module-init-tools = insmod modprobe rmmod depmod modinfo
INSTALL_module-init-tools = $(addprefix $(INSTALL_ROOT)/sbin/,$(module-init-tools))
BUILD_module-init-tools = $(addprefix $(BUILD_DIR)/$(UNPACK_module-init-tools)/,$(module-init-tools))

build-module-init-tools: $(INSTALL_module-init-tools)

$(INSTALL_module-init-tools): $(BUILD_module-init-tools)
	@mkdir -p $(INSTALL_ROOT)/sbin
	@cp $^ $(@D)/.
	@strip $(INSTALL_module-init-tools)

receipts/extract-module-init-tools-$(VERSION_module-init-tools): $(ARCHIVE_DIR)/$(TARBALL_module-init-tools)
	@echo Extracting $(<F) in $(SRCDIR).
	$(Q) mkdir -p $(SRCDIR) receipts
	$(Q) (cd $(SRCDIR) && tar zxf $<)
	$(Q) touch $@

$(BUILD_module-init-tools): receipts/extract-module-init-tools-$(VERSION_module-init-tools)
	@echo Building module-init-tools in $(BUILD_DIR)/$(UNPACK_module-init-tools)
	$(Q) mkdir -p $(BUILD_DIR)/$(UNPACK_module-init-tools)
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_module-init-tools) && $(SRCDIR)/$(UNPACK_module-init-tools)/configure --prefix=$(INSTALL_ROOT) CFLAGS=-static)
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_module-init-tools) && make insmod modprobe rmmod depmod modinfo)

clean-module-init-tools:
	-$(Q) (cd $(BUILD_DIR) && rm -rf $(UNPACK_module-init-tools))
	-$(Q) rm -f receipts/install-module-init-tools-$(VERSION_module-init-tools)
	-$(Q) rm -f receipts/extract-module-init-tools-$(VERSION_module-init-tools)


######################################################################
# Build genromfs
######################################################################

help-build::
	@echo "  build-genromfs"
help-clean::
	@echo "  clean-genromfs"
#build-utils-i386:: build-genromfs
#clean-utils-i386:: clean-genromfs

build-genromfs: $(INSTALL_ROOT)/bin/genromfs

$(INSTALL_ROOT)/bin/genromfs: $(SRCDIR)/$(UNPACK_genromfs)/genromfs
	@cp $< $@

receipts/extract-genromfs-$(VERSION_genromfs): $(ARCHIVE_DIR)/$(TARBALL_genromfs)
	@echo Extracting $(<F) in $(SRCDIR).
	$(Q) mkdir -p $(SRCDIR) receipts
	$(Q) (cd $(SRCDIR) && tar zxf $<)
	$(Q) touch $@

$(SRCDIR)/$(UNPACK_genromfs)/genromfs: receipts/extract-genromfs-$(VERSION_genromfs)
	@echo Building genromfs in $(SRCDIR)/$(UNPACK_genromfs)
	$(Q) (cd $(SRCDIR)/$(UNPACK_genromfs) && make genromfs)

clean-genromfs:
	-$(Q) (cd $(SRCDIR)/$(UNPACK_genromfs) && make clean)


######################################################################
# Build gdb and psim
######################################################################

help-build::
	@echo "  build-psim"
help-clean::
	@echo "  clean-psim"
build-utils-powerpc:: build-psim
clean-utils-powerpc:: clean-psim

build-psim: receipts/install-psim-$(VERSION_gdb)

receipts/extract-gdb-$(VERSION_gdb): $(ARCHIVE_DIR)/$(TARBALL_gdb)
	@echo Extracting $(<F) in $(SRCDIR).
	$(Q) mkdir -p $(SRCDIR) receipts
	$(Q) (cd $(SRCDIR) && tar jxf $<)
	$(Q) touch $@

ifeq ($(HOST_PLATFORM),Darwin)
  ENV_gdb = env CC="cc -no-cpp-precomp"
endif

receipts/install-psim-$(VERSION_gdb): receipts/extract-gdb-$(VERSION_gdb)
	@echo Building psim in $(BUILD_DIR)/$(UNPACK_gdb)-psim
	$(Q) mkdir -p $(BUILD_DIR)/$(UNPACK_gdb)-psim receipts $(INSTALL_ROOT)
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_gdb)-psim && $(ENV_gdb) $(SRCDIR)/$(UNPACK_gdb)/configure --prefix=$(INSTALL_ROOT) --enable-sim-powerpc --target=powerpc-unknown-eabi --disable-nls)
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_gdb)-psim && make all)
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_gdb)-psim && make install)
	$(Q) touch $@

clean-psim:
	-$(Q) (cd $(BUILD_DIR) && rm -rf $(UNPACK_gdb)-psim)
	-$(Q) rm -f receipts/install-psim-$(VERSION_gdb)


######################################################################
# CVS for external projects
######################################################################
MARZIPAN_CVS ?= marzipan
RESOURCEMON_CVS ?= resourcemon

.PHONY: pistachio-cvs marzipan-cvs idl4-cvs
pistachio-cvs: receipts/cvs-pistachio
marzipan-cvs: receipts/cvs-marzipan
idl4-cvs: receipts/cvs-idl4

receipts/cvs-pistachio:
	@echo Retrieving Pistachio from CVS.
	$(Q) mkdir -p $(SRCDIR) receipts
	$(Q) cd $(SRCDIR) && cvs -Q -d $(L4KA_CVS_ROOT) co pistachio
	$(Q) touch $@

receipts/cvs-marzipan:
	@echo Retrieving Marzipan from CVS.
	$(Q) mkdir -p $(SRCDIR) receipts
	$(Q) cd $(SRCDIR) && cvs -Q -d $(L4KA_CVS_ROOT) co $(MARZIPAN_CVS)/$(RESOURCEMON_CVS)
	$(Q) cd $(SRCDIR) && cvs -Q -d $(L4KA_CVS_ROOT) co $(MARZIPAN_CVS)/interfaces
	$(Q) touch $@

receipts/cvs-idl4:
	@echo Retrieving IDL4 from CVS.
	$(Q) mkdir -p $(SRCDIR) receipts
	$(Q) cd $(SRCDIR) && cvs -Q -d $(L4KA_CVS_ROOT) co idl4
	$(Q) touch $@

receipts/cvs-rhype:
	@echo Retrieving rHype from CVS.
	$(Q) mkdir -p $(SRCDIR) receipts
	$(Q) cd $(SRCDIR) && cvs -Q -d $(RHYPE_CVS_ROOT) -z 6 co rhype
	$(Q) touch $@

######################################################################
# Mercurial external projects
######################################################################

receipts/hg-xen-3:
	@echo Retrieving Xen from Hg.
	$(Q) mkdir -p $(SRCDIR) receipts
	$(Q) cd $(SRCDIR) && hg clone http://xenbits.xensource.com/xen-unstable.hg
	$(Q) touch $@

######################################################################
# Build Pistachio kernel
######################################################################
help-build::
	@echo "  build-pistachio-kernel"
	@echo "    build-pistachio-kernel-p3"
	@echo "    build-pistachio-kernel-p4"
	@echo "    build-pistachio-kernel-qemu"
help-clean::
	@echo "  clean-pistachio-kernel"
	@echo "    clean-pistachio-kernel-p3"
	@echo "    clean-pistachio-kernel-p4"
	@echo "    clean-pistachio-kernel-qemu"
build-runtime:: build-pistachio-kernel
build-console:: build-pistachio-kernel
clean-runtime:: clean-pistachio-kernel
clean-console:: clean-pistachio-kernel

build-pistachio-kernel: build-pistachio-kernel-$(TARGET_ARCH)

build-pistachio-kernel-i386: \
  build-pistachio-kernel-p3 build-pistachio-kernel-p4 \
  build-pistachio-kernel-qemu
build-pistachio-kernel-powerpc: \
  build-pistachio-kernel-powerpc-qemu

# Give enough declarations to Make to figure out that 
# pattern-matching rules are unsatisfied.
build-pistachio-kernel-p3: type=p3
build-pistachio-kernel-p3: \
  $(BUILD_DIR)/pistachio-kernel-p3/ia32-kernel $(BOOT_DIR)/pistachio-p3

build-pistachio-kernel-p4: type=p4
build-pistachio-kernel-p4: \
  $(BUILD_DIR)/pistachio-kernel-p4/ia32-kernel $(BOOT_DIR)/pistachio-p4

build-pistachio-kernel-p4perf: type=p4perf
build-pistachio-kernel-p4perf: \
  $(BUILD_DIR)/pistachio-kernel-p4perf/ia32-kernel $(BOOT_DIR)/pistachio-p4perf

build-pistachio-kernel-qemu: type=qemu
build-pistachio-kernel-qemu: \
  $(BUILD_DIR)/pistachio-kernel-qemu/ia32-kernel $(BOOT_DIR)/pistachio-qemu

build-pistachio-kernel-amdk8: type=amdk8
build-pistachio-kernel-amdk8: \
  $(BUILD_DIR)/pistachio-kernel-amdk8/ia32-kernel $(BOOT_DIR)/pistachio-amdk8

build-pistachio-kernel-powerpc-qemu: type=powerpc-qemu
build-pistachio-kernel-powerpc-qemu: \
  $(BUILD_DIR)/pistachio-kernel-powerpc-qemu/powerpc-kernel \
  $(BOOT_DIR)/pistachio-powerpc-qemu

$(BOOT_DIR)/pistachio-%: $(BUILD_DIR)/pistachio-kernel-%/ia32-kernel
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

$(BOOT_DIR)/pistachio-%: $(BUILD_DIR)/pistachio-kernel-%/powerpc-kernel
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

OPTS_pistachio-kernel-p3 = ARCH_IA32 CPU_IA32_I686 PLAT_PC99 SMP=n IOAPIC=n \
  IPC_FASTPATH DEBUG EXPERIMENTAL X_PAGER_EXREGS PERFMON IA32_SMALL_SPACES=n \
  SPIN_WHEELS=n KDB KDB_DISAS KDB_BREAKIN TRACEPOINTS KMEM_TRACE
ifdef VGA
  OPTS_pistachio-kernel-p3 += KDB_CONS_KBD
  OPTS2_pistachio-kernel-p3 = KDB_BREAKIN_ESCAPE
else
  OPTS_pistachio-kernel-p3 += KDB_CONS_COM
  OPTS2_pistachio-kernel-p3 = KDB_BREAKIN_BREAK KDB_BREAKIN_ESCAPE=n
endif

OPTS_pistachio-kernel-p4 = ARCH_IA32 CPU_IA32_P4 PLAT_PC99 SMP=n IOAPIC=n \
  IPC_FASTPATH DEBUG EXPERIMENTAL X_PAGER_EXREGS PERFMON IA32_SMALL_SPACES=n \
  SPIN_WHEELS=n KDB KDB_DISAS KDB_BREAKIN TRACEPOINTS KMEM_TRACE
ifdef VGA
  OPTS_pistachio-kernel-p4 += KDB_CONS_KBD
  OPTS2_pistachio-kernel-p4 = KDB_BREAKIN_ESCAPE
else
  OPTS_pistachio-kernel-p4 += KDB_CONS_COM
  OPTS2_pistachio-kernel-p4 = KDB_BREAKIN_BREAK KDB_BREAKIN_ESCAPE=n
endif

OPTS_pistachio-kernel-p4perf = ARCH_IA32 CPU_IA32_P4 PLAT_PC99 SMP=n IOAPIC=n \
  IPC_FASTPATH DEBUG EXPERIMENTAL X_PAGER_EXREGS PERFMON IA32_SMALL_SPACES=n \
  SPIN_WHEELS=n KDB KDB_DISAS KDB_NO_ASSERTS=y
ifdef VGA
  OPTS_pistachio-kernel-p4perf += KDB_CONS_KBD
else
  OPTS_pistachio-kernel-p4perf += KDB_CONS_COM
endif

OPTS_pistachio-kernel-amdk8 = ARCH_IA32 CPU_IA32_K8 PLAT_PC99 SMP=n IOAPIC=n \
  IPC_FASTPATH DEBUG EXPERIMENTAL X_PAGER_EXREGS PERFMON IA32_SMALL_SPACES=n \
  SPIN_WHEELS=n KDB KDB_DISAS KDB_BREAKIN TRACEPOINTS KMEM_TRACE
ifdef VGA
  OPTS_pistachio-kernel-amdk8 += KDB_CONS_KBD
  OPTS2_pistachio-kernel-amdk8 = KDB_BREAKIN_ESCAPE
else
  OPTS_pistachio-kernel-amdk8 += KDB_CONS_COM
  OPTS2_pistachio-kernel-amdk8 = KDB_BREAKIN_BREAK KDB_BREAKIN_ESCAPE=n
endif

OPTS_pistachio-kernel-powerpc-qemu = ARCH_POWERPC IPC_FASTPATH DEBUG \
  EXPERIMENTAL X_PAGER_EXREGS KDB KDB_DISAS KDB_ON_STARTUP \
  VERBOSE_INIT TRACEPOINTS KMEM_TRACE
OPTS2_pistachio-kernel-powerpc-qemu = KDB_CONS_PSIM_COM=y KDB_CONS_OF1275=y

## QEMU:
## - Currently has a problem with the debugger break-in via escape.  For
##   some reason, most of the input stream is clobbered in the search for
##   the escape key.
OPTS_pistachio-kernel-qemu = ARCH_IA32 CPU_IA32_I586 PLAT_PC99 SMP=n IOAPIC=n \
  IPC_FASTPATH DEBUG EXPERIMENTAL X_PAGER_EXREGS PERFMON \
  IA32_SMALL_SPACES=n SPIN_WHEELS=n KDB KDB_DISAS KDB_BREAKIN TRACEPOINTS \
  KMEM_TRACE
ifdef VGA
  OPTS_pistachio-kernel-qemu += KDB_CONS_KBD
  OPTS2_pistachio-kernel-qemu = KDB_BREAKIN_ESCAPE
else
  OPTS_pistachio-kernel-qemu += KDB_CONS_COM
  OPTS2_pistachio-kernel-qemu = KDB_BREAKIN_BREAK KDB_BREAKIN_ESCAPE
endif

.PRECIOUS: $(BUILD_DIR)/pistachio-kernel-%/config/config.h
$(BUILD_DIR)/pistachio-kernel-%/config/config.h: receipts/patch-pistachio
	$(Q) mkdir -p $(BUILD_DIR)
	$(Q) (cd $(SRCDIR)/pistachio/kernel && make BUILDDIR=$(BUILD_DIR)/pistachio-kernel-$(type))
	$(Q) (cd $(BUILD_DIR)/pistachio-kernel-$(type) && make batchconfig CMLBATCH_PARAMS='$(OPTS_pistachio-kernel-$(type))')
	$(Q) (cd $(BUILD_DIR)/pistachio-kernel-$(type) && make batchconfig CMLBATCH_PARAMS='$(OPTS2_pistachio-kernel-$(type))')

$(BUILD_DIR)/pistachio-kernel-%/ia32-kernel: $(BUILD_DIR)/pistachio-kernel-%/config/config.h
	@echo Building Pistachio microkernel in $(@D)
	$(Q) (cd $(@D) && make -s NO_CCACHE=1 TOOLPREFIX=$(TOOLPREFIX))

$(BUILD_DIR)/pistachio-kernel-%/powerpc-kernel: $(BUILD_DIR)/pistachio-kernel-%/config/config.h
	@echo Building Pistachio microkernel in $(@D)
	$(Q) (cd $(@D) && make -s NO_CCACHE=1 TOOLPREFIX=$(TOOLPREFIX))

reconfig-pistachio-kernel-%:
	$(Q) (cd $(BUILD_DIR)/$(subst reconfig-,,$@) && make menuconfig)

clean-pistachio-kernel: \
  clean-pistachio-kernel-p4 clean-pistachio-kernel-p3 \
  clean-pistachio-kernel-qemu clean-pistachio-kernel-amdk8 \
  clean-pistachio-kernel-powerpc-qemu

clean-pistachio-kernel-%:
	-$(Q) (cd $(BUILD_DIR) && rm -rf $(subst clean-,,$@))


######################################################################
# Build Pistachio user
######################################################################
help-build::
	@echo "  build-pistachio-user"
help-clean::
	@echo "  clean-pistachio-user"
build-runtime:: build-pistachio-user
build-console:: build-pistachio-user
clean-runtime:: clean-pistachio-user
clean-console:: clean-pistachio-user

build-pistachio-user: $(BOOT_DIR)/sigma0 $(BOOT_DIR)/pingpong build-pistachio-user-$(TARGET_ARCH)
build-pistachio-user-i386: $(BOOT_DIR)/kickstart
build-pistachio-user-powerpc: $(BOOT_DIR)/ofppc-loader

$(BOOT_DIR)/sigma0: $(BUILD_DIR)/pistachio-user/serv/sigma0/sigma0
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

$(BOOT_DIR)/kickstart: $(BUILD_DIR)/pistachio-user/util/kickstart/kickstart
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

$(BOOT_DIR)/pingpong: $(BUILD_DIR)/pistachio-user/apps/bench/pingpong/pingpong
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

$(BOOT_DIR)/ofppc-loader: $(BUILD_DIR)/pistachio-user/util/piggybacker/ofppc/ofppc-loader
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

pistachio_user_results = \
  $(BUILD_DIR)/pistachio-user/serv/sigma0/sigma0 \
  $(BUILD_DIR)/pistachio-user/util/kickstart/kickstart \
  $(BUILD_DIR)/pistachio-user/apps/bench/pingpong/pingpong \
  $(BUILD_DIR)/pistachio-user/util/piggybacker/ofppc/ofppc-loader

PISTACHIO_USER_OPTS = \
  --with-kerneldir=$(BUILD_DIR)/pistachio-kernel-powerpc-qemu \
  --prefix=$(INSTALL_ROOT)
ifdef VGA
  PISTACHIO_USER_OPTS += --without-comport
endif
ifdef TOOLPREFIX
  PISTACHIO_USER_OPTS += --host=$(TARGET_ARCH)
endif

receipts/config-pistachio-user: receipts/patch-pistachio
	$(Q) mkdir -p $(BUILD_DIR)/pistachio-user receipts
	$(Q) if [ -f $(SRCDIR)/pistachio/user/README.CVS ] ; then cd $(SRCDIR)/pistachio/user && (autoheader2.50 || autoheader) && (autoconf2.50 || autoconf) ; fi
	$(Q) (cd $(BUILD_DIR)/pistachio-user && $(SRCDIR)/pistachio/user/configure $(PISTACHIO_USER_OPTS))
	$(Q) touch $@

receipts/install-pistachio-user $(pistachio_user_results): receipts/config-pistachio-user
	@echo Building Pistachio user in $(BUILD_DIR)/pistachio-user
	$(Q) mkdir -p $(INSTALL_ROOT) receipts
	$(Q) (cd $(BUILD_DIR)/pistachio-user && make -s)
	$(Q) (cd $(BUILD_DIR)/pistachio-user && make -s install)
	$(Q) touch $@

clean-pistachio-user:
	-$(Q) (cd $(BUILD_DIR) && rm -rf pistachio-user)
	-$(Q) rm -f receipts/config-pistachio-user
	-$(Q) rm -f receipts/install-pistachio-user

######################################################################
# Build IDL4
######################################################################
help-build::
	@echo "  build-idl4"
help-clean::
	@echo "  clean-idl4"
build-utils:: build-idl4
clean-utils:: clean-idl4

build-idl4: receipts/install-idl4-$(VERSION_idl4)

ifeq ($(HOST_PLATFORM),Darwin)
ENV_idl4 = env CC="cc -no-cpp-precomp"
endif

receipts/install-idl4-$(VERSION_idl4): receipts/patch-idl4-$(VERSION_idl4)
	@echo Building IDL4 in $(BUILD_DIR)/$(UNPACK_idl4)
	$(Q) mkdir -p $(BUILD_DIR)/$(UNPACK_idl4) $(INSTALL_ROOT) receipts
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_idl4) && $(ENV_idl4) $(SRCDIR)/$(UNPACK_idl4)/configure --prefix=$(INSTALL_ROOT))
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_idl4) && make -s)
	$(Q) (cd $(BUILD_DIR)/$(UNPACK_idl4) && make -s install)
	$(Q) touch $@

clean-idl4:
	-$(Q) (cd $(BUILD_DIR) && rm -rf $(UNPACK_idl4))
	-$(Q) rm -f receipts/install-idl4-$(VERSION_idl4)

######################################################################
# Build Marzipan
######################################################################
help-build::
	@echo "  build-marzipan"
help-clean::
	@echo "  clean-marzipan"
build-runtime-i386:: build-marzipan
clean-runtime-i386:: clean-marzipan

build-marzipan: $(BOOT_DIR)/marzipan

$(BOOT_DIR)/marzipan: $(BUILD_DIR)/marzipan/ia32-hypervisor
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

receipts/config-marzipan: receipts/cvs-marzipan receipts/install-idl4-$(VERSION_idl4)
	$(Q) mkdir -p $(BUILD_DIR) receipts
	$(Q) (cd $(SRCDIR)/$(MARZIPAN_CVS)/$(RESOURCEMON_CVS) && make BUILDDIR=$(BUILD_DIR)/marzipan)
	$(Q) echo "#define CONFIG_PERFMON" >> $(BUILD_DIR)/marzipan/config.h
	$(Q) echo "#define CONFIG_VGA_PASSTHROUGH" >> $(BUILD_DIR)/marzipan/config.h
	$(Q) touch $@

##  We need IDL4 in the path.
$(BUILD_DIR)/marzipan/ia32-hypervisor: receipts/config-marzipan receipts/install-idl4-$(VERSION_idl4) receipts/install-pistachio-user
	@echo Building Marzipan in $(BUILD_DIR)/marzipan
	$(Q) (cd $(BUILD_DIR)/marzipan && make -s NO_CCACHE=1 PISTACHIO_USER=$(INSTALL_ROOT) TOOLPREFIX=$(TOOLPREFIX))

clean-marzipan:
	-$(Q) (cd $(BUILD_DIR) && rm -rf marzipan)
	-$(Q) rm -f receipts/config-marzipan

######################################################################
# Build the assembler Afterburner
######################################################################

help-build::
	@echo "  build-afterburner"
help-clean::
	@echo "  clean-afterburner"
build-utils:: build-afterburner
clean-utils:: clean-afterburner

.PHONY: build-afterburner
build-afterburner: $(INSTALL_ROOT)/bin/afterburner 

$(INSTALL_ROOT)/bin:
	mkdir -p $@

$(INSTALL_ROOT)/bin/afterburner: $(BUILD_DIR)/asm-parser/afterburner | $(INSTALL_ROOT)/bin
	$(Q) cp $< $@

$(BUILD_DIR)/asm-parser:
	mkdir -p $@

$(BUILD_DIR)/asm-parser/afterburner: | $(BUILD_DIR)/asm-parser
	@echo Building the Afterburner assembler parser.
	$(Q) (cd ../asm-parser && make O=$(BUILD_DIR)/asm-parser test)

.PHONY: clean-afterburner uninstall-afterburner rebuild-afterburner
clean-afterburner:
	$(Q) rm -rf $(BUILD_DIR)/asm-parser
uninstall-afterburner:
	$(Q) rm -f $(INSTALL_ROOT)/bin/afterburner
rebuild-afterburner: clean-afterburner uninstall-afterburner build-afterburner

######################################################################
# Build the Afterburn wedges
######################################################################

help-build::
	@echo "  build-wedge"
	@echo "    build-wedge-l4ka-passthru"
	@echo "    build-wedge-kaxen-passthru"
	@echo "    build-wedge-kaxen-vmi-passthru"
	@echo "    build-wedge-kaxen-3-passthru"
	@echo "    build-wedge-l4ka"
	@echo "    build-wedge-kaxen"
	@echo "    build-wedge-kaxen-vmi"
help-clean::
	@echo "  clean-wedge"
	@echo "    clean-wedge-l4ka-passthru"
	@echo "    clean-wedge-kaxen-passthru"
	@echo "    clean-wedge-kaxen-vmi-passthru"
	@echo "    clean-wedge-l4ka"
	@echo "    clean-wedge-kaxen-vmi"
build-runtime-i386:: build-wedge
clean-runtime-i386:: clean-wedge
build-vmi:: build-wedge-kaxen-vmi-passthru build-wedge-kaxen-vmi

build-wedge: build-wedge-l4ka-passthru build-wedge-kaxen-passthru build-wedge-kaxen

build-wedge-l4ka-prereqs: receipts/install-pistachio-user \
  receipts/install-idl4-$(VERSION_idl4) receipts/cvs-marzipan
build-wedge-kaxen-prereqs: receipts/patch-xen-$(VERSION_xen-2)
build-wedge-kaxen-3-prereqs:

ifdef TOOLPREFIX
  OPTS_wedge = TOOLPREFIX=$(TOOLPREFIX)
endif

build-wedge-l4ka-passthru: type=l4ka-passthru
build-wedge-l4ka-passthru: build-wedge-l4ka-prereqs $(BOOT_DIR)/afterburn-wedge-l4ka-passthru
OPTS_wedge-l4ka-passthru = WEDGE_L4KA SMP=n DEVICE_PASSTHRU \
  L4KA_INTERFACE_DIR=$(SRCDIR)/$(MARZIPAN_CVS)/interfaces \
  L4KA_PISTACHIO_USER=$(INSTALL_ROOT)

build-wedge-l4ka-passthru-perf: type=l4ka-passthru-perf
build-wedge-l4ka-passthru-perf: build-wedge-l4ka-prereqs $(BOOT_DIR)/afterburn-wedge-l4ka-passthru-perf
OPTS_wedge-l4ka-passthru-perf = WEDGE_L4KA SMP=n DEVICE_PASSTHRU \
  L4KA_INTERFACE_DIR=$(SRCDIR)/$(MARZIPAN_CVS)/interfaces \
  L4KA_PISTACHIO_USER=$(INSTALL_ROOT) OPTIMIZE=y


build-wedge-kaxen-passthru: type=kaxen-passthru
build-wedge-kaxen-passthru: build-wedge-kaxen-prereqs $(BOOT_DIR)/afterburn-wedge-kaxen-passthru
OPTS_wedge-kaxen-passthru = WEDGE_KAXEN SMP=n DEVICE_PASSTHRU \
  KAXEN_HYP_INCLUDE=$(SRCDIR)/$(UNPACK_xen-2)/xen/include/public KAXEN_INT_FP \
  XEN_2_0

build-wedge-kaxen-vmi-passthru: type=kaxen-vmi-passthru
build-wedge-kaxen-vmi-passthru: build-wedge-kaxen-prereqs $(BOOT_DIR)/afterburn-wedge-kaxen-vmi-passthru
OPTS_wedge-kaxen-vmi-passthru = WEDGE_KAXEN SMP=n DEVICE_PASSTHRU \
  KAXEN_HYP_INCLUDE=$(SRCDIR)/$(UNPACK_xen-2)/xen/include/public KAXEN_INT_FP \
  XEN_2_0 VMI_SUPPORT=y WEDGE_VIRT=0xe0000000 WEDGE_VIRT_END=0xf0000000

build-wedge-kaxen-vmi-passthru-perf: type=kaxen-vmi-passthru-perf
build-wedge-kaxen-vmi-passthru-perf: build-wedge-kaxen-prereqs $(BOOT_DIR)/afterburn-wedge-kaxen-vmi-passthru-perf
OPTS_wedge-kaxen-vmi-passthru-perf = WEDGE_KAXEN SMP=n DEVICE_PASSTHRU \
  OPTIMIZE=y KAXEN_INT_FP=y \
  KAXEN_HYP_INCLUDE=$(SRCDIR)/$(UNPACK_xen-2)/xen/include/public \
  XEN_2_0 VMI_SUPPORT=y WEDGE_VIRT=0xe0000000 WEDGE_VIRT_END=0xf0000000

build-wedge-kaxen-3-passthru: type=kaxen-3-passthru
build-wedge-kaxen-3-passthru: build-wedge-kaxen-3-prereqs $(BOOT_DIR)/afterburn-wedge-kaxen-3-passthru
OPTS_wedge-kaxen-3-passthru = WEDGE_KAXEN SMP=n DEVICE_PASSTHRU \
  KAXEN_HYP_INCLUDE=$(SRCDIR)/$(UNPACK_xen-3)/xen/include/public KAXEN_INT_FP \
  XEN_3_0


build-wedge-kaxen-passthru-perf: type=kaxen-passthru-perf
build-wedge-kaxen-passthru-perf: build-wedge-kaxen-prereqs $(BOOT_DIR)/afterburn-wedge-kaxen-passthru-perf
OPTS_wedge-kaxen-passthru-perf = WEDGE_KAXEN SMP=n DEVICE_PASSTHRU \
  OPTIMIZE=y KAXEN_INT_FP=y \
  KAXEN_HYP_INCLUDE=$(SRCDIR)/$(UNPACK_xen-2)/xen/include/public \
  XEN_2_0


build-wedge-l4ka: type=l4ka
build-wedge-l4ka: build-wedge-l4ka-prereqs $(BOOT_DIR)/afterburn-wedge-l4ka
OPTS_wedge-l4ka = WEDGE_L4KA SMP=n DEVICE_PASSTHRU=n DEVICE_DP83820=y \
  L4KA_INTERFACE_DIR=$(SRCDIR)/$(MARZIPAN_CVS)/interfaces \
  L4KA_PISTACHIO_USER=$(INSTALL_ROOT)

build-wedge-kaxen: type=kaxen
build-wedge-kaxen: build-wedge-kaxen-prereqs $(BOOT_DIR)/afterburn-wedge-kaxen
OPTS_wedge-kaxen = WEDGE_KAXEN SMP=n DEVICE_PASSTHRU=n \
  KAXEN_HYP_INCLUDE=$(SRCDIR)/$(UNPACK_xen-2)/xen/include/public KAXEN_INT_FP \
  XEN_2_0

build-wedge-kaxen-vmi: type=kaxen-vmi
build-wedge-kaxen-vmi: build-wedge-kaxen-prereqs $(BOOT_DIR)/afterburn-wedge-kaxen-vmi
OPTS_wedge-kaxen-vmi = WEDGE_KAXEN SMP=n DEVICE_PASSTHRU=n \
  KAXEN_HYP_INCLUDE=$(SRCDIR)/$(UNPACK_xen-2)/xen/include/public KAXEN_INT_FP \
  XEN_2_0 VMI_SUPPORT=y WEDGE_VIRT=0xe0000000 WEDGE_VIRT_END=0xf0000000



$(BOOT_DIR)/afterburn-wedge-%: $(BUILD_DIR)/afterburn-wedge-%/afterburn-wedge
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

.PRECIOUS: $(BUILD_DIR)/afterburn-wedge-%/config/config.h
$(BUILD_DIR)/afterburn-wedge-%/config/config.h:
	$(Q) mkdir -p $(BUILD_DIR)
	$(Q) (cd ../afterburn-wedge && make BUILDDIR=$(BUILD_DIR)/afterburn-wedge-$(type))
	$(Q) (cd $(BUILD_DIR)/afterburn-wedge-$(type) && make CMLBATCH_PARAMS='$(OPTS_wedge-$(type))' batchconfig)

.PRECIOUS: $(BUILD_DIR)/afterburn-wedge-%/afterburn-wedge
$(BUILD_DIR)/afterburn-wedge-%/afterburn-wedge: $(BUILD_DIR)/afterburn-wedge-%/config/config.h
	@echo Building the $(type) Afterburn Wedge in $(@D)
	$(Q) (cd $(@D) && make -s $(OPTS_wedge))

reconfig-wedge-%:
	$(Q) (cd $(BUILD_DIR)/$(subst reconfig,afterburn,$@) && make menuconfig)
rebuild-wedge-%:
	-$(Q) rm -f $(BOOT_DIR)/$(subst rebuild,afterburn,$@)
	-$(Q) rm -f $(BUILD_DIR)/$(subst rebuild,afterburn,$@)/afterburn-wedge
	make $(subst rebuild,build,$@)

clean-wedge: clean-wedge-l4ka-passthru clean-wedge-kaxen-passthru \
  clean-wedge-l4ka clean-wedge-xen
clean-wedge-%:
	-$(Q) (cd $(BUILD_DIR) && rm -rf $(subst clean,afterburn,$@))

######################################################################
# Build empty wedge
######################################################################
help-build::
	@echo "  build-wedge-empty"
help-clean::
	@echo "  clean-wedge-empty"
build-runtime-i386:: build-wedge-empty
clean-runtime-i386:: clean-wedge-empty

build-wedge-empty: receipts/build-afterburn-wedge-empty

receipts/config-afterburn-wedge-empty: ../afterburn-wedge/Makefile
	$(Q) mkdir -p $(BUILD_DIR) receipts
	$(Q) (cd ../afterburn-wedge && make BUILDDIR=$(BUILD_DIR)/afterburn-wedge-empty)
	$(Q) (cd $(BUILD_DIR)/afterburn-wedge-empty && make CMLBATCH_PARAMS='WEDGE_WINNT' batchconfig)
	$(Q) touch $@

receipts/build-afterburn-wedge-empty: receipts/config-afterburn-wedge-empty
	@echo Building the empty Afterburn Wedge in $(BUILD_DIR)/afterburn-wedge-empty
	$(Q) (cd $(BUILD_DIR)/afterburn-wedge-empty && make -s macros)
	$(Q) touch $@

clean-wedge-empty:
	-$(Q) (cd $(BUILD_DIR) && rm -rf afterburn-wedge-empty)
	-$(Q) rm -f receipts/config-afterburn-wedge-empty
	-$(Q) rm -f receipts/build-afterburn-wedge-empty

######################################################################
# Build Xen 3
######################################################################
help-build::
	@echo "  build-xen-3"
help-clean::
	@echo "  clean-xen-3"

build-xen-3: $(BOOT_DIR)/xen-$(VERSION_xen-3).gz

$(BOOT_DIR)/xen-$(VERSION_xen-3).gz: $(SRCDIR)/$(UNPACK_xen-3)/xen/xen.gz
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

$(SRCDIR)/$(UNPACK_xen-3)/xen/xen.gz: receipts/patch-xen-$(VERSION_xen-3)
	@echo Building $@
	$(Q) (cd $(SRCDIR)/$(UNPACK_xen-3) && make -s xen $(OPT_xen))

clean-xen-3:
	-$(Q) (cd $(SRCDIR)/$(UNPACK_xen-3)/xen && make clean $(OPT_xen))

######################################################################
# Build Xen 2.0
######################################################################
help-build::
	@echo "  build-xen-2.0"
help-clean::
	@echo "  clean-xen-2.0"
build-runtime-i386:: build-xen-2.0
build-vmi:: build-xen-2.0
clean-runtime-i386:: clean-xen-2.0

build-xen-2.0: $(BOOT_DIR)/xen-$(VERSION_xen-2).gz

ifdef TOOLPREFIX
OPT_xen-2.0.2-i386 += TARGET_ARCH=x86 TARGET_SUBARCH=x86_32 CC="i386-elf-gcc -D__linux__" LD=i386-elf-ld OBJCOPY=i386-elf-objcopy
OPT_xen-2.0.6-i386 += COMPILE_ARCH=x86_32 CC="i386-elf-gcc -D__linux__ -DVERBOSE" LD=i386-elf-ld OBJCOPY=i386-elf-objcopy
OPT_xen += $(OPT_xen-$(VERSION_xen-2)-$(TARGET_ARCH))
else
#OPT_xen += verbose=y
endif

$(BOOT_DIR)/xen-$(VERSION_xen-2).gz: $(SRCDIR)/$(UNPACK_xen-2)/xen/xen.gz
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

$(SRCDIR)/$(UNPACK_xen-2)/xen/xen.gz: receipts/patch-xen-$(VERSION_xen-2)
	@echo Building $@
	$(Q) if [ -d $(SRCDIR)/$(UNPACK_xen-2)/xen/figlet ] ; then cd $(SRCDIR)/$(UNPACK_xen-2)/xen/figlet && make -s ; else cd $(SRCDIR)/$(UNPACK_xen-2)/xen/tools/figlet && make -s ; fi
	$(Q) (cd $(SRCDIR)/$(UNPACK_xen-2) && make -s xen $(OPT_xen))

clean-xen-2.0:
	-$(Q) (cd $(SRCDIR)/$(UNPACK_xen-2)/xen && make -s clean $(OPT_xen))


######################################################################
# Build Xen 2.0 tools
######################################################################
help-build::
	@echo "  build-xen-2.0-tools"
help-clean::
	@echo "  clean-xen-2.0-tools"

build-xen-2.0-tools: $(INSTALL_ROOT)/xen/xen-tools-$(VERSION_xen-2).tgz

OPT_xen_tools-2.0.2= prefix=$(INSTALL_ROOT)/xen/$(VERSION_xen-2) dist=yes install
OPT_xen_tools-2.0.7= DESTDIR=$(INSTALL_ROOT)/xen/$(VERSION_xen-2) install

$(INSTALL_ROOT)/xen/xen-tools-$(VERSION_xen-2).tgz:
	@mkdir -p $(INSTALL_ROOT)/xen/$(VERSION_xen-2)
	@echo Building the Xen 2.0 tools in $(SRCDIR)/$(UNPACK_xen-2)
	@echo and installing to $(INSTALL_ROOT)/xen/$(VERSION_xen-2)
	$(Q) (cd $(SRCDIR)/$(UNPACK_xen-2)/tools && make -s $(OPT_xen_tools-$(VERSION_xen-2)))
	$(Q) (cd $(INSTALL_ROOT)/xen/$(VERSION_xen-2) && tar zcf ../xen-tools-$(VERSION_xen-2).tgz .)

clean-xen-2.0-tools:
	-$(Q) (cd $(SRCDIR)/$(UNPACK_xen-2)/tools && make -s clean)
	-$(Q) rm -f $(INSTALL_ROOT)/xen/xen-tools-$(VERSION_xen-2).tgz
	-$(Q) rm -f $(INSTALL_ROOT)/xen/$(VERSION_xen-2)

######################################################################
# Build rHype
######################################################################

ifdef TOOLPREFIX
OPTS_rhype += --host=$(TARGET_ARCH)-elf --with-host-prefix=$(TOOLPREFIX)
endif
OPTS_rhype-i386 += --with-platform=x86 --with-serial-baudrate=115200
ifdef VGA
OPTS_rhype-i386 += --enable-vga
else
OPTS_rhype-i386 += --with-boot-console-dev=uart
endif

build-rhype: $(BOOT_DIR)/rhype.boot

$(BOOT_DIR)/rhype.boot: $(BUILD_DIR)/rhype/hype.boot
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

$(BUILD_DIR)/rhype/config.status: receipts/cvs-rhype
	@echo Configuring rHype
	$(Q) mkdir -p $(BUILD_DIR)/rhype
	$(Q) (cd $(BUILD_DIR)/rhype && $(SRCDIR)/rhype/configure --prefix=$(INSTALL_ROOT) $(OPTS_rhype) $(OPTS_rhype-$(TARGET_ARCH)) --with-copt="-finline-limit=999999999")

$(BUILD_DIR)/rhype/hype.boot: $(BUILD_DIR)/rhype/config.status
	@echo Building rHype
	$(Q) (cd $(BUILD_DIR)/rhype && make -s)

clean-rhype:
	-$(Q) (cd $(BUILD_DIR) && rm -rf rhype)

######################################################################
# Build Linux 2.4
######################################################################
help-build::
	@echo "  build-linux-2.4"
help-clean::
	@echo "  clean-linux-2.4"

build-linux-2.4: \
  $(BOOT_DIR)/vmlinuz-$(VERSION_linux-2.4) \
  $(BOOT_DIR)/bzImage-$(VERSION_linux-2.4)

.PRECIOUS: $(SRCDIR)/$(UNPACK_linux-2.4)/vmlinuz
$(SRCDIR)/$(UNPACK_linux-2.4)/vmlinuz: $(SRCDIR)/$(UNPACK_linux-2.4)/vmlinux
	$(Q) strip -o $(<D)/vmlinux.stripped $(<D)/vmlinux
	$(Q) gzip -fc $(<D)/vmlinux.stripped > $(<D)/vmlinuz

$(BOOT_DIR)/vmlinuz-$(VERSION_linux-2.4): $(SRCDIR)/$(UNPACK_linux-2.4)/vmlinuz
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

$(BOOT_DIR)/bzImage-$(VERSION_linux-2.4): $(SRCDIR)/$(UNPACK_linux-2.4)/arch/i386/boot/bzImage
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

.PRECIOUS: $(SRCDIR)/$(UNPACK_linux-2.4)/.config
$(SRCDIR)/$(UNPACK_linux-2.4)/.config: ../afterburn-wedge/doc/linux-2.4/dot-config
	$(Q) mkdir -p $(@D)
	$(Q) cat $< | sed -e '{ s/CONFIG_AFTERBURN_DIR=.*/CONFIG_AFTERBURN_DIR="$(subst /,\/,$(BUILD_DIR)/afterburn-wedge-empty)"/g ; }' > $@
	$(Q) (cd $(SRCDIR)/$(UNPACK_linux-2.4) && make -s oldconfig && make -s dep)

.PRECIOUS: $(SRCDIR)/$(UNPACK_linux-2.4)/vmlinux $(SRCDIR)/$(UNPACK_linux-2.4)/arch/i386/boot/bzImage
$(SRCDIR)/$(UNPACK_linux-2.4)/vmlinux \
$(SRCDIR)/$(UNPACK_linux-2.4)/arch/i386/boot/bzImage: \
		receipts/build-afterburn-wedge-empty \
		receipts/patch-linux-$(VERSION_linux-2.4) \
		$(SRCDIR)/$(UNPACK_linux-2.4)/.config
	@echo Building Linux in $(SRCDIR)/$(UNPACK_linux-2.4)
	$(Q) mkdir -p $(INSTALL_ROOT)
	$(Q) (cd $(SRCDIR)/$(UNPACK_linux-2.4) && make -s bzImage)

rebuild-linux-2.4:
	-$(Q) rm -f $(BOOT_DIR)/$(subst rebuild-linux-2.4,bzImage-$(VERSION_linux-2.4),$@)
	-$(Q) rm -f $(BOOT_DIR)/$(subst rebuild-linux-2.4,vmlinuz-$(VERSION_linux-2.4),$@)
	-$(Q) rm -f $(SRCDIR)/$(subst rebuild-linux-2.4,$(UNPACK_linux-2.4),$@)/vmlinux
	-$(Q) rm -f $(SRCDIR)/$(subst rebuild-linux-2.4,$(UNPACK_linux-2.4),$@)/vmlinux.stripped
	-$(Q) rm -f $(SRCDIR)/$(subst rebuild-linux-2.4,$(UNPACK_linux-2.4),$@)/vmlinuz
	-$(Q) rm -f $(SRCDIR)/$(subst rebuild-linux-2.4,$(UNPACK_linux-2.4),$@)/arch/i386/boot/bzImage
	make $(subst rebuild,build,$@)

######################################################################
# Build Linux 2.6 with VMI
######################################################################
help-build::
	@echo "  build-linux-2.6-vmi type=<driver|guest>"
help-clean::
	@echo "  clean-linux-2.6-vmi"
build-vmi:: build-linux-2.6-vmi

ifneq ($(type),)
build-linux-2.6-vmi: \
  $(BOOT_DIR)/vmlinuz-$(VERSION_linux-2.6-vmi)-vmi-$(type) \
  $(BOOT_DIR)/bzImage-$(VERSION_linux-2.6-vmi)-vmi-$(type) \
  $(modules_linux-2.6-vmi-$(type))
endif


.PRECIOUS: $(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-%/vmlinuz
$(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-%/vmlinuz: $(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-%/vmlinux
	$(Q) strip -o $(<D)/vmlinux.stripped $(<D)/vmlinux
	$(Q) gzip -fc $(<D)/vmlinux.stripped > $(<D)/vmlinuz

$(BOOT_DIR)/vmlinuz-$(VERSION_linux-2.6-vmi)-vmi-%: $(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-%/vmlinuz
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

$(BOOT_DIR)/bzImage-$(VERSION_linux-2.6-vmi)-vmi-%: $(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-%/arch/i386/boot/bzImage
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

.PRECIOUS: $(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-%/.config
$(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-%/.config: ../afterburn-wedge/doc/linux-2.6/dot-config-$(VERSION_linux-2.6-vmi)-vmi-%
	$(Q) mkdir -p $(@D)
	$(Q) cat $< | sed -e '{ s/CONFIG_LOCALVERSION=.*/CONFIG_LOCALVERSION="-vmi-$(type)"/g ; }' > $@

.PRECIOUS: $(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-%/vmlinux $(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-%/arch/i386/boot/bzImage
$(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-%/vmlinux \
$(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-%/arch/i386/boot/bzImage: \
		patch-linux-2.6-vmi \
		$(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-%/.config
	@echo Building Linux in $(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-$(type)
	$(Q) mkdir -p $(INSTALL_ROOT)
	$(Q) (cd $(SRCDIR)/$(UNPACK_linux-2.6-vmi) && make O=$(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-$(type) $(OPTS_linux-2.6))
	$(Q) if grep CONFIG_MODULES=y $(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-$(type)/.config ; then cd $(SRCDIR)/$(UNPACK_linux-2.6-vmi) && make O=$(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-$(type) $(OPTS_linux-2.6) modules_install ; fi

ifneq ($(type),)
.PHONY: reconfig-linux-2.6-vmi uninstall-linux-2.6-vmi clean-linux-2.6-vmi rebuild-linux-2.6-vmi

reconfig-linux-2.6-vmi:
	$(Q) (cd $(SRCDIR)/$(UNPACK_linux-2.6-vmi) && make O=$(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-$(type) $(OPTS_linux-2.6) menuconfig)

uninstall-linux-2.6-vmi:
	-$(Q) rm -f $(BOOT_DIR)/bzImage-$(VERSION_linux-2.6-vmi)-vmi-$(type)
	-$(Q) rm -f $(BOOT_DIR)/vmlinuz-$(VERSION_linux-2.6-vmi)-vmi-$(type)
ifneq ($(modules_linux-2.6-vmi-$(type)),)
	-$(Q) rm -f $(modules_linux-2.6-vmi-$(type))
endif

clean-linux-2.6-vmi:
	-$(Q) rm -f $(addprefix $(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-$(type)/,vmlinux vmlinux.stripped vmlinuz arch/i386/boot/bzImage)
	-$(Q) (cd $(SRCDIR)/$(UNPACK_linux-2.6-vmi) && make clean O=$(BUILD_DIR)/$(UNPACK_linux-2.6-vmi)-$(type) $(OPTS_linux-2.6))

rebuild-linux-2.6-vmi: uninstall-linux-2.6-vmi clean-linux-2.6-vmi build-linux-2.6-vmi
endif

######################################################################
# Build Linux 2.6
######################################################################
help-build::
	@echo "  build-linux-2.6 type=<driver|qemu|guest>"
help-clean::
	@echo "  clean-linux-2.6"
build-runtime-i386:: build-linux-2.6
clean-runtime-i386:: clean-linux-2.6

ifneq ($(type),)
build-linux-2.6: \
  $(BOOT_DIR)/vmlinuz-$(VERSION_linux-2.6)-$(type) \
  $(BOOT_DIR)/bzImage-$(VERSION_linux-2.6)-$(type) \
  $(modules_linux-2.6-$(type))
endif

OPTS_linux-2.6 += INSTALL_MOD_PATH=$(INSTALL_ROOT)
ifdef TOOLPREFIX
OPTS_linux-2.6 += ARCH=$(TARGET_ARCH) CROSS_COMPILE=$(TOOLPREFIX)
endif
ifeq ($(HOST_PLATFORM),Darwin)
OPTS_linux-2.6 += HOSTCC="cc -no-cpp-precomp"
endif

.PRECIOUS: $(BUILD_DIR)/$(UNPACK_linux-2.6)-%/vmlinuz
$(BUILD_DIR)/$(UNPACK_linux-2.6)-%/vmlinuz: $(BUILD_DIR)/$(UNPACK_linux-2.6)-%/vmlinux
	$(Q) strip -o $(<D)/vmlinux.stripped $(<D)/vmlinux
	$(Q) gzip -fc $(<D)/vmlinux.stripped > $(<D)/vmlinuz

$(BOOT_DIR)/vmlinuz-$(VERSION_linux-2.6)-%: $(BUILD_DIR)/$(UNPACK_linux-2.6)-%/vmlinuz
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

$(BOOT_DIR)/bzImage-$(VERSION_linux-2.6)-%: $(BUILD_DIR)/$(UNPACK_linux-2.6)-%/arch/i386/boot/bzImage
	$(Q) mkdir -p $(BOOT_DIR)
	$(Q) cp $< $@
	$(Q) $(BOOT_INSTALL_CMD)

.PRECIOUS: $(BUILD_DIR)/$(UNPACK_linux-2.6)-%/.config
$(BUILD_DIR)/$(UNPACK_linux-2.6)-%/.config: ../afterburn-wedge/doc/linux-2.6/dot-config-$(VERSION_linux-2.6)-%
	$(Q) mkdir -p $(@D)
	$(Q) cat $< | sed -e '{ s/CONFIG_AFTERBURN_DIR=.*/CONFIG_AFTERBURN_DIR="$(subst /,\/,$(BUILD_DIR)/afterburn-wedge-empty)"/g ; }' | sed -e '{ s/CONFIG_LOCALVERSION=.*/CONFIG_LOCALVERSION="-$(type)"/g ; }' > $@

.PRECIOUS: $(BUILD_DIR)/$(UNPACK_linux-2.6)-%/vmlinux $(BUILD_DIR)/$(UNPACK_linux-2.6)-%/arch/i386/boot/bzImage
$(BUILD_DIR)/$(UNPACK_linux-2.6)-%/vmlinux \
$(BUILD_DIR)/$(UNPACK_linux-2.6)-%/arch/i386/boot/bzImage: \
		patch-linux-2.6 \
		receipts/build-afterburn-wedge-empty \
		$(BUILD_DIR)/$(UNPACK_linux-2.6)-%/.config
	@echo Building Linux in $(BUILD_DIR)/$(UNPACK_linux-2.6)-$(type)
	$(Q) mkdir -p $(INSTALL_ROOT)
	$(Q) (cd $(SRCDIR)/$(UNPACK_linux-2.6) && make O=$(BUILD_DIR)/$(UNPACK_linux-2.6)-$(type) $(OPTS_linux-2.6))
	$(Q) if grep CONFIG_MODULES=y $(BUILD_DIR)/$(UNPACK_linux-2.6)-$(type)/.config ; then cd $(SRCDIR)/$(UNPACK_linux-2.6) && make O=$(BUILD_DIR)/$(UNPACK_linux-2.6)-$(type) $(OPTS_linux-2.6) modules_install ; fi

ifneq ($(type),)
.PHONY: reconfig-linux-2.6 uninstall-linux-2.6 clean-linux-2.6 rebuild-linux-2.6

reconfig-linux-2.6:
	$(Q) (cd $(SRCDIR)/$(UNPACK_linux-2.6) && make O=$(BUILD_DIR)/$(UNPACK_linux-2.6)-$(type) $(OPTS_linux-2.6) menuconfig)

uninstall-linux-2.6:
	-$(Q) rm -f $(BOOT_DIR)/bzImage-$(VERSION_linux-2.6)-$(type)
	-$(Q) rm -f $(BOOT_DIR)/vmlinuz-$(VERSION_linux-2.6)-$(type)
ifneq ($(modules_linux-2.6-$(type)),)
	-$(Q) rm -f $(modules_linux-2.6-$(type))
endif

clean-linux-2.6:
	-$(Q) rm -f $(addprefix $(BUILD_DIR)/$(UNPACK_linux-2.6)-$(type)/,vmlinux vmlinux.stripped vmlinuz arch/i386/boot/bzImage)
	-$(Q) (cd $(SRCDIR)/$(UNPACK_linux-2.6) && make clean O=$(BUILD_DIR)/$(UNPACK_linux-2.6)-$(type) $(OPTS_linux-2.6))

rebuild-linux-2.6: uninstall-linux-2.6 clean-linux-2.6 build-linux-2.6
endif

######################################################################
# Darwin's XNU
######################################################################

receipts/extract-xnu-$(VERSION_xnu): $(ARCHIVE_DIR)/$(TARBALL_xnu)
	@echo "Extracting Darwin's XNU to $(SRCDIR)/$(UNPACK_xnu)"
	$(Q) mkdir -p $(SRCDIR) receipts
	$(Q) (cd $(SRCDIR) && tar zxf $<)
	$(Q) touch $@

######################################################################
# Build a legitimate Python, since Linux distributions try to be clever 
# and screw-up standard packages.  The point of distributions is to
# avoid these hassles ...
######################################################################
help-build::
	@echo "  build-python"
help-clean::
	@echo "  clean-python"

build-python: receipts/install-python-$(VERSION_python)

receipts/unpack-python-$(VERSION_python): $(ARCHIVE_DIR)/$(TARBALL_python)
	@echo Extracting Python in $(SRCDIR)/$(UNPACK_python)
	$(Q) mkdir -p $(SRCDIR)
	$(Q) (cd $(SRCDIR) && tar zxf $<)
	$(Q) touch $@

receipts/install-python-$(VERSION_python): receipts/unpack-python-$(VERSION_python)
	@echo Building Python in $(SRCDIR)/$(UNPACK_python)
	$(Q) (cd $(SRCDIR)/$(UNPACK_python) && ./configure --prefix=$(INSTALL_ROOT) --enable-shared)
	$(Q) (cd $(SRCDIR)/$(UNPACK_python) && make -s install)
	$(Q) touch $@

clean-python:
	-$(Q) (cd $(SRCDIR) && rm -rf $(UNPACK_python))
	-$(Q) rm -f receipts/install-python-$(VERSION_python)

######################################################################
#  Build Mercurial (Hg)
######################################################################

help-build::
	@echo "  build-hg"

build-hg: receipts/install-hg-$(VERSION_hg)

receipts/install-hg-$(VERSION_hg): receipts/unpack-hg-$(VERSION_hg)
	@echo "Installing Mercurial (Hg) in $(INSTALL_ROOT)"
	$(Q) mkdir -p $(INSTALL_ROOT)
	$(Q) (cd $(SRCDIR)/$(UNPACK_hg) && python setup.py install --home=$(INSTALL_ROOT))
	$(Q) touch $@

receipts/unpack-hg-$(VERSION_hg): $(ARCHIVE_DIR)/$(TARBALL_hg)
	@echo "Extracting Mercurial (Hg) in $(SRCDIR)/$(UNPACK_hg)"
	$(Q) mkdir -p $(SRCDIR)
	$(Q) (cd $(SRCDIR) && tar zxf $<)
	$(Q) touch $@

######################################################################
# afterburn.lst
######################################################################
help-boot::
	@echo "  create-grub-menu"

build-runtime-i386:: create-grub-menu
build-vmi:: create-grub-menu


$(BOOT_DIR)/menu.lst:
	@echo "serial --unit=0 --speed=115200" >> $@
	@echo "terminal serial console" >> $@
	@echo "default 0" >> $@
	@echo >> $@
	@echo "title = Afterburn Stuff" >> $@
	@echo "configfile = $(GRUB_BOOT)/afterburn.lst" >> $@
	@echo >> $@


.PHONY: create-grub-menu create-grub-header create-grub-body
create-grub-menu: g=$(BOOT_DIR)/afterburn.lst
create-grub-menu: create-grub-header create-grub-body
create-grub-header::
	@echo "# Automatically generated file.  Changes will be lost." > $(g)
create-grub-body:: \
  grub-marzipan-device-p4 grub-marzipan-device-p3 grub-marzipan-device-qemu \
  grub-ping-pong-p4 grub-ping-pong-p3 grub-ping-pong-qemu
	@echo >> $(g)
	@echo "title = Afterburnt Linux on raw hardware" >> $(g)
	@echo "kernel = $(GRUB_BOOT)/bzImage-$(VERSION_linux-2.6)-driver $(LINUX_BOOT_PARAMS)" >> $(g)
	@echo >> $(g)
	@echo "title = Afterburnt Linux on KaXen" >> $(g)
	@echo "kernel = $(GRUB_BOOT)/xen-$(VERSION_xen-2).gz dom0_mem=$(VMSIZE) com1=115200,8n1 ignorebiostables=1 noacpi=1 nosmp=1 noht=1 noapic=1" >> $(g)
	@echo "module = $(GRUB_BOOT)/afterburn-wedge-kaxen-passthru $(LINUX_BOOT_PARAMS)" >> $(g)
	@echo "module = $(GRUB_BOOT)/vmlinuz-$(VERSION_linux-2.6)-driver" >> $(g)
	@echo >> $(g)
	@echo "title = VMI Linux on KaXen" >> $(g)
	@echo "kernel = $(GRUB_BOOT)/xen-$(VERSION_xen-2).gz dom0_mem=$(VMSIZE) com1=115200,8n1 ignorebiostables=1 noacpi=1 nosmp=1 noht=1 noapic=1" >> $(g)
	@echo "module = $(GRUB_BOOT)/afterburn-wedge-kaxen-vmi-passthru $(LINUX_BOOT_PARAMS) mem=nopentium" >> $(g)
	@echo "module = $(GRUB_BOOT)/vmlinuz-$(VERSION_linux-2.6-vmi)-vmi-driver" >> $(g)
	@echo >> $(g)
	@echo "title = VMI Linux on raw hardware" >> $(g)
	@echo "kernel = $(GRUB_BOOT)/bzImage-$(VERSION_linux-2.6-vmi)-vmi-driver $(LINUX_BOOT_PARAMS)" >> $(g)
	@echo >> $(g)


grub-marzipan-device-%:
	@echo >> $(g)
	@echo "title = $(subst grub-marzipan-device-,,$@): Afterburnt Linux on Marzipan with Device Pass-Through" >> $(g)
	@echo "kernel = $(GRUB_BOOT)/kickstart" >> $(g)
	@echo "module = $(GRUB_BOOT)/pistachio-$(subst grub-marzipan-device-,,$@)" >> $(g)
	@echo "module = $(GRUB_BOOT)/sigma0" >> $(g)
	@echo "module = $(GRUB_BOOT)/marzipan" >> $(g)
	@echo "module = $(GRUB_BOOT)/afterburn-wedge-l4ka-passthru vmstart vmsize=$(VMSIZE)K wedgeinstall=16M wedgesize=16M" >> $(g)
	@echo "module = $(GRUB_BOOT)/vmlinuz-$(VERSION_linux-2.6)-driver $(LINUX_BOOT_PARAMS)" >> $(g)

grub-marzipan-%:
	@echo >> $(g)
	@echo "title = $(subst grub-marzipan-,,$@): Afterburnt Linux on Marzipan" >> $(g)
	@echo "kernel = $(GRUB_BOOT)/kickstart" >> $(g)
	@echo "module = $(GRUB_BOOT)/pistachio-$(subst grub-marzipan-,,$@)" >> $(g)
	@echo "module = $(GRUB_BOOT)/sigma0" >> $(g)
	@echo "module = $(GRUB_BOOT)/marzipan" >> $(g)
	@echo "module = $(GRUB_BOOT)/afterburn-wedge-l4ka vmstart vmsize=$(VMSIZE)K wedgeinstall=16M wedgesize=16M" >> $(g)
	@echo "module = $(GRUB_BOOT)/vmlinuz-$(VERSION_linux-2.6)-guest $(LINUX_BOOT_PARAMS)" >> $(g)


grub-ping-pong-%:
	@echo >> $(g)
	@echo "title = $(subst grub-ping-pong-,,$@): Ping-Pong on Pistachio" >> $(g)
	@echo "kernel = $(GRUB_BOOT)/kickstart" >> $(g)
	@echo "module = $(GRUB_BOOT)/pistachio-$(subst grub-ping-pong-,,$@)" >> $(g)
	@echo "module = $(GRUB_BOOT)/sigma0" >> $(g)
	@echo "module = $(GRUB_BOOT)/pingpong" >> $(g)
	

######################################################################
# Build Qemu
######################################################################
help-build::
	@echo "  build-qemu"
help-clean::
	@echo "  clean-qemu"
build-runtime:: build-qemu
build-vmi:: build-qemu
clean-runtime:: clean-qemu

## QEMU options, looked-up by the $(HOST_PLATFORM) variable.
OPTS_qemu-Darwin = --enable-cocoa --cc=gcc-3.3

build-qemu: receipts/install-qemu-$(VERSION_qemu)

receipts/install-qemu-$(VERSION_qemu): receipts/patch-qemu-$(VERSION_qemu)
	@echo Building QEMU in $(SRCDIR)/$(UNPACK_qemu)
	$(Q) mkdir -p $(INSTALL_ROOT) receipts
	$(Q) (cd $(SRCDIR)/$(UNPACK_qemu) && ./configure --prefix=$(INSTALL_ROOT) $(OPTS_qemu-$(HOST_PLATFORM)) --target-list=i386-softmmu)
	$(Q) (cd $(SRCDIR)/$(UNPACK_qemu) && make -s)
	$(Q) (cd $(SRCDIR)/$(UNPACK_qemu) && make -s install)
	$(Q) touch $@

clean-qemu:
	-$(Q) (cd $(SRCDIR) && rm -rf $(UNPACK_qemu))
	-$(Q) rm -f receipts/install-qemu-$(VERSION_qemu)
	-$(Q) rm -f receipts/patch-qemu-$(VERSION_qemu)

######################################################################
# Run Qemu
######################################################################

OPTS_run-qemu-Darwin = -nographic -serial stdio -monitor stdio
OPTS_run-qemu-Linux = -nographic

$(BOOT_DIR)/qemu-afterburner-floppy.img: $(ARCHIVE_DIR)/qemu-afterburner-floppy.img.gz
	$(Q) gzip -cd $< > $@

.PHONY: run-qemu run-qemu-powerpc
run-qemu: $(BOOT_DIR)/qemu-afterburner-floppy.img $(BOOT_DIR)/menu.lst receipts/install-qemu-$(VERSION_qemu)
	$(Q) cd $(BOOT_DIR) && qemu $(OPTS_run-qemu-$(HOST_PLATFORM)) -pci -user-net -m 196 -tftp /tftpboot -fda $(BOOT_DIR)/qemu-afterburner-floppy.img -boot a -redir tcp:3022:10.0.2.10:22

run-qemu-debian: $(BOOT_DIR)/debian-small.img $(BOOT_DIR)/menu.lst $(BOOT_DIR)/qemu-afterburner-floppy.img receipts/install-qemu-$(VERSION_qemu)
	$(Q) cd $(BOOT_DIR) && qemu $(OPTS_run-qemu-$(HOST_PLATFORM)) -pci -user-net -m 128 -tftp /tftpboot -fda $(BOOT_DIR)/qemu-afterburner-floppy.img -boot a -hda $(BOOT_DIR)/debian-small.img -snapshot -redir tcp:3022:10.0.2.10:22


run-psim: $(BOOT_DIR)/psim.tree $(BOOT_DIR)/ofppc-loader
	$(Q) powerpc-unknown-eabi-run -e chirp -m 604 -f $(BOOT_DIR)/psim.tree $(BOOT_DIR)/ofppc-loader

