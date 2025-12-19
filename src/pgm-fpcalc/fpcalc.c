/* DannyNiu/NJF, 2025-01-08. Public Domain. */

#include "fpcalc-grammar.h"
#include "fpcalc.h"
#include <math.h>
#include "../lalr-common/print-prod.c.h"

void print_token(lex_token_t *tn, int indentlevel);
void print_prod(lalr_prod_t *prod, int indentlevel, strvec_t *ns);
#define eprintf(...) //fprintf(stderr, __VA_ARGS__)

static inline lalr_rule_t rules(int32_t r)
{
    return fpcalc_grammar_rules[r];
}

#define theRule rules(sp->body->rule)

#define prod_expect_next(rulefunc, n) do {                      \
        if( rules(x->rule) != rulefunc ) return NULL;           \
        else x = x->terms[n].production; } while( false )

static s2data_t *resolve_id_from_expr(lalr_prod_t *addexpr)
{
    // Returns the identifier token from the degenerate cases
    // rooted in the additive expression.

    lalr_prod_t *x = addexpr;

    prod_expect_next(addexpr_degenerate, 0);
    prod_expect_next(mulexpr_degenerate, 0);
    prod_expect_next(unaryexpr_degenerate, 0);
    prod_expect_next(primary_identexpr, 0);

    return x->terms[0].terminal->str;
}

static bool params_args_lengths_match(lalr_prod_t *params, lalr_prod_t *args)
{
    const char *a, *b;
    while( true )
    {
        a = strvec_i2str(ns_rules_fpcalc, params->production);
        b = strvec_i2str(ns_rules_fpcalc,   args->production);

        if( strcmp(a, "additive-expression") == 0 &&
            strcmp(b, "additive-expression") == 0 )
        {
            return true;
        }

        if( strcmp(a, "additive-expression-list") != 0 ||
            strcmp(b, "additive-expression-list") != 0 )
        {
            return false;
        }

        params = params->terms[0].production;
        args   =   args->terms[0].production;
    }
}

static lalr_prod_t *find_arg_from_param_using_id(
    s2data_t *id, lalr_prod_t *params, lalr_prod_t *args)
{
    s2data_t *paramid;

    if( !params || !args ) return NULL;

    while( true )
    {
        if( rules(params->rule) == addexprlist_recursion )
        {
            if( rules(args->rule) != addexprlist_recursion )
                return NULL;

            paramid = resolve_id_from_expr(params->terms[2].production);
            if( s2data_cmp(paramid, id) == 0 )
                return args->terms[2].production;
        }
        else if( rules(params->rule) == addexprlist_base )
        {
            if( rules(args->rule) != addexprlist_base )
                return NULL;

            paramid = resolve_id_from_expr(params->terms[2].production);
            if( s2data_cmp(paramid, id) == 0 )
                return args->terms[2].production;

            paramid = resolve_id_from_expr(params->terms[0].production);
            if( s2data_cmp(paramid, id) == 0 )
                return args->terms[0].production;
        }
        else if( rules(params->rule) == addexpr_degenerate )
        {
            // 2025-02-17:
            // In this case, argument can be any expression - not limited
            // to the addition degenerate case.
            //- if( rules(args->rule) != addexpr_degenerate )
            //-     return NULL;

            return args;
        }
        else return NULL;

        params = params->terms[0].production;
        args   = args  ->terms[0].production;
    }
}

s2dict_t *globaldefs = NULL;

static lalr_prod_t *find_def_from_id(s2data_t *id)
{
    lalr_prod_t *def = NULL;

    if( s2dict_get_T(lalr_prod_t)(globaldefs, id, &def) != s2_access_success)
        return NULL;
    else return def;
}

int remember_definition(lalr_prod_t *expr, int semantic)
{
    lalr_prod_t *x = expr;

    if( rules(x->rule) != fpcalc_goal ) return s2_access_nullval;
    else x = x->terms[0].production;

    if( rules(x->rule) != assignexpr_assignment ) return s2_access_nullval;
    else x = x->terms[0].production;

    if( strcmp(strvec_i2str(ns_rules_fpcalc, x->production),
               "identified-expression") != 0 )
        return s2_access_nullval;

    assert( s2_is_token(x->terms[0].terminal) );

    return s2dict_set(
        globaldefs, x->terms[0].terminal->str, expr->pobj, semantic);
}

typedef struct eval_stack_chained_ctx eval_stack_chained_t;

struct eval_stack_chained_ctx {
    eval_stack_chained_t *ret;
    size_t operand_index;
    lalr_prod_t *body;
};

#define create_value(lval, type)                                \
    lval = lval ? lval : (s2obj_t *)s2data_create(sizeof(type))

#define value_body_of_term(n)                           \
    ((s2data_t *)sp->body->terms[n].production->value)

bool fpcalc_eval_start(
    double *ret,
    lalr_prod_t *expr,
    lalr_prod_t *params,
    lalr_prod_t *args)
{
    eval_stack_chained_t rs_anch = {
        .ret = NULL,
        .operand_index = 0,
        .body = expr,
    };
    eval_stack_chained_t *sp = &rs_anch, *bp;
    double *fpreg;

    if( params && args )
    {
        if( !params_args_lengths_match(params, args) )
        {
            // 2025-02-16:
            // Diagnostic return, not a functional part.
            return false;
        }
    }

    eprintf("\n========\nEntering `fpcalc_eval_start`.\n");
    /*
    printf("== == == expr:\n");
    print_prod(expr, 0, ns_rules_fpcalc);
    if( params )
    {
        printf("-------- params:\n");
        print_prod(params, 0, ns_rules_fpcalc);
    }
    if( args )
    {
        printf("-------- args:\n");
        print_prod(args, 0, ns_rules_fpcalc);
    }
    printf("-- -- --\n");//*/

start_eval_1term:

    while( sp->operand_index < sp->body->terms_count )
    {
        eprintf("sp: %p, opind: %zd, ", sp, sp->operand_index);
        if( !s2_is_prod(sp->body->terms[sp->operand_index].production) )
        {
            eprintf("terminal: %s.\n",
                    (const char *)s2data_weakmap(
                        sp->body->terms[sp->operand_index].terminal->str));
            sp->operand_index++;
            continue;
        }

        bp = calloc(1, sizeof(eval_stack_chained_t));
        if( !bp )
        {
            eprintf("Memory Allocation Error. sp: %p.\n", sp);
            goto fail;
        }

        bp->ret = sp;
        bp->operand_index = 0;
        bp->body = sp->body->terms[sp->operand_index].production;

        sp = bp;
        eprintf("descend.\n");
        goto start_eval_1term;
    }

    create_value(sp->body->value, double);

    fpreg = s2data_map((s2data_t *)sp->body->value, 0, sizeof(double));

    if( theRule == fpcalc_goal )
    {
        assert( s2_is_prod(sp->body->terms[0].production) );
        *fpreg =
            *(double *)s2data_weakmap(value_body_of_term(0));
    }

    if( theRule == assignexpr_degenerate )
    {
        assert( s2_is_prod(sp->body->terms[0].production) );
        *fpreg =
            *(double *)s2data_weakmap(value_body_of_term(0));
    }

    if( theRule == addexpr_degenerate )
    {
        assert( s2_is_prod(sp->body->terms[0].production) );
        *fpreg =
            *(double *)s2data_weakmap(value_body_of_term(0));
    }

    if( theRule == addexpr_addition )
    {
        assert( s2_is_prod(sp->body->terms[0].production) );
        assert( s2_is_prod(sp->body->terms[2].production) );
        *fpreg =
            *(double *)s2data_weakmap(value_body_of_term(0)) +
            *(double *)s2data_weakmap(value_body_of_term(2));
    }

    if( theRule == addexpr_subtraction )
    {
        assert( s2_is_prod(sp->body->terms[0].production) );
        assert( s2_is_prod(sp->body->terms[2].production) );
        *fpreg =
            *(double *)s2data_weakmap(value_body_of_term(0)) -
            *(double *)s2data_weakmap(value_body_of_term(2));
    }

    if( theRule == mulexpr_degenerate )
    {
        assert( s2_is_prod(sp->body->terms[0].production) );
        *fpreg =
            *(double *)s2data_weakmap(value_body_of_term(0));
    }

    if( theRule == mulexpr_multiplication )
    {
        assert( s2_is_prod(sp->body->terms[0].production) );
        assert( s2_is_prod(sp->body->terms[2].production) );
        *fpreg =
            *(double *)s2data_weakmap(value_body_of_term(0)) *
            *(double *)s2data_weakmap(value_body_of_term(2));
    }

    if( theRule == mulexpr_division )
    {
        assert( s2_is_prod(sp->body->terms[0].production) );
        assert( s2_is_prod(sp->body->terms[2].production) );
        *fpreg =
            *(double *)s2data_weakmap(value_body_of_term(0)) /
            *(double *)s2data_weakmap(value_body_of_term(2));
    }

    if( theRule == unaryexpr_degenerate )
    {
        assert( s2_is_prod(sp->body->terms[0].production) );
        *fpreg =
            *(double *)s2data_weakmap(value_body_of_term(0));
    }

    if( theRule == unaryexpr_positive )
    {
        assert( s2_is_prod(sp->body->terms[1].production) );
        *fpreg =
            *(double *)s2data_weakmap(value_body_of_term(1));
    }

    if( theRule == unaryexpr_negative )
    {
        assert( s2_is_prod(sp->body->terms[1].production) );
        *fpreg =
            -*(double *)s2data_weakmap(value_body_of_term(1));
    }

    if( theRule == primary_identexpr )
    {
        assert( s2_is_prod(sp->body->terms[0].production) );
        *fpreg =
            *(double *)s2data_weakmap(value_body_of_term(0));
    }

    if( theRule == primary_number )
    {
        assert( s2_is_token(sp->body->terms[0].terminal) );
        *fpreg = atof(s2data_weakmap(sp->body->terms[0].terminal->str));
        eprintf("Dump: %f,\n", *fpreg);
    }

    if( theRule == primary_paren )
    {
        *fpreg =
            *(double *)s2data_weakmap(value_body_of_term(1));
    }

    if( theRule == identexpr_label ) // 2025-02-16: still implementing.
    {
        bool subret, builtin;
        s2data_t *idname, *idres;
        lalr_prod_t *def;
        lalr_prod_t *arg;
        const char *name;
        double resultvalue;

        eprintf("Evaluating value for variable.\n");

        assert( s2_is_token(sp->body->terms[0].terminal) );
        idname = sp->body->terms[0].terminal->str;
        builtin = false;

        arg = find_arg_from_param_using_id(idname, params, args);

        // from this point onwards till the end of this block,
        // `arg` shall not be modified.
        if( arg )
        {
            resultvalue = *(double *)s2data_weakmap((s2data_t *)arg->value);
            eprintf("- found in arguments: %f.\n", resultvalue);
        }
        else
        {
            idres = idname;
            name = s2data_map(idres, 0, 0);
#define match_builtin_var(nm, v)                \
            do {                                \
                if( 0 == strcmp(name, #nm) ) {  \
                    builtin = true;             \
                    resultvalue = v;            \
                }                               \
            } while( false )

            match_builtin_var(e, M_E);
            match_builtin_var(pi, M_PI);
            s2data_unmap(idres);
        }

        if( !builtin && !arg )
        {
            def = find_def_from_id(idres);
            if( !def )
            {
                eprintf(". not a builtin, argument, or defined variable.\n");
                goto fail;
            }

            subret = fpcalc_eval_start(
                &resultvalue,
                //
                //-- expr --//
                def        ->terms[0] // from the goal symbol,
                .production->terms[2] // from `assignexpr_assignment`,
                .production,
                //
                //-- params, args --//
                NULL, NULL);

            eprintf("- attempt finding in globals: %f (subret: %d).\n",
                    resultvalue, subret);

            if( !subret )
            {
                // 2025-05-18:
                // silent failure of identifier evaluation -
                // could be that the identifier isn't a number.
                resultvalue = 0.0 / 0.0;
            }
        }

        *fpreg = resultvalue;
    }

    if( theRule == identexpr_function_noparam )
    {
        // 2025-02-12 TODO: implement me.
    }

    if( theRule == identexpr_function_1param ||
        theRule == identexpr_function_multiparam )
    {
        bool subret, builtin;
        s2data_t *idname, *idres;
        lalr_prod_t *def;
        lalr_prod_t *arg;
        const char *name;
        double resultvalue, a, b;

        eprintf("Evaluating function call expression.\n");

        assert( s2_is_token(sp->body->terms[0].terminal) );
        idname = sp->body->terms[0].terminal->str;
        builtin = false;

        idres = NULL;
        arg = find_arg_from_param_using_id(idname, params, args);
        if( arg ) idres = resolve_id_from_expr(arg);
        if( !idres ) idres = idname;

        name = s2data_map(idres, 0, 0);
#define match_builtin_func(nm, ...)             \
        do {                                    \
            if( 0 == strcmp(name, #nm) ) {      \
                builtin = true;                 \
                resultvalue = nm(__VA_ARGS__);  \
            }                                   \
        } while( false )

        if( theRule == identexpr_function_1param )
        {
            a = *(double *)s2data_weakmap(value_body_of_term(2));

            match_builtin_func(sin, a);
            match_builtin_func(cos, a);
            match_builtin_func(tan, a);
            match_builtin_func(asin, a);
            match_builtin_func(acos, a);
            match_builtin_func(atan, a);

            match_builtin_func(sinh, a);
            match_builtin_func(cosh, a);
            match_builtin_func(tanh, a);
            match_builtin_func(asinh, a);
            match_builtin_func(acosh, a);
            match_builtin_func(atanh, a);

            match_builtin_func(exp, a);
            match_builtin_func(exp2, a);
            // match_builtin_func(exp10, a);
            match_builtin_func(expm1, a);
            // match_builtin_func(exp2m1, a);
            // match_builtin_func(exp10m1, a);

            match_builtin_func(log, a);
            match_builtin_func(log2, a);
            match_builtin_func(log10, a);
            // match_builtin_func(logp1, a);
            // match_builtin_func(log2p1, a);
            // match_builtin_func(log10p1, a);
            match_builtin_func(log1p, a); // alias.

            match_builtin_func(sqrt, a);
            match_builtin_func(cbrt, a);
            match_builtin_func(fabs, a);
            if( 0 == strcmp(name, "abs") ) {
                builtin = true;
                resultvalue = fabs(a);
            }
        }
        else
        {
            lalr_rule_t fprule = rules(sp->body->terms[2].production->rule);
            if( fprule == addexprlist_base )
            {
                def = sp->body->terms[2].production->terms[0].production;
                a = *(double *)s2data_weakmap((s2data_t *)def->value);

                def = sp->body->terms[2].production->terms[2].production;
                b = *(double *)s2data_weakmap((s2data_t *)def->value);

                match_builtin_func(hypot, a, b);
                match_builtin_func(atan2, a, b);
            }
        }
        s2data_unmap(idres);

        if( !builtin ) // not a built-in.
        {
            def = find_def_from_id(idres);
            if( !def )
            {
                eprintf(". not a builtin or defined function.\n");
                goto fail;
            }

            subret = fpcalc_eval_start(
                &resultvalue,
                //
                //-- expr --//
                def        ->terms[0] // from the goal symbol,
                .production->terms[2] // from the `assignexpr_assignment` rule,
                .production,
                //
                //-- params --//
                def        ->terms[0] // from the goal symbol,
                .production->terms[0] // ident-expr from assign-expr-assignment,
                .production->terms[2] // param from function-form ident-expr.
                .production,
                //
                //-- args --//
                sp->body->terms[2] // the currently evaluated expressions.
                .production);

            if( !subret )
            {
                eprintf(". function evaluation failure.\n");
                goto fail;
            }
        }

        *fpreg = resultvalue;
    }

    if( sp->body == expr ) // 2025-02-16: was `theRule == fpcalc_goal`.
    {
        *ret = *fpreg;
        while( sp != &rs_anch )
        {
            bp = sp;
            sp = sp->ret;
            free(bp);
        }
        eprintf("fpreg: %f, rule %i -- returning --.\n",
                *fpreg, sp->body->rule);
        s2data_unmap((s2data_t *)sp->body->value);
        return true;
    }

    eprintf("rule applied; ");
    eprintf("fpreg: %f, rule %i; ", *fpreg, sp->body->rule);
    s2data_unmap((s2data_t *)sp->body->value);

    bp = sp;
    sp = sp->ret;
    free(bp);
    sp->operand_index++;
    eprintf("returning.\n");
    goto start_eval_1term;

    { fail:
        eprintf("!!FAILURE!!\n");
        while( sp != &rs_anch )
        {
            bp = sp;
            sp = sp->ret;
            free(bp);
        }
        return false;
    }
}
