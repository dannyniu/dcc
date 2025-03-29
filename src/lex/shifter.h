/* DannyNiu/NJF, 2025-03-29. Public Domain. */

#ifndef dcc_shifter_h
#define dcc_shifter_h 1

#include "lex.h"

typedef struct {
    lex_getc_base_t base;
    union {
        FILE *fp;
        const char *expr;
    };
    const struct lex_fsm_trans *fsm;
    const char **puncts;
    lexer_state_t matched_fallback;
    int32_t lineno, column;
} shifter_ctx_t;

shifter_ctx_t *shifter_init_from_fp(shifter_ctx_t *shctx, FILE *fp);
shifter_ctx_t *shifter_init_from_expr(shifter_ctx_t *shctx, const char *expr);
lex_token_t *shifter(shifter_ctx_t *shctx);

#endif /* dcc_shifter_h */
