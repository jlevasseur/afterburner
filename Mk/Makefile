
Q ?= @

all:

include Makefile.cml
-include config.out

install-y =
install-n =

cfg_common_dir :=  $(subst ",,$(cfg_common_dir))
cfg_build_dir :=   $(subst ",,$(cfg_build_dir))
cfg_src_dir :=     $(subst ",,$(cfg_src_dir))
cfg_usr_dir :=     $(subst ",,$(cfg_usr_dir))
cfg_boot_dir :=    $(subst ",,$(cfg_boot_dir))
cfg_archive_dir := $(subst ",,$(cfg_archive_dir))

afterburn_dir = $(dir $(CURDIR))
ifeq ($(cfg_common_dir),.)
cfg_common_dir = $(CURDIR)
endif
ifneq ($(cfg_common_dir),)
cfg_build_dir := $(cfg_common_dir)/$(cfg_build_dir)
cfg_src_dir := $(cfg_common_dir)/$(cfg_src_dir)
cfg_usr_dir := $(cfg_common_dir)/$(cfg_usr_dir)
cfg_boot_dir := $(cfg_common_dir)/$(cfg_boot_dir)
cfg_archive_dir := $(cfg_common_dir)/$(cfg_archive_dir)
endif

PATH := $(cfg_usr_dir)/bin:$(PATH)
LD_LIBRARY_PATH := $(cfg_usr_dir)/lib:$(LD_LIBRARY_PATH)
export LD_LIBRARY_PATH

.SECONDARY:


##  Include in order of dependencies.
include rules/idl4
include rules/xen

.PHONY: world install
world: $(install-y)
install: $(install-y)


.PHONY: dirs
dirs:
	@echo "Common:   $(cfg_common_dir)"
	@echo "Build:    $(cfg_build_dir)"
	@echo "Source:   $(cfg_src_dir)"
	@echo "Install:  $(cfg_usr_dir)"
	@echo "Boot:     $(cfg_boot_dir)"
	@echo "Archives: $(cfg_archive_dir)"

define do_retrieve
  if [ ! -f "$(cfg_archive_dir)/$(tarball_$1)" ] ; then \
    mkdir -p $(cfg_archive_dir) && \
    echo "Retrieving the remote archive for $1" && \
    trap "curl --output $(cfg_archive_dir)/$(tarball_$1) $(url_$1) && exit 0" ERR && wget -O $(cfg_archive_dir)/$(tarball_$1) $(url_$1) ; \
  fi
endef

untar_.gz  = -zxf
untar_.tgz = -zxf
untar_.bz2 = -jxf
untar_.bgz = -jxf

untar_opts = $(untar_$(suffix $(1)))

receipts/patch-%:
	@echo "Extracting the archive $(tarball_$(name)) to apply a new patch."
	$(Q) if [ ! -f "$(patch-file_$(name))" ] ; then \
	  echo "Missing patch file." ; \
	  exit 1 ; \
	fi
	$(Q) if [ -d $(cfg_src_dir)/$(unpack_$(name)) ] ; then \
	  echo "$(cfg_src_dir)/$(unpack_$(name)) already exists." ; \
	  echo "Please remove it first." ; \
	  exit 1 ; \
        fi
	$(Q) $(call do_retrieve,$(name))
	$(Q) mkdir -p $(cfg_src_dir)/$(unpack_$(name)) receipts
	$(Q) (cd $(cfg_src_dir)/$(unpack_$(name)) && tar --strip-components=1 $(call untar_opts,$(tarball_$(name))) $(cfg_archive_dir)/$(tarball_$(name)))
	@echo "Applying the patch."
	$(Q) (cd $(cfg_src_dir)/$(unpack_$(name)) && patch -Np1 < $(patch-file_$(name)))
	$(Q) touch receipts/patch-$(unpack_$(name))
	@echo "The newly patched project: $(cfg_src_dir)/$(unpack_$(name))"

