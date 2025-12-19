#!/bin/sh

util="../../utils/grammar2rules.py"
lexheader="../langlex/langlex-cxing.h"
export PYTHONPATH="$(dirname "$0")"
"$util" decl cxing "$lexheader" < grammar-syntax.txt > cxing-grammar.h
"$util" def  cxing "$lexheader" < grammar-syntax.txt > cxing-grammar.T.c
