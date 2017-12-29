REQUIRE = $(if $(filter $(1), $(.FEATURES)),, $(error $(REQERROR)))
REQERROR = Required feature "$(strip $(1))" not supported by $(MAKE)

$(call REQUIRE, target-specific)
$(call REQUIRE, else-if)
$(call REQUIRE, order-only)

ifeq "$(MAKE_VERSION)" "3.81"
  $(warning Disabling build optimization to work around a bug in GNU Make 3.81)
else
  make-lazy = $(eval $1 = $$(eval $1 := $(value $(1)))$$($1))
endif
