S_FLAG := $(findstring s,$(firstword -$(MAKEFLAGS)))$(filter -s,$(MAKEFLAGS))

ifneq "$(S_FLAG)$(V)" ""
  define cmd =
    $(call cmd_$(1),$(2))
  endef
else
  define cmd =
    @printf ' %-8s %s\n' $(quiet_cmd_$(1))
    @$(call cmd_$(1),$(2))
  endef
endif

# Avoid random internationalization problems
export LC_ALL := C

# CFLAGS and LDFLAGS can be set from the command line
BASIC_CFLAGS =
BASIC_LDFLAGS =
BASIC_HOST_CFLAGS =
BASIC_HOST_LDFLAGS =

quiet_cmd_rc2c = GEN    $@

cmd_rc2c = \
    echo 'static const char *$(1) =' > $@; \
    $(SED) -f mk/rc2c.sed $< >> $@

# The following macros have been taken from Kbuild and simplified

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

# Dependency generation macros copied from GIT

ifndef NO_DEPS
  CC_CAN_GENERATE_DEPS := $(call try-run,$(CC) -MMD -MP -MF /dev/null -c -x c /dev/null -o /dev/null,y,n)
  ifeq "$(CC_CAN_GENERATE_DEPS)" "n"
    NO_DEPS := y
  endif
endif

ifndef NO_DEPS
dep_files := $(foreach f,$(OBJECTS),$(dir $f).depend/$(notdir $f).d)
dep_dirs := $(addsuffix .depend,$(sort $(dir $(OBJECTS))))
missing_dep_dirs := $(filter-out $(wildcard $(dep_dirs)),$(dep_dirs))
BASIC_CFLAGS += -MF $(dir $@).depend/$(notdir $@).d -MMD -MP
dep_files_present := $(wildcard $(dep_files))

ifneq "$(dep_files_present)" ""
include $(dep_files_present)
endif

$(dep_dirs):
	@mkdir -p $@
else
missing_dep_dirs =
endif

%.o: %.c $(missing_dep_dirs)
	$(call cmd,cc)
