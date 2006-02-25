
Q ?= @

all:

## Extract the afterburn directory from the path to this Makefile.
mk_dir := $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))
afterburn_dir ?= $(mk_dir)/..

include $(mk_dir)/Makefile.cml
include config.out

ifeq ($(cfg_cml),y)

install-y =
install-n =

cfg_common_dir :=  $(subst ",,$(cfg_common_dir))
cfg_build_dir :=   $(subst ",,$(cfg_build_dir))
cfg_src_dir :=     $(subst ",,$(cfg_src_dir))
cfg_usr_dir :=     $(subst ",,$(cfg_usr_dir))
cfg_boot_dir :=    $(subst ",,$(cfg_boot_dir))
cfg_archive_dir := $(subst ",,$(cfg_archive_dir))

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
include $(mk_dir)/rules/idl4
include $(mk_dir)/rules/antlr
include $(mk_dir)/rules/afterburner
include $(mk_dir)/rules/xen

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

.PHONY: variables makefiles
variables:
	@echo "$(.VARIABLES)"
makefiles:
	@echo "$(MAKEFILE_LIST)"

define do_retrieve
  if [ ! -f "$(cfg_archive_dir)/$(tarball_$1)" ] ; then \
    mkdir -p $(cfg_archive_dir) && \
    echo "Retrieving the remote archive for $1" && \
    trap "curl --output $(cfg_archive_dir)/$(tarball_$1) $(url_$1) && exit 0" ERR && wget -O $(cfg_archive_dir)/$(tarball_$1) $(url_$1) ; \
  fi
endef

define do_unpack
  if [ ! -d "$(cfg_src_dir)/$(unpack_$1)" ] ; then \
    $(call do_retrieve,$1) && \
    mkdir -p $(cfg_src_dir)/$(unpack_$1) && \
    (cd $(cfg_src_dir)/$(unpack_$1) && tar --strip-components=1 $(call untar_opts,$(tarball_$1)) $(cfg_archive_dir)/$(tarball_$1)) ; \
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

endif

