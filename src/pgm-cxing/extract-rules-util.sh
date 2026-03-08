#!/bin/sh

implexpr="$(
sed -En 's/^.*theRule == ([[:alnum:]_]+).*$/-e \1/p' cxing-interp-func-exec.*.bits.h
sed -En 's/^.*entRule[(].*[)] == ([[:alnum:]_]+).*$/-e \1/p' cxing-interp-module.c
)"

grep -E ^void cxing-grammar.h | grep -Ev $implexpr -e degenerate
