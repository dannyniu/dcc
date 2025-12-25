# DannyNiu/NJF, 2023-07-27. Public Domain.

.PHONY: all install uninstall clean distclean

all:
	${MAKE} -f common.mk -f inc-config.mk \
	-f target-pgm-fpcalc.mk -f build-pgm.mk ${MAKEFLAGS} $@

install uninstall clean:
	${MAKE} -f common.mk -f inc-config.mk \
	-f target-pgm-fpcalc.mk -f housekeeping.mk ${MAKEFLAGS} $@

distclean: clean
	rm -f inc-*.mk auto/configure[-.]*
