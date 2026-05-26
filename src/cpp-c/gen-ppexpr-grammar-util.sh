#!/bin/sh

util="../../utils/grammar2rules.py"
lexheader="../langlex/langlex-c.h"
export PYTHONPATH="$(dirname "$0")"
"$util" decl ppexpr "$lexheader" < grammar-syntax.txt | tr -d '\r' > ppexpr-grammar.h
"$util" def  ppexpr "$lexheader" < grammar-syntax.txt | tr -d '\r' > ppexpr-grammar.c
