#!/bin/sh

util="../../utils/grammar2rules.py"
lexheader="../langlex/langlex-c.h"
export PYTHONPATH="$(dirname "$0")"
mode=s

"$util" decl c "$lexheader" < c-grammar.txt | tr -d '\r' > c-grammar.h
"$util" def  c "$lexheader" < c-grammar.txt | tr -d '\r' > c-grammar.c
