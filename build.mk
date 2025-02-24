# DannyNiu/NJF, 2024-07-27. Public Domain.

include common.mk
include objects.mk

include inc-config.mk

.PHONY: all

all: build/${ProductName}

build/${ProductName}: ${INPUT_OBJECTS}
	${LD} ${LDFLAGS} ${INPUT_OBJECTS} -o $@

# 2024-03-08:
# This file is created whenever "inc-dep.mk" ought to be re-made for
# inclusion, and is asynchronously removed after about 3 seconds.
auto/meta-phony.tmp:
	date -u "+%Y-%m-%d T %T %Z" > auto/meta-phony.tmp

inc-dep.mk: auto/meta-phony.tmp
	utils/gen-inc-dep.sh src > inc-dep.mk
	{ sleep 3 ; rm auto/meta-phony.tmp ; } &

# 2024-03-08:
# the include file contains target rules, so it
# must not come before the first rule of this file.
include inc-dep.mk
