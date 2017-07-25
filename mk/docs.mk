man := Documentation/$(PROGRAM).1 Documentation/$(PROGRAM)-syntax.7

quiet_cmd_ttman = MAN    $@

cmd_ttman = \
    $(SED) -e s/%MAN%/$(shell echo $@ | $(SED) 's:.*/\([^.]*\)\..*:\1:' | tr a-z A-Z)/g \
    -e s/%PROGRAM%/$(PROGRAM)/g \
    < $< | Documentation/ttman$(X) > $@

man: $(man)

$(man): Documentation/ttman$(X)

%.1: %.txt
	$(call cmd,ttman)

%.7: %.txt
	$(call cmd,ttman)

Documentation/ttman.o: Documentation/ttman.c
	$(call cmd,host_cc)

Documentation/ttman$(X): Documentation/ttman.o
	$(call cmd,host_ld,)


CLEANFILES += $(man) Documentation/*.o Documentation/ttman$(X)
.PHONY: man
