/* DannyNiu/NJF, 2024-12-30. Public Domain. */

#ifndef dcc_lalr_h
#define dcc_lalr_h 1

#include "../lex/lex.h"
#include "../infra/strvec.h"

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

    int32_t production;
    int32_t rule;

    // 2025-01-30:
    // Semantics are evaluated during reduce operations during parse,
    // and are assigned to `value`.
    s2obj_t *value;

    size_t terms_count;
    union {
        lalr_prod_t *production;
        lex_token_t *terminal;
    } *terms;
};

struct lalr_term {
    // 2025-01-27:
    // This represents an element on the parsing stack.
    // It's a bi-directional linked-list.

    lalr_stack_t *container; // unretained (weak ref).

    // down towards the bottom, up towards top.
    lalr_term_t *dn, *up;

    // retained on behalf of the stack.
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

#ifdef dcc_lalr_defining_grammar
#define symtype_prod   .type = lalr_symtype_prod
#define symtype_vtoken .type = lalr_symtype_vtoken
#define symtype_stoken .type = lalr_symtype_stoken
#define lalr_opt       .optional = true
#define lalr_rule_gen_args symbolseq, production, action, terms, ns_rules
#endif /* dcc_lalr_defining_grammar */

typedef enum {
    // - returns 0 if not matched,
    // - returns 1 if prefix-matched,
    // - returns 2 if completely matches.
    lalr_rule_action_match = 1,

    // - NULL on failure, otherwise
    // - the reduced symbol is returned.
    lalr_rule_action_reduce,

    // Purpose: if the 1st symbol in the rule's rewrite sequence matches
    // one of the 'expectations' and the left-hand side of the rule's
    // production is none in the 'expectations', then false is returned to
    // indicate to not further reduce the recent-most anchored tokens using
    // the current rule; otherwise true is returned.
    // Additionally, if ''expecting'' some terminal symbols, and the next
    // token isn't one of the expeceted, false is returned to indicate the
    // rule is not one of the candidates.
    lalr_rule_action_expect,
} lalr_rule_action_t;

#define lalr_rule_params                                \
    lalr_rule_action_t action,                          \
        lalr_term_t *restrict terms,                    \
        void *restrict ctx, strvec_t *restrict ns_rules

// The grammar define a set of rules in the form of an array of `lalr_rule_t`
// function pointers. The 0th element of the array is the goal symbol,
// the production of the goal symbol is "." .
typedef void *(*lalr_rule_t)(lalr_rule_params);

// Implements common rule actions for all rules.
void *lalr_rule_actions_generic(
    lalr_rule_symbol_t *restrict symbolseq, int32_t production,
    lalr_rule_action_t action,
    lalr_term_t *restrict terms,
    // `ctx` is used by rules themselves, to e.g. build semantics.
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
