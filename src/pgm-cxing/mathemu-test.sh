#!/bin/sh
# <CXING-check008(2026-04-06)> #

optimize=optimize
testfunc()
{
    $exec
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cxing-src-common.inc
unset src_common
src="\
mathemu-test.c
mathemu-stubs.c
"

arch_family=defaults
srcset="Plain C"

tests_run
