#!/bin/sh

util="../../utils/grammar2rules.py"
lexheader="../langlex/langlex-c.h"
export PYTHONPATH="$(dirname "$0")"
"$util" decl ppexpr "$lexheader" < grammar-syntax.txt | tr -d '\r' > ppexpr-grammar.h
"$util" def  ppexpr "$lexheader" < grammar-syntax.txt | tr -d '\r' > ppexpr-grammar.c

mode=s

"$util" decl ppexpr "$lexheader" < grammar-syntax.txt | tr -d '\r' |
    sed -E 's/^void \*/void *pp_/g' > ppexpr-grammar.h

"$util" def  ppexpr "$lexheader" < grammar-syntax.txt | tr -d '\r' |
    sed -E 's/^void \*/void *pp_/g' | while read l ; do

    case "$l" in
        *NULL,*)
            mode=s
            ;;
    esac

    if [ X"$mode" = Xt ]
    then echo "$l" | sed -E 's/^ */  pp_/g'
    else echo "$l" ; fi

    case "$l" in
        *lalr_rule_t*ppexpr_grammar_rules*)
            mode=t
            ;;
    esac
done > ppexpr-grammar.c
