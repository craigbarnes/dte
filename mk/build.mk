# build verbosity
ifneq ($(findstring s,$(MAKEFLAGS)),)
	# make -s
	cmd = $(call cmd_$(1),$(2))
else
	ifeq ($(V),1)
		# make V=1
		cmd = @echo "$(call cmd_$(1),$(2))"; $(call cmd_$(1),$(2))
	else
		cmd = @echo "   $(quiet_cmd_$(1))"; $(call cmd_$(1),$(2))
	endif
endif

# avoid random internationalization problems
LC_ALL := C
export LC_ALL

clean =
distclean =

# CFLAGS and LDFLAGS can be set from the command line
BASIC_CFLAGS =
BASIC_LDFLAGS =
BASIC_HOST_CFLAGS =
BASIC_HOST_LDFLAGS =

# following macros have been taken from Kbuild and simplified

quiet_cmd_cc = CC     $@
      cmd_cc = $(CC) $(CFLAGS) $(BASIC_CFLAGS) -c -o $@ $<

quiet_cmd_ld = LD     $@
      cmd_ld = $(LD) $(LDFLAGS) $(BASIC_LDFLAGS) -o $@ $^ $(1)

quiet_cmd_host_cc = HOSTCC $@
      cmd_host_cc = $(HOST_CC) $(HOST_CFLAGS) $(BASIC_HOST_CFLAGS) -c -o $@ $<

quiet_cmd_host_ld = HOSTLD $@
      cmd_host_ld = $(HOST_LD) $(HOST_LDFLAGS) $(BASIC_HOST_LDFLAGS) -o $@ $^ $(1)

try-run = $(shell if ($(1)) >/dev/null 2>&1; then echo "$(2)"; else echo "$(3)"; fi)
cc-option = $(call try-run, $(CC) $(1) -c -x c /dev/null -o /dev/null,$(1),$(2))

# dependency generation macros copied from GIT

ifndef NO_DEPS
CC_CAN_GENERATE_DEPS := $(call try-run,$(CC) -MMD -MP -MF /dev/null -c -x c /dev/null -o /dev/null,y,n)
ifeq ($(CC_CAN_GENERATE_DEPS),n)
NO_DEPS := y
endif
endif

ifndef NO_DEPS
dep_files := $(foreach f,$(OBJECTS),$(dir $f).depend/$(notdir $f).d)
dep_dirs := $(addsuffix .depend,$(sort $(dir $(OBJECTS))))
missing_dep_dirs := $(filter-out $(wildcard $(dep_dirs)),$(dep_dirs))
BASIC_CFLAGS += -MF $(dir $@).depend/$(notdir $@).d -MMD -MP
dep_files_present := $(wildcard $(dep_files))

ifneq ($(dep_files_present),)
include $(dep_files_present)
endif

$(dep_dirs):
	@mkdir -p $@
else
missing_dep_dirs =
endif

%.o: %.c $(missing_dep_dirs)
	$(call cmd,cc)

clean:
	rm -f $(clean)
	rm -rf $(dep_dirs)

distclean: clean
	rm -f $(distclean)

.PHONY: clean distclean

# Treat all targets as secondary (Never delete intermediate targets).
.SECONDARY:
