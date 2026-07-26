/* DannyNiu/NJF, 2026-07-05. Public Domain */

#ifndef c_misc_diagnose_h
#define c_misc_diagnose_h 1

//#define DccTrainingWheel 1 // Option for me (@dannyniu) while developing.

enum {
    elog_raw = -1,
    elog_fatal,
    elog_error,
    elog_warn,
    elog_note,
    elog_trace,
};

void ccDiagnose(int level, const char *msg, ...);


#ifndef DccTrainingWheel
#define __diagnose_msg_concat__(level, ctx_tu, msg, submsg, ...)        \
    ccDiagnose(level, "In file \"%s\", " msg submsg "\n",               \
               (char *)s2data_weakmap(ctx_tu->HotFile), __VA_ARGS__)
#else
#define __diagnose_msg_concat__(level, ctx_tu, msg, submsg, ...)        \
    ccDiagnose(level, "%s %s: in file \"%s\", "                         \
               msg submsg "\n", __FILE__, __func__,                     \
               (char *)s2data_weakmap(ctx_tu->HotFile), __VA_ARGS__)
#endif // DccTrainingWheel


#define ccDiagnoseError(ctx_tu, msg, ...)                               \
    __diagnose_msg_concat__(elog_error, ctx_tu, msg, __VA_ARGS__)
#define ccDiagnoseWarn(ctx_tu, msg, ...)                                \
    __diagnose_msg_concat__(elog_warn, ctx_tu, msg, __VA_ARGS__)

#define spelling_and_site(token) " `%s` at line %d, column %d.", (const char *)s2data_weakmap(token->str), token->lineno, token->column //+undef

extern volatile int undef;

#endif /* c_misc_diagnose_h */
