/* DannyNiu/NJF, 2025-01-08. Public Domain. */

#include "cxing-grammar.h"
#include "cxing-interp.h"
#include "runtime.h"
#include <math.h>
#include "../lalr-common/print-prod.c.h"

void print_token(lex_token_t *tn, int indentlevel);
void print_prod(lalr_prod_t *prod, int indentlevel, strvec_t *ns);
#define eprintf(...) //fprintf(stderr, __VA_ARGS__)

static inline lalr_rule_t rules(int32_t r)
{
    return cxing_grammar_rules[r];
}

#define theRule rules(sp->body->rule)

#define prod_expect_next(rulefunc, n) do {                      \
        if( rules(x->rule) != rulefunc ) return NULL;           \
        else x = x->terms[n].production; } while( false )

typedef struct {
    size_t capacity;
    size_t spind; // stack pointer/index.
    struct value_nativeobj *values;
} values_nativobj_stack_t;

#define ValuesStackInit(s) memset(&s, 0, sizeof(values_nativobj_stack))

bool ValuesStackPush(
    values_nativobj_stack_t *values, struct value_nativeobj value)
{
    assert( values->spind < values->capacity );

    if( ++values->spind == values->capacity )
    {
        struct value_nativeobj *T = realloc(
            values->values, values->capacity+1);
        if( !T )
        {
            CxingFatal("Stack Growing Failed!");
            return false;
        }

        values->values = T;
    }

    values->values[values->spind] = value;
    return true;
}

typedef struct cxing_program_counter cxing_program_counter_t;

struct cxing_program_counter {
    lalr_prod_t *body; // function body.
    values_nativobj_stack_t *stackfrm; // stack frame.
    int flags;
    int traversal_order; // in-order traversal.
};
