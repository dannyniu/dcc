/* DannyNiu/NJF, 2026-01-09. Public Domain. */

#include "cxing-interp.h"
#include "runtime.h"
#include "expr.h"
#include "../langlex/langlex-cxing.h"
#include <stdarg.h>

#define NS_RULES ns_rules_cxing
#define var_lex_elems LexElems

bool CXParserInitCommon()
{
    int subret = 0, i;
    for(i=0; var_lex_elems[i].pattern; i++)
    {
        subret = libregcomp(
            &var_lex_elems[i].preg,
            var_lex_elems[i].pattern,
            var_lex_elems[i].cflags);
        if( subret != 0 ) goto cleanup;
    }

    NS_RULES = strvec_create();
    if( !NS_RULES )
        goto cleanup;

    return true;

cleanup:
    if( NS_RULES ) s2obj_release(NS_RULES->pobj);

    for(i=0; var_lex_elems[i].pattern; i++)
    {
        libregfree(&var_lex_elems[i].preg);
    }

    return false;
}

void CXParserFinalCommon()
{
    int i;

    for(i=0; var_lex_elems[i].pattern; i++)
    {
        libregfree(&var_lex_elems[i].preg);
    }

    lalr_parse_accel_cache_clear();
    s2obj_release(NS_RULES->pobj);
}

void CxingSyntaxErr(cxing_module_t *restrict module, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "[Syntax Error]: ");
    vfprintf(stderr, msg, ap);
    va_end(ap);
    module->error_count ++;
}

void CxingSemanticErr(cxing_module_t *restrict module, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "[Semantic Error]: ");
    vfprintf(stderr, msg, ap);
    va_end(ap);
    module->error_count ++;
}

static inline lalr_rule_t rules(int32_t r)
{
    return cxing_grammar_rules[r];
}

#define theRule rules(node_body->semantic_rule)

struct value_nativeobj CXConstDefParse(lalr_prod_t *node_body)
{
    struct value_nativeobj ret;

    if( theRule == const_true ) //>RULEIMPL<//
    {
        ret = (struct value_nativeobj){
            .proper.l = 1,
            .type = (const void *)&type_nativeobj_long };
    }

    if( theRule == const_false ) //>RULEIMPL<//
    {
        ret = (struct value_nativeobj){
            .proper.l = 0,
            .type = (const void *)&type_nativeobj_long };
    }

    if( theRule == const_null ) //>RULEIMPL<//
    {
        ret = (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    if( theRule == const_declit ) //>RULEIMPL<//
    {
        ret = (struct value_nativeobj){
            .proper.l = atoll(s2data_weakmap(
                                  node_body
                                  ->terms[0].terminal->str)),
            .type = (const void *)&type_nativeobj_long };
    }

    if( theRule == const_octlit ) //>RULEIMPL<//
    {
        char *t;
        t = s2data_weakmap(node_body->terms[0].terminal->str);
        if( strncmp(t, "0o", 2) == 0 ||
            strncmp(t, "0O", 2) == 0 )
        {
            // support `0o` notation.
            t += 2;
        }
        else if( strlen(t) > 1 )
        {
            CxingDebug("Integer literals with leading digits are octal! "
                       "Use `0o` prefix to silence this warning.\n");
        }

        ret = (struct value_nativeobj){
            .proper.u = strtoll(t, NULL, 8),
            .type = (const void *)&type_nativeobj_long };
    }

    if( theRule == const_hexlit ) //>RULEIMPL<//
    {
        ret = (struct value_nativeobj){
            .proper.u = strtoll(s2data_weakmap(
                                    node_body
                                    ->terms[0].terminal->str), NULL, 16),
            .type = (const void *)&type_nativeobj_ulong };
    }

    if( theRule == const_decfplit || //>RULEIMPL<//
        theRule == const_hexfplit ) //>RULEIMPL<//
    {
        ret = (struct value_nativeobj){
            .proper.f = strtod(s2data_weakmap(
                                   node_body
                                   ->terms[0].terminal->str), NULL),
            .type = (const void *)&type_nativeobj_double };
    }

    if( theRule == const_charlit ) //>RULEIMPL<//
    {
        s2data_t *litsrc = node_body->terms[0].terminal->str;
        size_t len = s2data_len(litsrc);
        const uint8_t *ptr;

        ptr = s2data_map(litsrc, 0, len);
        assert( ptr[0] == '\'' && ptr[len-1] == '\'' );

        ptr++;
        if( len == 2 )
        {
            ret = (struct value_nativeobj){
                .proper.l = '\0',
                .type = (const void *)&type_nativeobj_long };
        }
        else
        {
            ret = (struct value_nativeobj){
                .proper.l = ChrLit_Unquote(&ptr),
                .type = (const void *)&type_nativeobj_long };
        }
    }

    return ret;
}
