
afterburn_dir := $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))
ifneq ($(firstword $(subst /,/ ,$(afterburn_dir))),/)
  afterburn_dir := $(CURDIR)/$(afterburn_dir)
endif

include $(afterburn_dir)/Mk/Makefile.top

