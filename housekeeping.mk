# DannyNiu/NJF, 2024-07-27. Public Domain.

.PHONY: all install uninstall clean distclean

all:;:

install:
	cp build/"${ProductName}" ${bindir}

uninstall:
	rm ${bindir}/"${ProductName}"

clean:
	rm -f ${INPUT_OBJECTS}
