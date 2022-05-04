ifeq "$(MAKE_VERSION)" ""
  $(error GNU Make 4.0 or later is required)
endif

ifneq ($(shell expr '$(MAKE_VERSION)' : '[0-3]\.'),0)
  $(error GNU Make 4.0 or later is required)
endif

REQUIRE = $(if $(filter $(1), $(.FEATURES)),, $(error $(REQERROR)))
REQERROR = Required feature "$(strip $(1))" not supported by $(MAKE)

$(call REQUIRE, target-specific)
$(call REQUIRE, else-if)
$(call REQUIRE, order-only)
