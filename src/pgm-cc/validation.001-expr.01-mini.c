/* DannyNiu/NJF, 2026-09-26. Public Domain. */

#include "../cpp-c/cpp-c.h"
#include "c-grammar.h"

#include "mental-comp.h"
#include "../pgm-cgen-abi-spec/omega-supported.h"

#include <s2obj.h>

#define GRAMMAR_RULES c_grammar_rules
#define NS_RULES ns_rules_c
#define var_lex_elems CLexElems
#include "../lalr-common/lalr.h"

#include <time.h>

int logger(void *ctx, const char *msg)
{
    (void)ctx;
    fprintf(stderr, "%s\n", msg);
    return 0;
}

void validation_mini_stream_check(lalr_stack_t *parsed, cpptu_t *cpptu);

int main(int argc, char *argv[])
{
    cpptu_t *cpptu;
    lalr_stack_t *parsed;
    lalr_term_t *te;
    int indentlevel = 0;
    int subret = 0, i;

    clock_t perfcounter;

#if INTERCEPT_MEM_CALLS
    long acq_before = 0;
    long rel_before = 0;
    long acq_after = 0;
    long rel_after = 0;
#endif /* INTERCEPT_MEM_CALLS */

    ccPreprocInit();
    perfcounter = clock();

    assert( argc > 1 );

    cpptu = cpptu_create(argv[1], NULL);
    s2list_insert(cpptu->IncPaths, s2data_from_str(
                      "../tests/dcc-preproc")->pobj, s2_setter_gave);
    cpptu->ctx_shifter.logger_base = (struct logging_ctxbase){
        .logger = (logger_func)logger,
    };
    cpptu->misc = (void **)&parsed;

#if INTERCEPT_MEM_CALLS
    acq_before = allocs;
    rel_before = frees;
#endif /* INTERCEPT_MEM_CALLS */

    perfcounter = clock();
    i = lalr_parse(&parsed, GRAMMAR_RULES, NULL, NS_RULES,
                   (token_shifter_t)cppMainProgramCoroutine, (void *)cpptu);
    printf("parsing returned: %d after %ld clock cycles, stack:\n",
           i, clock() - perfcounter);

    te = parsed->bottom;
    while( te )
    {
        printf("%p\t: ", te);

        if( s2_is_prod(te->production) )
        {
            print_prod(te->production, indentlevel, NS_RULES);
        }
        else print_token(te->terminal, indentlevel);

        te = te->up;
    }

    validation_mini_stream_check(parsed, cpptu);

    s2obj_release(parsed->pobj);
    s2obj_release(cpptu->pobj);

    perfcounter = clock() - perfcounter;
    lalr_parse_accel_cache_clear();
    ccPreprocFin();

#ifndef SAFETYPES2_BUILD_WITHOUT_GC
    s2obj_t *gctail = s2gc_obj_alloc(0x6543, 128);
    s2obj_t *gcsave = gctail;
    i=0;
    for(i=0; gctail; i++)
    {
        if( !gcsave->gc_prev ) break;
        printf("%d: (%p) %x %d+%d.\n", i, gctail, gctail->type, gctail->refcnt, gctail->keptcnt);
        if( s2_is_token(gctail) )
        {
            lex_token_t *tok = (void *)gctail;
            printf("tok<%d>: `%s`\n", tok->completion, (char *)s2data_weakmap(tok->str));
        }
        if( s2_is_data(gctail) )
        {
            printf("%s (%p, %zd bytes)\n",
                   (const char *)s2data_weakmap((s2data_t *)gctail),
                   gctail, s2data_len((s2data_t *)gctail));
        }
        if( gctail->type == S2_OBJ_TYPE_MINI_INSTR )
        {
            mInstr_t *ins = (void *)gctail;
            print_minstr(ins);
        }

        gctail = gctail->gc_prev;
    }
    s2obj_release(gcsave);
#endif

#if INTERCEPT_MEM_CALLS
    acq_after = allocs;
    rel_after = frees;
    printf("acq-before: %ld, acq-after: %ld.\n", acq_before, acq_after);
    printf("rel-before: %ld, rel-after: %ld.\n", rel_before, rel_after);
    printf("mem-acquire: %ld, mem-release: %ld.\n", allocs, frees);
    for(i=0; i<4; i++)
    {
        if( mh[i] ) subret = EXIT_FAILURE;
        printf("%08lx%c", (long)mh[i], i==3 ? '\n' : ' ');
    }
#endif /* INTERCEPT_MEM_CALLS */
    return subret;
}

void validation_mini_stream_check(lalr_stack_t *parsed, cpptu_t *cpptu)
{
    dccMeta_t MetaCtx = {};
    omega_register_allocator_t *regalloc;
    lalr_prod_t *expression = parsed->bottom->production;
    int64_t cgen_stats;

    expression = expression->terms[3].production; // func-body<comp-stmt>.
    expression = expression->terms[1].production; // blk-itm-lst<unlab-stmt>.
    expression = expression->terms[1].production; // jmp-stmt<jmp-stmt>.
    expression = expression->terms[1].production; // expr<add-expr>.

    print_prod(expression, 0, NS_RULES);

    MetaCtx.mini_stream = s2list_create();
    MetaCtx.scoped_decls = s2list_create();
    MetaCtx.abi_oracle = &OmegaAArch64;
    MetaCtx.stack_entries_reusedops = s2data_create(0);
    MetaCtx.stack_entries_ephemerals = s2data_create(0);
    MetaCtx.ctx_tu = cpptu;
    regalloc = MetaCtx.abi_oracle->create_register_allocator_ctx();

    SemaTrav_NodeColoring(expression, &MetaCtx);
    SemaTrav_OperandDiscount(expression, &MetaCtx);
    SemaTrav_CollectReuseStats(expression, &MetaCtx);
    XhaleTrav_MiniStream(expression, &MetaCtx);
    cgen_stats = MiniStream_InsertSaveRestores(
        &MetaCtx, false,
        regalloc, 0);

    printf("CGen Stats: %lld.\n", cgen_stats);

    s2list_seek(MetaCtx.mini_stream, 0, S2_LIST_SEEK_SET);
    while( s2list_pos(MetaCtx.mini_stream) < s2list_len(MetaCtx.mini_stream) )
    {
        mInstr_t *ins;

        s2list_get_T(mInstr_t)(MetaCtx.mini_stream, &ins);
        print_minstr(ins);

        s2list_seek(MetaCtx.mini_stream, 1, S2_LIST_SEEK_CUR);
    }

    s2obj_release(MetaCtx.stack_entries_ephemerals->pobj);
    s2obj_release(MetaCtx.stack_entries_reusedops->pobj);
    s2obj_release(MetaCtx.scoped_decls->pobj);
    s2obj_release(MetaCtx.mini_stream->pobj);
    CookieTable_Destroy(&MetaCtx.consumer_stats);
    CookieTable_Destroy(&MetaCtx.usage_stats);
    free(regalloc);
}
