SHELL=/bin/sh
#
# Makefile for NeoCD/SDL
#

all:
	@echo "Please specify target:"
	@echo "(For example, type \"make linux\" for a Linux system.)"
	@(cd makefiles && ls -C Makefile* | sed -e 's/Makefile.//g')

.DEFAULT:
	@($(MAKE) -f makefiles/Makefile.$@)

clean:
	@($(MAKE) -f src/Makefile.common clean)
