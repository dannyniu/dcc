/* DannyNiu/NJF, 2024-12-30. Public Domain. */

#ifndef dcc_lalr_h
#define dcc_lalr_h 1

#include "../lex-common/lex.h"
#include "../infra/strvec.h"
#include <s2dict.h>

#define S2_OBJ_TYPE_PRODUCTION 0x2021
#define S2_OBJ_TYPE_STACK      0x2022
#define s2_is_prod(obj)  (((s2obj_t *)obj)->type == S2_OBJ_TYPE_PRODUCTION)
#define s2_is_stack(obj) (((s2obj_t *)obj)->type == S2_OBJ_TYPE_STACK)

typedef struct lalr_production lalr_prod_t;
typedef struct lalr_term       lalr_term_t;
typedef struct lalr_stack      lalr_stack_t;
typedef struct lalr_rulesym    lalr_rule_symbol_t;

// non-SafeTypes2 types.
void lalr_term_free(lalr_term_t *term);
void lalr_rule_symbol_free(lalr_rule_symbol_t *chain);

struct lalr_production {
    // 2025-01-27:
    // This is the result of a reduce. It consist of
    // a number (the `production` member) indicating
    // its left-hand side name, and `terms_count` number
    // of elements representing what's reduced on the
    // right-hand side.
    s2obj_base;

    int32_t production; // for to be recognized in matching,
    int32_t semantic_production; // the actual semantic,
    int32_t rule; // index into the rules table.
    int32_t semantic_rule; // same distinction as above.
    size_t terms_count; // number of elements in `terms`.

    union {
        lalr_prod_t *production;
        lex_token_t *terminal;
    } *terms;

    // 2025-01-30:
    // Semantics are evaluated during reduce operations during parse,
    // and are assigned to `value`.
    s2obj_t *value;
};

struct lalr_term {
    // 2025-01-27:
    // This represents an element on the parsing stack.
    // It's a bi-directional linked-list.

    lalr_stack_t *container; // unretained (weak ref).

    // down towards the bottom, up towards top.
    lalr_term_t *dn, *up;

    // retained on behalf of the stack, but not on behalf of itself,
    // therefore `lalr_term_free` doesn't free it/them;
    // distinguish based on object type id.
    union {
        lalr_prod_t *production;
        lex_token_t *terminal;
    };

    lalr_rule_symbol_t *expecting;
    bool anchored;
};

struct lalr_stack {
    // 2025-01-27:
    // This is the type definition for the parsing stack.
    s2obj_base;
    lalr_term_t *bottom, *top;
};

struct lalr_rulesym {
    // 2025-01-27:
    // In a rule definition (an `lalr_rule_t` instance), this represents
    // an element of the right-hand side of the production.

    enum {
        // reserved for end-of-list.
        lalr_symtype_invalid = 0,

        // a production rule.
        lalr_symtype_prod,

        // V means value-varying. Typically, the token is of a type that
        // can have varying values of a certain type, such as the value of
        // a number, a string literal, or the spelling of an identifier, etc.
        lalr_symtype_vtoken,

        // S means static string. Typically, the spelling of the token
        // determines its meaning.
        lalr_symtype_stoken,
    } type;

    bool optional;

    union {
        const char *value;
        lexer_state_t vtype;
    };

    // 2025-01-27:
    // The elements in the right-hand side of a rule
    // instance (an `lalr_rule_t`), are represented as
    // an array. This member (named `next` as can be seen)
    // is used in the 'expectation' mechanism, allowing
    // it to expect on multiple rule symbols. See comments
    // in `lalr_rule_action_expect` for details.
    lalr_rule_symbol_t *next;
};

// `lalr_parse_accel_cache*` interfaces are not thread-safe.
extern s2dict_t *lalr_parse_accel_cache;
void lalr_parse_accel_cache_clear();

#ifdef dcc_lalr_defining_grammar
#define symtype_prod   .type = lalr_symtype_prod
#define symtype_vtoken .type = lalr_symtype_vtoken
#define symtype_stoken .type = lalr_symtype_stoken
#define lalr_opt       .optional = true
#define lalr_rule_gen_args \
    symbolseq, production, ri, action, terms, rules, ns_rules
#endif /* dcc_lalr_defining_grammar */

typedef enum {
    // - returns 0 if not matched,
    // - returns 1 if prefix-matched,
    // - returns 2 if completely matches.
    lalr_rule_action_match = 1,

    // - NULL on failure, otherwise
    // - the reduced symbol is returned.
    lalr_rule_action_reduce,

    // if the current rule cannot lead to any of what's expected,
    // then exclude that rule from applicable candidates.
    lalr_rule_action_expect,

    // returns the left-hand-side as an int32_t into the rules namespace table.
    lalr_rule_inspect_lhs,

    // returns the symbol sequence as a pointer to `lalr_rule_symbol_t`.
    lalr_rule_inspect_symseq,

    // Semantic actions associated with the rule. Not used by the parser.
    lalr_rule_action_evaluate,
} lalr_rule_action_t;

#define lalr_rule_params                        \
    lalr_rule_action_t action,                  \
        lalr_term_t *restrict terms,            \
        int32_t ri,                             \
        void *restrict ctx,                     \
        void *restrict rules,                   \
        strvec_t *restrict ns_rules

// The grammar define a set of rules in the form of an array of `lalr_rule_t`
// function pointers. The 0th element of the array is the goal symbol,
// the production of the goal symbol is "." .
typedef void *(*lalr_rule_t)(lalr_rule_params);

// Implements common rule actions for all rules.
void *lalr_rule_actions_generic(
    lalr_rule_symbol_t *restrict symbolseq,
    int32_t production, int32_t ri,
    lalr_rule_action_t action,
    lalr_term_t *restrict terms,
    // `ctx` is used by rules themselves, to e.g. build semantics.
    lalr_rule_t rules[restrict],
    strvec_t *restrict ns_rules);

typedef lex_token_t *(*token_shifter_t)(void *);

int lalr_parse(
    lalr_stack_t *restrict *restrict out,
    lalr_rule_t rules[restrict],
    void *restrict ctx,
    strvec_t *restrict ns_rules,
    token_shifter_t shifter,
    void *restrict shifter_ctx);

#endif /* dcc_lalr_h */
