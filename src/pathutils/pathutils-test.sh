#!/bin/sh

testfunc()
{
    : &&
        test $($exec /foo/bar baz |
                   tee /dev/tty) = /foo/baz &&
        test $($exec //foo//bar/hello// world.txt |
                   tee /dev/tty) = //foo//bar/world.txt &&
        test $($exec /dannyniu///dcc// studio.cnf |
                   tee /dev/tty) = /dannyniu///studio.cnf &&
        test $($exec ./pkg/contrib/README.md ../hi/INSTALL.md |
                   tee /dev/tty) = ./pkg/contrib/../hi/INSTALL.md
        test $($exec /pkg/contrib/README.md ./op/INSTALL.md |
                   tee /dev/tty) = /pkg/contrib/op/INSTALL.md
}

cd "$(dirname "0")"
unitest_sh=../unitest.sh
. $unitest_sh

src="\
pathutils-test.c
pathutils.c
"

arch_family=defaults
srcset="Plain C"

tests_run
