HASFEATURE = $(filter $(1), $(.FEATURES))
REQERROR   = Required feature "$(strip $(1))" not supported by $(MAKE)
REQUIRE    = $(and $(or $(HASFEATURE), $(error $(REQERROR))),)

$(call REQUIRE, target-specific)
