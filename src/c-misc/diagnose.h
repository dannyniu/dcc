/* DannyNiu/NJF, 2026-07-05. Public Domain */

#ifndef c_misc_diagnose_h
#define c_misc_diagnose_h 1

//void ccDiagnoseError(void *restrict ctx_tu, const char *fmt, ...);
//void ccDiagnoseWarning(void *restrict ctx_tu, const char *fmt, ...);

enum {
    elog_fatal,
    elog_error,
    elog_warn,
    elog_note,
    elog_trace,
};

void ccDiagnose(int level, const char *msg, ...);

#define __diagnose_msg_concat__(level, msg, submsg, ...) ccDiagnose(level, "%s %s: " msg submsg "\n", __FILE__, __func__, __VA_ARGS__)

#define ccDiagnoseError(ctx_tu, msg, ...) __diagnose_msg_concat__(elog_error, msg, __VA_ARGS__)
#define ccDiagnoseWarn(ctx_tu, msg, ...) __diagnose_msg_concat__(elog_warn, msg, __VA_ARGS__)

#define spelling_and_site(token) " `%s` at line %d, column %d of file <unknown>", (const char *)s2data_weakmap(token->str), token->lineno, token->column

#endif /* c_misc_diagnose_h */
