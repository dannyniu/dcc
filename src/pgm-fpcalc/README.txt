# 2025-01-01: This is a minimal calculator program aimed at testing
# expression parsing using the facilities in "lex" and "lalr".

identified-expression
: <identifier>
| <identifier> "(" ")"
| <identifier> "(" additive-expression ")"
| <identifier> "(" additive-expression-list ")"
;

additive-expression-list
: additive-expression, additive-expression
| additive-expression-list, additive-expression
;

primary-expression
: identified-expression
| <literal>
| "(" additive-expression ")"
;

unary-expression
: primary-expression
| ("+"|"-") primary-expression
;

multiplicative-expression
: unary-expression
| multiplicative-expression ("*"|"/") unary-expression
;

additive-expression
: multiplicative-expression
| additive-expression ("+"|"-") multiplicative-expression
;

assignment-expression
: additive-expression
| identified-expression "=" additive-expression
;

%-> assignment-expression
