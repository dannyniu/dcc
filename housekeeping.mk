# DannyNiu/NJF, 2024-07-27. Public Domain.

include common.mk
include objects.mk

include inc-config.mk

.PHONY: all install uninstall clean distclean

all:;:

install:
	cp build/"${ProductName}" ${bindir}

uninstall:
	rm ${bindir}/"${ProductName}"

clean:
	rm -f build/"${ProductName}"
	rm -f ${OBJS_GROUP_ALL} ${OBJS_GROUP_WITH_ADDITION}

distclean: clean
	rm -f inc-*.mk auto/configure[-.]*
