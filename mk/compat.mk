REQUIRE = $(if $(filter $(1), $(.FEATURES)),, $(error $(REQERROR)))
REQERROR = Required feature "$(strip $(1))" not supported by $(MAKE)

$(call REQUIRE, target-specific)
$(call REQUIRE, else-if)
$(call REQUIRE, order-only)
