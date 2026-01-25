/* DannyNiu/NJF, 2025-12-30. Public Domain. */

#include "cxing-grammar.h"
#include "cxing-interp.h"
#include "runtime.h"
#include "langsem.h"
#include "expr.h"
#include "../lex-common/lex.h"

#define eprintf(...) 0 // fprintf(stderr, __VA_ARGS__)
#define Reached() eprintf("Reached %d! flags: %d, spind: %zd, ri: %i; opind: %u.\n", __LINE__, instruction->flags, pc.spind, instruction->node_body->semantic_rule, instruction->operand_index);
#define CapturePoint() eprintf("CapturePoint %d! flags: %d, spind: %zd, ri: %i; opind: %u.\n", __LINE__, instruction->flags, pc.spind, instruction->node_body->semantic_rule, instruction->operand_index);

static inline lalr_rule_t rules(int32_t r)
{
    return cxing_grammar_rules[r];
}

#define theRule rules(instruction->node_body->semantic_rule)

typedef struct cxing_ast_node {
    // evaluation result.
    struct value_nativeobj ax;

    // Usages:
    // `this` argument for method calls.
    struct value_nativeobj bx;

    // arguments to a function call.
    struct value_nativeobj *fargs;

    lalr_prod_t *node_body; // unowned.
    s2dict_t *vardecls; // owned.x

    // Moved here for data structure alignment.
    unsigned fargn;
    unsigned operand_index;

    enum {
        ast_node_action_default = 0,

        // logic determined, short-circuit.
        ast_node_action_stop,

        // for conditionals.
        ast_node_action_noelse,

        // for loops.
        ast_node_action_break,
        ast_node_action_continue,

        // for function bodies.
        ast_node_action_return,

        // Flags are inherited across AST nodes.
    } flags;

    enum {
        ast_node_opt_default = 0,
        // the current production (i.e. AST node) resulted in an lvalue,
        // this flag instructs the epilogue to not clear the scope key
        // when it restores the context of the parent AST node.
        ast_node_lvalue_register,

        // An operand of this expression (or phrase) was an lvalue,
        // process it similarly to that in epilogue.
        ast_node_lvalue_operand,

        // Options are cleared across AST nodes.
    } opts;
} cxing_ast_node_t;

typedef struct {
    size_t capacity;
    size_t spind; // stack pointer/index.
    cxing_ast_node_t *instructions; // owned.
} cxing_program_counter_t;

static bool PcStackPush(
    cxing_program_counter_t *pc, lalr_prod_t *instructions)
{
    assert( pc->spind < pc->capacity );

    if( ++pc->spind == pc->capacity )
    {
        void *T = realloc(
            pc->instructions,
            (pc->capacity+1) *
            sizeof(*pc->instructions));
        if( !T )
        {
            CxingFatal("Stack Growing Failed!");
            return false;
        }

        pc->capacity ++;
        pc->instructions = T;
    }

    pc->instructions[pc->spind].node_body = instructions;
    pc->instructions[pc->spind].vardecls = NULL;
    pc->instructions[pc->spind].ax = (struct value_nativeobj){
        .proper.p = NULL, .type = (const void *)&type_nativeobj_morgoth };
    pc->instructions[pc->spind].bx = (struct value_nativeobj){
        .proper.p = NULL, .type = (const void *)&type_nativeobj_morgoth };
    pc->instructions[pc->spind].fargs = NULL;
    pc->instructions[pc->spind].fargn = 0;
    pc->instructions[pc->spind].operand_index = 0;
    pc->instructions[pc->spind].flags = ast_node_action_default;
    pc->instructions[pc->spind].opts = ast_node_opt_default;
    return true;
}

static struct value_nativeobj PcStackPop(cxing_program_counter_t *pc)
{
    cxing_ast_node_t *instruction;
    assert( pc->spind > 0 );

    instruction = &pc->instructions[pc->spind];
    /* // let funccall rule processor(s) handle resource management.
      while( instruction->fargn --> 0 )
    {
        ValueDestroy(instruction->fargs[instruction->fargn]);
        } // */

    if( instruction->vardecls )
    {
        s2obj_release(instruction->vardecls->pobj);
        instruction->vardecls = NULL;
    }

    // 2026-01-17 TODO: resource management issues.
    pc->instructions[pc->spind - 1].bx = pc->instructions[pc->spind].bx;

    // letting funccall rule processor(s) handle resource management.
    pc->instructions[pc->spind - 1].fargs = pc->instructions[pc->spind].fargs;
    pc->instructions[pc->spind - 1].fargn = pc->instructions[pc->spind].fargn;

    pc->instructions[pc->spind - 1].flags = pc->instructions[pc->spind].flags;
    // `pc->instructions[pc->spind - 1].opts` is unaltered.
    eprintf("popped: %llu t=%lli\n",
            pc->instructions[pc->spind].ax.proper.u,
            pc->instructions[pc->spind].ax.type->typeid);
    return pc->instructions[pc->spind --].ax;
}

static void PcStackAbandon(cxing_program_counter_t *pc)
{
    while( pc->spind > 0 )
        PcStackPop(pc);
}

#define PcStack_PushOrAbandon() do {                            \
        if( !PcStackPush(                                       \
                &pc, instruction->node_body->terms[             \
                    instruction->operand_index].production) )   \
            goto func_exec_abort;                               \
        else goto start_eval_1term;                             \
    } while( false )

struct CxingFuncLocalVars {
    cxing_program_counter_t *restrict pc;
    lalr_prod_t *restrict params;
    int argn;
    struct value_nativeobj *args;

    cxing_module_t *module;
    // TODO: exposed and imported symbols.
};

static int CxingFunc_ArgumentIndex(lalr_prod_t *params, s2data_t *key)
{
    int argind = -1;

    if( !params ) return -1;

    while( true )
    {
        s2data_t *ki;
        if( rules(params->semantic_rule) == arglist_empty )
        {
            return -1;
        }
        else if( rules(params->semantic_rule) == arglist_some )
        {
            params = params->terms[0].production;
            continue;
        }

        else if( rules(params->semantic_rule) == args_genrule )
        {
            if( !s2_is_token(params->terms[2].production->terms[0].terminal) )
            {
                CxingDiagnose(
                    "Expected terminal symbol token after parsing 01.");
                return -1;
            }

            ki = params->terms[2].production->terms[0].terminal->str;
            if( s2data_cmp(ki, key) == 0 )
            {
                argind = 0;
            }
            else if( argind >= 0 ) argind ++;

            params = params->terms[0].production;
            continue;
        }

        else if( rules(params->semantic_rule) == args_base )
        {
            if( !s2_is_token(params->terms[1].production->terms[0].terminal) )
            {
                CxingDiagnose(
                    "Expected terminal symbol token after parsing 02.");
                return -1;
            }

            ki = params->terms[1].production->terms[0].terminal->str;
            if( s2data_cmp(ki, key) == 0 )
            {
                argind = 0;
            }
            else if( argind >= 0 ) argind ++;

            break;
        }
    }

    return argind;
}

#define entRule(x) (cxing_grammar_rules[(x)->semantic_rule])

static struct value_nativeobj CxingFuncLocalVar_GetImpl0(
    struct CxingFuncLocalVars *localvars, s2data_t *key)
{
    size_t sfind = localvars->pc->spind;
    s2cxing_value_t *ret_wrapped;
    lalr_prod_t *de; // the definition for the entity.
    int argind = -1, accessed;

    // first try looking on the stack frame.
    for( ; sfind <= localvars->pc->spind; sfind -- )
    {
        eprintf("%d sfind: %zd\n", __LINE__, sfind);

        if( !localvars->pc->instructions[sfind].vardecls )
            continue;

        accessed = s2dict_get_T(s2cxing_value_t)(
            localvars->pc->instructions[sfind].vardecls,
            key, &ret_wrapped);

        if( accessed == s2_access_error )
        {
            CxingFatal("Error while getting local variables 01.");
            // Fail as gracefully as possible.
            return (struct value_nativeobj) {
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }

        if( accessed == s2_access_nullval )
            continue;

        eprintf("%d localvars got: %p.\n", __LINE__, ret_wrapped->cxing_value.proper.p);
        return ret_wrapped->cxing_value;
    }

    // Otherwise, in the arguments.

    argind = CxingFunc_ArgumentIndex(localvars->params, key);

    eprintf("Looked at arguments: %d\n", argind);

    if( argind >= 0 && argind < localvars->argn )
    {
        // found in arguments.
        return localvars->args[argind];
    }

    // Next, in the set of entities.

    accessed = s2dict_get_T(lalr_prod_t)(
        localvars->module->entities, key, &de);

    if( accessed == s2_access_error )
    {
        CxingFatal("Error while getting local variables 02.");
        // Fail as gracefully as possible.
        return (struct value_nativeobj) {
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    if( accessed == s2_access_success )
    {
        if( entRule(de) == funcdecl_subr )
        {
            return (struct value_nativeobj){
                .proper.p = CXSym(localvars->module, s2data_weakmap(key)),
                .type = (const void *)&type_nativeobj_subr };
        }
        else if( entRule(de) == funcdecl_method )
        {
            return (struct value_nativeobj){
                .proper.p = CXSym(localvars->module, s2data_weakmap(key)),
                .type = (const void *)&type_nativeobj_method };
        }
        else
        {
            CxingWarning("Unexpected kind of entity definition encountered. "
                         "Returning null for now.\n");
            //- assert( 0 );
            return (struct value_nativeobj) {
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }
    }

    // Finally, in the linked definitions.

    accessed = s2dict_get_T(s2cxing_value_t)(
        localvars->module->linked, key, &ret_wrapped);

    if( accessed == s2_access_error )
    {
        CxingFatal("Error while getting local variables 03.");
        // Fail as gracefully as possible.
        return (struct value_nativeobj) {
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }
    else if( accessed == s2_access_nullval )
    {
        CxingDebug(
            "The program is trying to read from a non-existing variable. "
            "Please inform the developer to debug the program.");

        return (struct value_nativeobj) {
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }
    else
    {
        assert( accessed == s2_access_success );
        return ret_wrapped->cxing_value;
    }
}

static struct value_nativeobj CxingFuncLocalVar_SetImpl0(
    struct CxingFuncLocalVars *localvars,
    s2data_t *key, struct value_nativeobj val)
{
    size_t sfind = localvars->pc->spind;
    s2cxing_value_t *ret_wrapped;
    int argind = -1, accessed;

    // first try looking on the stack frame.
    for( ; sfind <= localvars->pc->spind; sfind -- )
    {
        if( !localvars->pc->instructions[sfind].vardecls )
            continue;

        accessed = s2dict_get_T(s2cxing_value_t)(
            localvars->pc->instructions[sfind].vardecls,
            key, &ret_wrapped);

        if( accessed == s2_access_error )
        {
            CxingFatal("Error while setting local variables 01.");
            // Fail as gracefully as possible.
            return (struct value_nativeobj) {
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }

        if( accessed == s2_access_nullval )
            continue;

        if( !(ret_wrapped = s2cxing_value_create(val)) )
        {
            CxingFatal("Error while setting local variables 2.");
            // Fail as gracefully as possible.
            return (struct value_nativeobj) {
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }

        accessed = s2dict_set(
            localvars->pc->instructions[sfind].vardecls,
            key, ret_wrapped->pobj, s2_setter_gave);

        if( accessed == s2_access_error )
        {
            CxingFatal("Error while setting local variables 03.");
            // Fail as gracefully as possible.
            return (struct value_nativeobj) {
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }

        return ret_wrapped->cxing_value;
    }

    // Otherwise, in the arguments.

    argind = CxingFunc_ArgumentIndex(localvars->params, key);

    if( argind < 0 || argind >= localvars->argn )
    {
        CxingDebug(
            "The program is trying to write to a non-existing variable. "
            "Please inform the developer to debug the program.");

        return (struct value_nativeobj) {
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    // 2026-01-17 TODO: Modifying an argument parameter.

    // found in arguments.
    return localvars->args[argind];
}

static struct value_nativeobj CxingFuncLocalVar_GetImpl(
    int argn, struct value_nativeobj args[])
{
    if( argn < 2 )
    {
        return (struct value_nativeobj) {
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }
    return CxingFuncLocalVar_GetImpl0(args[0].proper.p, args[1].proper.p);
}

static struct value_nativeobj CxingFuncLocalVar_SetImpl(
    int argn, struct value_nativeobj args[])
{
    if( argn < 3 )
    {
        return (struct value_nativeobj) {
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    return CxingFuncLocalVar_SetImpl0(
        args[0].proper.p, args[1].proper.p, args[2]);
}

struct value_nativeobj CxingFuncLocalVar_Get = {
    .proper.p = CxingFuncLocalVar_GetImpl,
    .type = (const void *)&type_nativeobj_method,
};

struct value_nativeobj CxingFuncLocalVar_Set = {
    .proper.p = CxingFuncLocalVar_SetImpl,
    .type = (const void *)&type_nativeobj_method,
};

static struct TYPE_NATIVEOBJ_STRUCT(3) type_nativeobj_localvars = {
    .typeid = valtyp_obj,
    .n_entries = 2,
    .static_members = {
        { .name = "__get__", .member = &CxingFuncLocalVar_Get },
        { .name = "__set__", .member = &CxingFuncLocalVar_Set },
    },
};

#define ValueCopy(x) (eprintf("VC:%d.\n", __LINE__), ValueCopy(x))

// 2025-12-30: subroutines first, methods next.
struct value_nativeobj CxingExecuteFunction(
    cxing_module_t *restrict module,
    lalr_prod_t *restrict textsegment,
    lalr_prod_t *restrict params,
    int argn, struct value_nativeobj args[])
{
    cxing_program_counter_t pc = {
        .capacity = 0,
        .spind = 0,
        .instructions = NULL,
    };
    struct CxingFuncLocalVars localvars0 = {
        .pc = &pc,
        .params = params,
        .argn = argn, .args = args,
        .module = module,
    };
    struct value_nativeobj localvars = {
        .proper.p = &localvars0,
        .type = (const void *)&type_nativeobj_localvars,
    };
    cxing_ast_node_t *instruction;

    struct lvalue_nativeobj varreg = {
        .value.proper.p = NULL,
        .value.type = (const void *)&type_nativeobj_morgoth,
        .scope.proper.p = NULL,
        .scope.type = (const void *)&type_nativeobj_morgoth,
        .key = NULL };
#define valreg varreg.value

    if( !(pc.instructions = calloc(1, sizeof(cxing_ast_node_t))) )
    {
        CxingFatal("Unable to allocate buffer for instruction texts.");
        return (struct value_nativeobj){
            .proper.p = NULL, .type = (const void *)&type_nativeobj_morgoth };
    }
    else pc.capacity = 1;

    pc.instructions->node_body = textsegment;
    pc.instructions->operand_index = 0;
    pc.instructions->flags = ast_node_action_default;

    eprintf("Function evaluation started.\n");
    if( argn >= 1 )
        eprintf("val-proper: %p\n", args[0].proper.p);

start_eval_1term:;
    // static long cnt = 0; if( ++cnt >= 32768 ) exit(12); // TODO: remove after debugging.
    instruction = &pc.instructions[pc.spind];
    Reached();

    if( pc.instructions[0].flags == ast_node_action_return )
    {
        goto finish_eval_1term;
    }

    while( instruction->operand_index <
           instruction->node_body->terms_count )
    {
        lalr_prod_t *term = instruction->node_body->terms[
            instruction->operand_index].production; /* //
        lex_token_t *tok = instruction->node_body->terms[
            instruction->operand_index].terminal;

        if( s2_is_prod(term) )
        {
            print_prod(instruction->node_body->terms[
                       instruction->operand_index].production,
                       0, ns_rules_cxing);
            break;
        }
        else
        {
            eprintf("lineno=%d, column=%d; ", tok->lineno, tok->column);
            print_token(tok, 3);
        } /*/
        if( s2_is_prod(term) )
            break; // */
        instruction->operand_index ++;
    }

    assert( instruction->operand_index <=
            instruction->node_body->terms_count );

    //
    // lexicons for literals.

    // scalar literals.

    if( theRule == const_true )
    {
        Reached();
        instruction->ax = (struct value_nativeobj){
            .proper.l = 1,
            .type = (const void *)&type_nativeobj_long };
    }

    if( theRule == const_false )
    {
        Reached();
        instruction->ax = (struct value_nativeobj){
            .proper.l = 0,
            .type = (const void *)&type_nativeobj_long };
    }

    if( theRule == const_null )
    {
        Reached();
        instruction->ax = (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    if( theRule == const_declit )
    {
        Reached();
        instruction->ax = (struct value_nativeobj){
            .proper.l = atoll(s2data_weakmap(
                                  instruction->node_body
                                  ->terms[0].terminal->str)),
            .type = (const void *)&type_nativeobj_long };
    }

    if( theRule == const_octlit )
    {
        char *t;
        Reached();
        t = s2data_weakmap(instruction->node_body->terms[0].terminal->str);
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

        instruction->ax = (struct value_nativeobj){
            .proper.u = strtoll(t, NULL, 8),
            .type = (const void *)&type_nativeobj_long };
    }

    if( theRule == const_hexlit )
    {
        Reached();
        instruction->ax = (struct value_nativeobj){
            .proper.u = strtoll(s2data_weakmap(
                                    instruction->node_body
                                    ->terms[0].terminal->str), NULL, 16),
            .type = (const void *)&type_nativeobj_ulong };
    }

    if( theRule == const_r64lit )
    {
        Reached();
        instruction->ax = (struct value_nativeobj){
            .proper.u = Radix64Literal(s2data_weakmap(
                                           instruction->node_body
                                           ->terms[0].terminal->str)),
            .type = (const void *)&type_nativeobj_ulong };
    }

    if( theRule == const_decfplit ||
        theRule == const_hexfplit )
    {
        Reached();
        instruction->ax = (struct value_nativeobj){
            .proper.f = strtod(s2data_weakmap(
                                   instruction->node_body
                                   ->terms[0].terminal->str), NULL),
            .type = (const void *)&type_nativeobj_double };
    }

    if( theRule == const_charlit )
    {
        Reached();
        assert( 0 ); // TODO (2026-01-24).
    }

    if( theRule == const_strlit )
    {
        size_t litlen;
        s2data_t *litsrc = instruction->node_body->terms[0].terminal->str;
        s2data_t *litobj = s2data_create(litlen = s2data_len(litsrc));
        Reached();
        if( !litobj )
        {
            CxingFatal("Unable to instantiate string literal!\n");
            goto func_exec_abort;
        }

        s2obj_keep(litobj->pobj);
        s2obj_release(litobj->pobj);
        memcpy(s2data_weakmap(litobj), s2data_weakmap(litsrc), litlen);

        instruction->ax = (struct value_nativeobj){
            .proper.p = litobj,
            .type = (const void *)&type_nativeobj_s2impl_str };
    }

    //
    // identifiers resolution.

    if( theRule == ident_ident )
    {
        Reached();
        varreg = GetValProperty(
            localvars, (struct value_nativeobj){
                .proper.p = instruction->node_body->terms[0].terminal->str,
                .type = (const void *)&type_nativeobj_s2impl_str });
        instruction->ax = valreg;
        instruction->opts = ast_node_lvalue_register;
        instruction->operand_index = instruction->node_body->terms_count;
    }

    if( theRule == postfix_member )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else
        {
            Reached();
            varreg = GetValProperty(
                valreg, (struct value_nativeobj){
                    .proper.p = instruction->node_body
                    ->terms[2].production
                    ->terms[0].terminal->str,
                    .type = (const void *)&type_nativeobj_s2impl_str });
            instruction->ax = valreg;
            instruction->opts = ast_node_lvalue_register;
            instruction->operand_index = instruction->node_body->terms_count;
        }
    }

    //
    // function calls.

    if( theRule == funcinvokenocomma_base )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 2 )
        {
            Reached();
            instruction->ax = valreg;
            instruction->bx = varreg.scope;
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            instruction->fargs = calloc(2, sizeof(struct value_nativeobj));
            if( !instruction->fargs )
            {
                CxingFatal("Call to calloc failed when trying to allocate "
                           "space for function arguments.");
                goto func_exec_abort;
            }

            instruction->fargn = 2;
            // reserve 1 slot for `this` argument to methods,
            // pointer arithmetic to ignore it for subroutines.
            //- instruction->fargs[0] = ...;

            valreg = ValueCopy(valreg);
            instruction->fargs[1] = valreg;
        }
        else assert( 0 );
    }

    if( theRule == funcinvokenocomma_genrule )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 2 )
        {
            Reached();
            instruction->ax = valreg;
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            void *tmp = realloc(
                instruction->fargs,
                (instruction->fargn + 1) *
                sizeof(struct value_nativeobj));

            if( !tmp )
            {
                CxingFatal("Call to realloc failed when trying to allocate "
                           "space for function arguments.");
                goto func_exec_abort;
            }

            Reached();
            instruction->fargs = tmp;

            valreg = ValueCopy(valreg);
            instruction->fargs[
                instruction->fargn ++] = valreg;
        }
        else assert( 0 );
    }

    if( theRule == funccall_somearg )
    {
        Reached();
        if( instruction->operand_index <
            instruction->node_body->terms_count )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            eprintf("calling func %p with %d args.\n",
                    valreg.proper.p, instruction->fargn);
            if( valreg.type->typeid == valtyp_subr )
            {
                instruction->fargs[0] = (struct value_nativeobj){
                    .proper.p = NULL,
                    .type = (const void *)&type_nativeobj_morgoth };
                instruction->ax =
                    ((cxing_call_proto)valreg.proper.p)(
                        instruction->fargn - 1,
                        instruction->fargs + 1);

                while( instruction->fargn --> 0 )
                {
                    ValueDestroy(instruction->fargs[instruction->fargn]);
                }
                free(instruction->fargs);
                instruction->fargs = NULL;
            }
            else if( valreg.type->typeid == valtyp_method )
            {
                instruction->fargs[0] = instruction->bx;
                instruction->ax =
                    ((cxing_call_proto)valreg.proper.p)(
                        instruction->fargn,
                        instruction->fargs);

                while( instruction->fargn --> 1 )
                {
                    ValueDestroy(instruction->fargs[instruction->fargn]);
                }
                free(instruction->fargs);
                instruction->fargs = NULL;
            }
            else
            {
                CxingDebug("A function call is made to "
                           "a non-function value 01.");
            }
        }
        else assert( 0 );
    }

    if( theRule == funccall_noarg )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            eprintf("calling func %p with %d args.\n",
                    valreg.proper.p, instruction->fargn);
            if( valreg.type->typeid == valtyp_subr )
            {
                instruction->ax =
                    ((cxing_call_proto)valreg.proper.p)(0, NULL);
            }
            else if( valreg.type->typeid == valtyp_method )
            {
                instruction->ax =
                    ((cxing_call_proto)valreg.proper.p)(
                        1, &instruction->bx);
            }
            else
            {
                CxingDebug("A function call is made to "
                           "a non-function value 02.");
            }
        }
        else assert( 0 );
    }

    //
    // expressions.

    if( theRule == unary_dec ||
        theRule == postfix_dec)
    {
        Reached();
        if( instruction->operand_index <
            instruction->node_body->terms_count )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            instruction->ax = DecrementExpr(
                varreg, theRule == postfix_dec);
        }
        else assert( 0 );
    }

    if( theRule == unary_inc ||
        theRule == postfix_inc )
    {
        Reached();
        if( instruction->operand_index <
            instruction->node_body->terms_count )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            instruction->ax = IncrementExpr(
                varreg, theRule == postfix_inc);
        }
        else assert( 0 );
    }

    if( theRule == unary_positive ||
        theRule == unary_negative ||
        theRule == unary_bitcompl ||
        theRule == unary_logicnot )
    {
        Reached();
        if( instruction->operand_index <
            instruction->node_body->terms_count )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            if( theRule == unary_positive )
                instruction->ax = PositiveExpr(valreg);

            if( theRule == unary_negative )
                instruction->ax = NegativeExpr(valreg);

            if( theRule == unary_bitcompl )
                instruction->ax = BitComplExpr(valreg);

            if( theRule == unary_logicnot )
                instruction->ax = LogicNotExpr(valreg);
        }
        else assert( 0 );
    }

    if( theRule == bitor_bitor ||
        theRule == bitxor_bitxor ||
        theRule == bitand_bitand ||

        theRule == eqops_eq ||
        theRule == eqops_ne ||
        theRule == eqops_ideq ||
        theRule == eqops_idne ||

        theRule == relops_lt ||
        theRule == relops_gt ||
        theRule == relops_le ||
        theRule == relops_ge ||

        theRule == shiftexpr_lshift ||
        theRule == shiftexpr_arshift ||
        theRule == shiftexpr_rshift ||

        theRule == addexpr_add ||
        theRule == addexpr_subtract ||
        theRule == mulexpr_multiply ||
        theRule == mulexpr_divide ||
        theRule == mulexpr_remainder )
    {
        Reached();

        if( instruction->operand_index == 0 )
        {
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 2 )
        {
            if( varreg.key )
            {
                // 2026-01-18:
                // There are 2 operands in this expression.
                // The left operand would be an lvalue should
                // this conditional block be entered, and
                // when it does, set this option so that
                // `ValueDestroy` is not called on it.
                // The right operand is managed by the epilogue.
                instruction->opts = ast_node_lvalue_operand;
            }

            instruction->ax = valreg;
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            struct value_nativeobj newval;

            if( theRule == bitor_bitor )
                newval = BitOrExpr(instruction->ax, valreg);

            if( theRule == bitxor_bitxor )
                newval = BitXorExpr(instruction->ax, valreg);

            if( theRule == bitand_bitand )
                newval = BitAndExpr(instruction->ax, valreg);


            if( theRule == eqops_eq )
                newval = LooseEqualityExpr(instruction->ax, valreg);

            if( theRule == eqops_ideq )
                newval = StrictEqualityExpr(instruction->ax, valreg);

            if( theRule == eqops_ne )
                newval = LogicNotExpr(
                    LooseEqualityExpr(instruction->ax, valreg));

            if( theRule == eqops_idne )
                newval = LogicNotExpr(
                    StrictEqualityExpr(instruction->ax, valreg));


            if( theRule == relops_lt )
                newval = LessThanExpr(instruction->ax, valreg);

            if( theRule == relops_gt )
                newval = GreaterThanExpr(instruction->ax, valreg);

            if( theRule == relops_le )
                newval = LessEqaulExpr(instruction->ax, valreg);

            if( theRule == relops_ge )
                newval = GreaterEqaulExpr(instruction->ax, valreg);


            if( theRule == shiftexpr_lshift )
                newval = BitLShiftExpr(instruction->ax, valreg);

            if( theRule == shiftexpr_arshift )
                newval = BitARShiftExpr(instruction->ax, valreg);

            if( theRule == shiftexpr_rshift )
                newval = BitRShiftExpr(instruction->ax, valreg);


            if( theRule == addexpr_add )
                newval = ArithAddExpr(instruction->ax, valreg);

            if( theRule == addexpr_subtract )
                newval = ArithSubExpr(instruction->ax, valreg);

            if( theRule == mulexpr_multiply )
                newval = ArithMulExpr(instruction->ax, valreg);

            if( theRule == mulexpr_divide )
                newval = ArithDivExpr(instruction->ax, valreg);

            if( theRule == mulexpr_remainder )
                newval = ArithModExpr(instruction->ax, valreg);

            if( instruction->opts != ast_node_lvalue_operand )
                ValueDestroy(instruction->ax);
            instruction->ax = newval;
        }
    }

    if( theRule == postfix_nullcoalesce ||
        theRule == logicor_nullcoalesce )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 2 )
        {
            if( IsNullish(valreg) )
            {
                PcStack_PushOrAbandon();
            }
            else
            {
                instruction->ax = valreg;
                instruction->operand_index =
                    instruction->node_body->terms_count;
            }
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            instruction->ax = valreg;
        }
        else assert( 0 );
    }

    if( theRule == logicor_logicor ||
        theRule == logicand_logicand )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 2 )
        {
            if( (theRule == logicand_logicand) ==
                ValueNativeObj2Logic(valreg) )
            {
                PcStack_PushOrAbandon();
            }
            else
            {
                instruction->ax = valreg;
                instruction->operand_index =
                    instruction->node_body->terms_count;
            }
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            instruction->ax = valreg;
        }
        else assert( 0 );
    }

    if( theRule == assignment_directassign ||
        theRule == assignment_mulassign ||
        theRule == assignment_divassign ||
        theRule == assignment_remassign ||
        theRule == assignment_addassign ||
        theRule == assignment_subassign ||
        theRule == assignment_lshiftassign ||
        theRule == assignment_arshiftassign ||
        theRule == assignment_rshiftassign ||
        theRule == assignment_andassign ||
        theRule == assignment_xorassign ||
        theRule == assignment_orassign )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            // Evaluate the right-hand-side value first.
            Reached();
            instruction->operand_index = 1;
            if( !PcStackPush(
                    &pc, instruction->node_body
                    ->terms[2].production) )
            {
                goto func_exec_abort;
            }
            else goto start_eval_1term;
        }
        else if( instruction->operand_index == 2 )
        {
            // Then, resolve the lvalue.
            Reached();

            if( varreg.key )
            {
                // See note dated 2026-01-18 in binary expressions' processor.
                instruction->opts = ast_node_lvalue_operand;
            }

            instruction->ax = valreg;
            eprintf("rhs: %llu t=%lli\n",
                    instruction->ax.proper.u,
                    instruction->ax.type->typeid);
            if( !PcStackPush(
                    &pc, instruction->node_body
                    ->terms[0].production) )
            {
                goto func_exec_abort;
            }
            else goto start_eval_1term;
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count &&
                 instruction->node_body->terms_count == 3 )
        {
            struct value_nativeobj newval;
            Reached();

            if( !varreg.key )
            {
                CxingDebug("The left-hand side of the assignment operation "
                           "is not an lvalue!");
                goto finish_eval_1term;
            }
            else eprintf("The key is: %s.\n", (const char *)s2data_weakmap(varreg.key));

            eprintf("asn: %llu t=%lli\n",
                    instruction->ax.proper.u,
                    instruction->ax.type->typeid);
            if( theRule == assignment_directassign )
                newval = instruction->ax;

            if( theRule == assignment_mulassign )
                newval = ArithMulExpr(valreg, instruction->ax);

            if( theRule == assignment_divassign )
                newval = ArithDivExpr(valreg, instruction->ax);

            if( theRule == assignment_remassign )
                newval = ArithModExpr(valreg, instruction->ax);

            if( theRule == assignment_addassign )
                newval = ArithAddExpr(valreg, instruction->ax);

            if( theRule == assignment_subassign )
                newval = ArithSubExpr(valreg, instruction->ax);

            if( theRule == assignment_lshiftassign )
                newval = BitLShiftExpr(valreg, instruction->ax);

            if( theRule == assignment_arshiftassign )
                newval = BitARShiftExpr(valreg, instruction->ax);

            if( theRule == assignment_rshiftassign )
                newval = BitRShiftExpr(valreg, instruction->ax);

            if( theRule == assignment_andassign )
                newval = BitAndExpr(valreg, instruction->ax);

            if( theRule == assignment_xorassign )
                newval = BitXorExpr(valreg, instruction->ax);

            if( theRule == assignment_orassign )
                newval = BitOrExpr(valreg, instruction->ax);

            eprintf("lvalue: scope=%p, key=%p.\n", varreg.scope.proper.p, varreg.key);

            newval = SetValProperty(varreg.scope, (struct value_nativeobj){
                    .proper.p = varreg.key,
                    .type = (const void *)&type_nativeobj_s2impl_str
                }, newval);

            if( instruction->opts != ast_node_lvalue_operand )
                ValueDestroy(instruction->ax);
            instruction->ax = newval;
            goto finish_eval_1term;
        }
    }

    if( theRule == exprlist_exprlist )
    {
        Reached();
        if( instruction->operand_index <
            instruction->node_body->terms_count )
        {
            PcStack_PushOrAbandon();
        }
        else instruction->ax = valreg;
    }

    //
    // phrases.

    if( theRule == ctrl_flow_ion_op_or ||
        theRule == ctrl_flow_ion_op_nc ||
        theRule == ctrl_flow_ion_labelledop_or ||
        theRule == ctrl_flow_ion_labelledop_nc ||
        theRule == ctrl_flow_molecule_op ||
        theRule == ctrl_flow_molecule_labelledop )
    {
        Reached();
        lalr_rule_t rs = theRule;

        long optype =
            instruction->node_body
            ->terms[0].production->semantic_rule;

        bool labelledop =
            theRule == ctrl_flow_ion_labelledop_or ||
            theRule == ctrl_flow_ion_labelledop_nc ||
            theRule == ctrl_flow_molecule_labelledop;

        lex_token_t *label = labelledop ?
            instruction->node_body
            ->terms[1].production // the label,
            ->terms[0].terminal : // the terminal identifier.
            NULL; // not a labelled control-flow phrase.

        while( true )
        {
            Reached();
            if( labelledop )
            {
                assert( pc.spind > 1 );
                if( rs == while_rule ||
                    rs == dowhile_rule ||
                    rs == for_forever ||
                    rs == for_iterated ||
                    rs == for_conditioned ||
                    rs == for_controlled ||
                    rs == for_initonly ||
                    rs == for_nocond ||
                    rs == for_noiter ||
                    rs == for_classic ||
                    rs == for_vardecl ||
                    rs == for_vardecl_nocond ||
                    rs == for_vardecl_noiter ||
                    rs == for_vardecl_controlled )
                {
                    if( s2data_cmp(instruction[-1].node_body
                                   ->terms[0].production
                                   ->terms[0].terminal->str,
                                   label->str) == 0 )
                        break;
                }
            }
            else
            {
                if( rs == while_rule ||
                    rs == dowhile_rule ||
                    rs == for_forever ||
                    rs == for_iterated ||
                    rs == for_conditioned ||
                    rs == for_controlled ||
                    rs == for_initonly ||
                    rs == for_nocond ||
                    rs == for_noiter ||
                    rs == for_classic ||
                    rs == for_vardecl ||
                    rs == for_vardecl_nocond ||
                    rs == for_vardecl_noiter ||
                    rs == for_vardecl_controlled )
                    break;
            }
            Reached();
            ValueDestroy(PcStackPop(&pc));
            instruction = &pc.instructions[pc.spind];
            rs = theRule;
            Reached();
        }

        if( rules(optype) == flowctrlop_break )
        {
            instruction->flags = ast_node_action_break;
            instruction->operand_index = instruction->node_body->terms_count;
            goto start_eval_1term;
        }
        else if( rules(optype) == flowctrlop_continue )
        {
            instruction->flags = ast_node_action_continue;
            if( rs == while_rule )
            {
                // the controlling condition expression.
                instruction->operand_index = 2;
            }
            else if( rs == dowhile_rule )
            {
                // the controlling condition expression.
                instruction->operand_index = 6;
            }
            else if( rs == for_vardecl )
            {
                // the incrementation expression.
                instruction->operand_index = 6;
            }
            else if( rs == for_forever ||
                     rs == for_iterated ||
                     rs == for_conditioned )
            {
                // The semicolon before the condition expression.
                instruction->operand_index = 2;
            }
            else if( rs == for_initonly || // the loop body,
                     rs == for_nocond || // the incrementation expression
                     rs == for_noiter || // the controlling condition
                     rs == for_vardecl_nocond || // the incrementation expr
                     rs == for_vardecl_noiter ) // the controlling cond.
            {
                instruction->operand_index = 3;
            }
            else if( rs == for_controlled )
            {
                instruction->operand_index = 4;
            }
            else if( rs == for_classic ||
                     rs == for_vardecl_controlled )
            {
                instruction->operand_index = 5;
            }
            else assert( 0 );
            goto start_eval_1term;
        }
        else assert( 0 );
    }

    if( theRule == ctrl_flow_ion_returnnull_or ||
        theRule == ctrl_flow_ion_returnnull_nc ||
        theRule == ctrl_flow_molecule_returnnull )
    {
        Reached();
        pc.instructions[0].ax = (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
        pc.instructions[0].flags = ast_node_action_return;
        goto finish_eval_1term;
    }

    if( theRule == ctrl_flow_ion_returnexpr_or ||
        theRule == ctrl_flow_ion_returnexpr_nc ||
        theRule == ctrl_flow_molecule_returnexpr )
    {
        Reached();

        if( instruction->operand_index == 1 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            eprintf("ax-val: %lld\n", valreg.proper.u);
            pc.instructions[0].ax = valreg;
            pc.instructions[0].flags = ast_node_action_return;
            goto finish_eval_1term;
        }
    }

    if( theRule == and_phrase_ion_and ||
        theRule == and_phrase_ion_then ||
        theRule == or_phrase_ion_or ||
        theRule == or_phrase_ion_nc )
    {
        Reached();

        if( instruction->operand_index == 0 )
        {
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            if( theRule == and_phrase_ion_then &&
                IsNullish(valreg) )
            {
                Reached();
                instruction->flags = ast_node_action_stop;
            }
            else if( theRule == or_phrase_ion_nc &&
                     !IsNullish(valreg) )
            {
                Reached();
                instruction->flags = ast_node_action_stop;
            }
            else if( theRule == and_phrase_ion_and &&
                     !ValueNativeObj2Logic(valreg) )
            {
                Reached();
                instruction->flags = ast_node_action_stop;
            }
            else if( theRule == or_phrase_ion_or &&
                     ValueNativeObj2Logic(valreg) )
            {
                Reached();
                instruction->flags = ast_node_action_stop;
            }

            instruction->ax = valreg;
        }
    }

    if( theRule == phrase_stmt_base )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            instruction->ax = valreg;
            goto finish_eval_1term;
        }
        else assert( 0 );
    }

    if( theRule == or_phrase_atom_atomize ||
        theRule == and_phrase_atom_atomize ||
        theRule == phrase_stmt_conj_ctrl_flow ||
        theRule == phrase_stmt_disj_ctrl_flow )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 1 )
        {
            if( ast_node_action_stop == instruction->flags ||
                ast_node_action_return == pc.instructions[0].flags )
            {
                Reached();
                instruction->ax = valreg;
                goto finish_eval_1term;
            }
            else
            {
                Reached();
                PcStack_PushOrAbandon();
            }
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            instruction->ax = valreg;
            goto finish_eval_1term;
        }
        else assert( 0 );
    }

    if( theRule == or_phrase_ion_ctrl_flow )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 1 )
        {
            if( ast_node_action_stop == instruction->flags ||
                ast_node_action_return == pc.instructions[0].flags )
            {
                Reached();
                instruction->ax = valreg;

                if( instruction->flags == ast_node_action_stop )
                {
                    lalr_prod_t *cntl =
                        instruction->node_body
                        ->terms[1].production;
                    s2data_t *disj_op =
                        cntl->terms[cntl->terms_count - 1].terminal->str;

                    if( strcmp(s2data_weakmap(disj_op), "_Fallback") == 0 &&
                        !IsNullish(valreg) )
                    {
                        Reached();
                        // Here, the disjunction short-circuit completes
                        // due to a non-nullish value being present at the
                        // left-hand side of a nullish-coalescing operator.
                    }
                    else
                    {
                        Reached();
                        instruction->flags = ast_node_action_default;
                    }
                }
                goto finish_eval_1term;
            }
            else
            {
                Reached();
                PcStack_PushOrAbandon();
            }
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            instruction->ax = valreg;
            goto finish_eval_1term;
        }
        else assert( 0 );
    }

    //
    // declarations.

    if( theRule == decl_singledecl ||
        theRule == decl_singledeclinit ||
        theRule == decl_declarelist1 ||
        theRule == decl_declarelist2 )
    {
        Reached();
        lalr_rule_t rs = theRule;
        size_t sfind = pc.spind; // stack frame index / (braced) statement find.
        s2cxing_value_t *rval; // right-hand side value of the assignment.
        unsigned opind;

        if( theRule == decl_declarelist1 ||
            theRule == decl_declarelist2 )
        {
            if( instruction->operand_index == 0 )
            {
                PcStack_PushOrAbandon();
            }
            opind = 2;
        }
        else opind = 1;

        if( theRule == decl_singledeclinit ||
            theRule == decl_declarelist2 )
        {
            if( instruction->operand_index == opind + 2 )
            {
                PcStack_PushOrAbandon();
            }
            else if( instruction->operand_index <
                     instruction->node_body->terms_count )
            {
                instruction->operand_index ++;
                goto start_eval_1term;
            }
        }

        while( true )
        {
            if( rs == for_vardecl ||
                rs == for_vardecl_nocond ||
                rs == for_vardecl_noiter ||
                rs == for_vardecl_controlled ||
                rs == stmt_brace )
                break;

            sfind --;
            rs = rules(pc.instructions[sfind].node_body->semantic_rule);
        }

        if( !pc.instructions[sfind].vardecls )
            pc.instructions[sfind].vardecls = s2dict_create();

        if( !pc.instructions[sfind].vardecls )
        {
            CxingFatal("Variable declaration resource allocation failure 01.");
            goto func_exec_abort;
        }

        if( theRule == decl_singledeclinit ||
            theRule == decl_declarelist2 )
        {
            Reached();
            // 2026-01-18: Setting to scopes require copying, to add `ValueCopy` - did?.
            if( !(rval = s2cxing_value_create(ValueCopy(valreg))) )
            {
                CxingFatal(
                    "Variable declaration resource allocation failure 02.");
                goto func_exec_abort;
            }
        }
        else
        {
            if( !(rval = s2cxing_value_create((struct value_nativeobj){
                            .proper.p = NULL,
                            .type = (const void *)&type_nativeobj_morgoth,
                        })) )
            {
                CxingFatal(
                    "Variable declaration resource allocation failure 03.");
                goto func_exec_abort;
            }
        }

        eprintf("%d sfind: %zd\n", __LINE__, sfind);
        eprintf("%d: %p %p\n", __LINE__, rval, rval->pobj);

        if( s2dict_set(
                pc.instructions[sfind].vardecls,
                instruction->node_body
                ->terms[opind].production
                ->terms[0].terminal->str,
                rval->pobj, s2_setter_gave) != s2_access_success )
        {
            CxingFatal("Variable declaration failure.");
            goto func_exec_abort;
        }
    }

    //
    // Type Definition and Object Initialization Syntax

    if( theRule == objdefstartnocomma_genrule )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            // nop.
        }
        else assert( 0 );
    }

    if( theRule == objdefstartnocomma_base ||
        theRule == objdefstartnocomma_genrule )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }

        else if( instruction->operand_index == 2 )
        {
            Reached();

            if( theRule == objdefstartnocomma_base )
            {
                // for the duration of obj-def,
                // `instruction->bx` is the type object.
                instruction->bx = valreg;
            }

            PcStack_PushOrAbandon();
        }

        else if( instruction->operand_index == 4 )
        {
            Reached();
            instruction->ax = valreg;
            PcStack_PushOrAbandon();
        }

        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            struct value_nativeobj InitSetMethod =
                // A possibility is considered where
                // `__initset__` member is replaced
                // during the evaluation of the notation.
                GetValProperty(
                    instruction->bx,
                    CxingPropName_InitSet).value;

            if( InitSetMethod.type->typeid != valtyp_method )
            {
                if( theRule == objdefstartnocomma_base )
                    CxingDiagnose("The postfix expression identified by "
                                  "the token at line %d column %d "
                                  "in source code file \"%s\" "
                                  "is not a method as required by "
                                  "object definition notation.\n",
                                  instruction->node_body
                                  ->terms[0].terminal->lineno,
                                  instruction->node_body
                                  ->terms[0].terminal->column,
                                  module->filename);
                else CxingDiagnose("The postfix expression used "
                                   "in the object definition notation "
                                   "was not a method.");
            }
            else
            {
                struct value_nativeobj args[3] = {
                    instruction->bx,
                    instruction->ax,
                    valreg };
                ((cxing_call_proto)InitSetMethod.proper.p)(
                    3, args);
            }
        }

        else assert( 0 );
    }

    if( theRule == objdef_some ||
        theRule == objdef_empty )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }

        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            struct value_nativeobj InitSetMethod =
                // A possibility is considered where
                // `__initset__` member is replaced
                // during the evaluation of the notation.
                GetValProperty(
                    instruction->bx,
                    CxingPropName_InitSet).value;

            if( InitSetMethod.type->typeid != valtyp_method )
            {
                CxingDiagnose("The postfix expression used "
                              "in the object definition notation "
                              "was not a method.");
            }
            else
            {
                struct value_nativeobj args[3] = {
                    instruction->bx,
                    CxingPropName_Proto,
                    instruction->bx };
                ((cxing_call_proto)InitSetMethod.proper.p)(
                    3, args);
            }
        }

        else assert( 0 );
    }

    // auto-indexed arrays.

    if( theRule == array_piece_base )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }

        else if( instruction->operand_index == 2 )
        {
            Reached();

            // for the duration of obj-def,
            // `instruction->bx` is the type object.
            instruction->bx = valreg;

            // zero-based indexing.
            instruction->ax.proper.l = 0;
            instruction->ax.type = (const void *)&type_nativeobj_long;

            PcStack_PushOrAbandon();
        }

        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            struct value_nativeobj InitSetMethod =
                // A possibility is considered where
                // `__initset__` member is replaced
                // during the evaluation of the notation.
                GetValProperty(
                    instruction->bx,
                    CxingPropName_InitSet).value;

            if( InitSetMethod.type->typeid != valtyp_method )
            {
                CxingDiagnose("The postfix expression identified by "
                              "the token at line %d column %d "
                              "in source code file \"%s\" "
                              "is not a method as required by "
                              "object definition notation.\n",
                              instruction->node_body
                              ->terms[0].terminal->lineno,
                              instruction->node_body
                              ->terms[0].terminal->column,
                              module->filename);
            }
            else
            {
                struct value_nativeobj args[3] = {
                    instruction->bx,
                    instruction->ax,
                    valreg };
                ((cxing_call_proto)InitSetMethod.proper.p)(
                    3, args);
            }
        }

        else assert( 0 );
    }

    if( theRule == array_piece_genrule )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }

        else if( instruction->operand_index == 1 )
        {
            Reached();

            // for the duration of obj-def,
            // `instruction->bx` is the type object.
            // ---

            // zero-based indexing.
            instruction->ax = valreg;
            instruction->ax.proper.l ++;

            PcStack_PushOrAbandon();
        }

        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            struct value_nativeobj InitSetMethod =
                // A possibility is considered where
                // `__initset__` member is replaced
                // during the evaluation of the notation.
                GetValProperty(
                    instruction->bx,
                    CxingPropName_InitSet).value;

            if( InitSetMethod.type->typeid != valtyp_method )
            {
                CxingDiagnose("The postfix expression used "
                              "in the object definition notation "
                              "was not a method.");
            }
            else
            {
                struct value_nativeobj args[3] = {
                    instruction->bx,
                    instruction->ax,
                    valreg };
                ((cxing_call_proto)InitSetMethod.proper.p)(
                    3, args);
            }
        }

        else assert( 0 );
    }

    if( theRule == array_streamline )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }

        else if( instruction->operand_index == 1 )
        {
            Reached();

            // for the duration of obj-def,
            // `instruction->bx` is the type object.
            // ---

            // zero-based indexing.
            instruction->ax = valreg;
            instruction->ax.proper.l ++;

            PcStack_PushOrAbandon();
        }

        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            struct value_nativeobj InitSetMethod =
                // A possibility is considered where
                // `__initset__` member is replaced
                // during the evaluation of the notation.
                GetValProperty(
                    instruction->bx,
                    CxingPropName_InitSet).value;

            if( InitSetMethod.type->typeid != valtyp_method )
            {
                CxingDiagnose("The postfix expression used "
                              "in the object definition notation "
                              "was not a method.");
            }
            else
            {
                struct value_nativeobj args[3] = {
                    instruction->bx,
                    instruction->ax,
                    valreg };
                ((cxing_call_proto)InitSetMethod.proper.p)(
                    3, args);

                // a 2nd call for end of list
                args[1] = CxingPropName_Proto;
                args[2] = instruction->bx;
                ((cxing_call_proto)InitSetMethod.proper.p)(
                    3, args);
            }
        }

        else assert( 0 );
    }

    if( theRule == array_complete )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }

        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            struct value_nativeobj InitSetMethod =
                // A possibility is considered where
                // `__initset__` member is replaced
                // during the evaluation of the notation.
                GetValProperty(
                    instruction->bx,
                    CxingPropName_InitSet).value;

            if( InitSetMethod.type->typeid != valtyp_method )
            {
                CxingDiagnose("The postfix expression used "
                              "in the object definition notation "
                              "was not a method.");
            }
            else
            {
                struct value_nativeobj args[3] = {
                    instruction->bx,
                    CxingPropName_Proto,
                    instruction->bx };
                ((cxing_call_proto)InitSetMethod.proper.p)(
                    3, args);
            }
        }

        else assert( 0 );
    }

    //
    // statements.

    if( theRule == stmt_decl ||
        theRule == stmt_brace ||
        theRule == stmtlist_base ||
        theRule == stmtlist_genrule )
    {
        Reached();
        if( instruction->operand_index <
            instruction->node_body->terms_count )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
    }

    if( theRule == predclause_base )
    {
        Reached();
        if( instruction->operand_index == 2 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 4 )
        {
            if( ValueNativeObj2Logic(valreg) )
            {
                PcStack_PushOrAbandon();
                instruction->flags = ast_node_action_noelse;
            }
        }
    }

    if( theRule == predclause_genrule )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 3 )
        {
            Reached();
            if( instruction->flags == ast_node_action_noelse )
            {
                instruction->operand_index =
                    instruction->node_body->terms_count;
                goto finish_eval_1term;
            }
            else
            {
                Reached();
                PcStack_PushOrAbandon();
            }
        }
        else if( instruction->operand_index == 5 )
        {
            Reached();
            if( ValueNativeObj2Logic(valreg) )
            {
                PcStack_PushOrAbandon();
                instruction->flags = ast_node_action_noelse;
            }
        }
        else assert( 0 );
    }

    if( theRule == condstmt_else )
    {
        Reached();
        if( instruction->operand_index == 0 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 2 )
        {
            Reached();
            if( instruction->flags == ast_node_action_noelse )
            {
                instruction->operand_index =
                    instruction->node_body->terms_count;
                goto finish_eval_1term;
            }
            else
            {
                Reached();
                PcStack_PushOrAbandon();
            }
        }
        else assert( 0 );
    }

    if( theRule == while_rule )
    {
        Reached();
        if( instruction->operand_index == 2 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 4 )
        {
            if( ValueNativeObj2Logic(valreg) )
            {
                Reached();
                PcStack_PushOrAbandon();
            }
            else instruction->operand_index ++;
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            if( instruction->flags == ast_node_action_break )
            {
                goto finish_eval_1term;
            }
            else
            {
                instruction->operand_index = 0;
                goto start_eval_1term;
            }
        }
        // else do not assert,
        // because that operand_index incrementation
        // depends on this to break out of the loop.
    }

    if( theRule == dowhile_rule )
    {
        Reached();
        if( instruction->operand_index == 2 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 6 )
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();

            if( instruction->flags == ast_node_action_break )
                goto finish_eval_1term;

            if( !ValueNativeObj2Logic(valreg) )
                goto finish_eval_1term;

            instruction->operand_index = 0;
            goto start_eval_1term;
        }
        else assert( 0 );
    }

    if( theRule == for_forever )
    {
        Reached();
        if( instruction->operand_index == 5 ) // the body clause.
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            if( instruction->flags == ast_node_action_break )
            {
                goto finish_eval_1term;
            }
            else
            {
                instruction->operand_index = 0; // start allover.
                goto start_eval_1term;
            }
        }
        else assert( 0 );
    }

    if( theRule == for_initonly ||
        theRule == for_vardecl )
    {
        Reached();
        if( instruction->operand_index == 2 ) // the init clause.
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 6 ) // the body clause.
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            if( instruction->flags == ast_node_action_break )
            {
                goto finish_eval_1term;
            }
            else
            {
                instruction->flags = ast_node_action_continue;
                instruction->operand_index = 5; // start again just before body.
                goto start_eval_1term;
            }
        }
        else assert( 0 );
    }

    if( theRule == for_iterated )
    {
        Reached();
        if( instruction->operand_index == 4 ) // the increment clause.
        {
            if( instruction->flags != ast_node_action_continue )
            {
                Reached();
                instruction->operand_index ++;
                goto start_eval_1term;
            }
            else
            {
                Reached();
                PcStack_PushOrAbandon();
            }
        }
        else if( instruction->operand_index == 6 ) // the body clause.
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            if( instruction->flags == ast_node_action_break )
            {
                goto finish_eval_1term;
            }
            else
            {
                instruction->flags = ast_node_action_continue;
                instruction->operand_index = 3; // proceed to just before incr.
                goto start_eval_1term;
            }
        }
        else assert( 0 );
    }

    if( theRule == for_conditioned )
    {
        Reached();
        if( instruction->operand_index == 3 ) // the condition clause.
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        if( instruction->operand_index == 6 ) // the body clause.
        {
            if( ValueNativeObj2Logic(valreg) )
            {
                Reached();
                PcStack_PushOrAbandon();
            }
            else instruction->operand_index ++;
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            if( instruction->flags == ast_node_action_break )
            {
                goto finish_eval_1term;
            }
            else
            {
                instruction->operand_index = 0; // start allover.
                goto start_eval_1term;
            }
        }
        // else do not assert,
        // because that operand_index incrementation
        // depends on this to break out of the loop.
    }

    if( theRule == for_controlled )
    {
        Reached();
        if( instruction->operand_index == 3 ) // the condition clause.
        {
            Reached();
            instruction->operand_index = 6; // jump just before the body ..
            if( !PcStackPush(
                    &pc,
                    instruction->node_body
                    ->terms[3].production) ) // .. after evaluating cond.
            {
                goto func_exec_abort;
            }
            else
            {
                goto start_eval_1term;
            }
        }
        else if( instruction->operand_index == 5 ) // the increment clause.
        {
            Reached();
            instruction->operand_index = 2; // jump just before the cond ..
            if( instruction->flags == ast_node_action_continue )
            {
                if( !PcStackPush(
                        &pc,
                        instruction->node_body
                        ->terms[5].production) ) // .. after evaluating incr.
                {
                    goto func_exec_abort;
                }
                Reached();
            }
            eprintf("opind: %d.\n", instruction->operand_index);
            goto start_eval_1term;
        }
        else if( instruction->operand_index == 7 ) // the body clause.
        {
            if( ValueNativeObj2Logic(valreg) )
            {
                Reached();
                PcStack_PushOrAbandon();
            }
            else instruction->operand_index ++;
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            if( instruction->flags == ast_node_action_break )
            {
                goto finish_eval_1term;
            }
            else
            {
                instruction->flags = ast_node_action_continue;
                instruction->operand_index = 4; // proceed to just before incr.
                goto start_eval_1term;
            }
        }
        // else do not assert,
        // because that operand_index incrementation
        // depends on this to break out of the loop.
    }

    if( theRule == for_classic ||
        theRule == for_vardecl_controlled )
    {
        Reached();
        if( instruction->operand_index == 2 ) // the init clause.
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 4 ) // the condition clause.
        {
            Reached();
            instruction->operand_index = 7; // jump just before the body ..
            if( !PcStackPush(
                    &pc,
                    instruction->node_body
                    ->terms[4].production) ) // .. after evaluating cond.
            {
                goto func_exec_abort;
            }
            else
            {
                goto start_eval_1term;
            }
        }
        else if( instruction->operand_index == 6 ) // the increment clause.
        {
            Reached();
            instruction->operand_index = 3; // jump just before the cond ..
            if( instruction->flags == ast_node_action_continue )
            {
                if( !PcStackPush(
                        &pc,
                        instruction->node_body
                        ->terms[6].production) ) // .. after evaluating incr.
                {
                    goto func_exec_abort;
                }
            }
            goto start_eval_1term;
        }
        else if( instruction->operand_index == 8 ) // the body clause.
        {
            if( ValueNativeObj2Logic(valreg) )
            {
                Reached();
                PcStack_PushOrAbandon();
            }
            else instruction->operand_index ++;
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            if( instruction->flags == ast_node_action_break )
            {
                goto finish_eval_1term;
            }
            else
            {
                instruction->flags = ast_node_action_continue;
                instruction->operand_index = 5; // proceed to just before incr.
                goto start_eval_1term;
            }
        }
        // else do not assert,
        // because that operand_index incrementation
        // depends on this to break out of the loop.
    }

    if( theRule == for_nocond ||
        theRule == for_vardecl_nocond )
    {
        Reached();
        if( instruction->operand_index == 2 ) // the init clause.
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 5 ) // the increment clause.
        {
            Reached();
            instruction->operand_index = 6; // jump just before the body ..
            if( instruction->flags == ast_node_action_continue )
            {
                if( !PcStackPush(
                        &pc,
                        instruction->node_body
                        ->terms[5].production) ) // .. after evaluating incr.
                {
                    goto func_exec_abort;
                }
            }
            goto start_eval_1term;
        }
        else if( instruction->operand_index == 7 ) // the body clause.
        {
            if( ValueNativeObj2Logic(valreg) )
            {
                Reached();
                PcStack_PushOrAbandon();
            }
            else instruction->operand_index ++;
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            if( instruction->flags == ast_node_action_break )
            {
                goto finish_eval_1term;
            }
            else
            {
                instruction->flags = ast_node_action_continue;
                instruction->operand_index = 4; // proceed to just before incr.
                goto start_eval_1term;
            }
        }
        // else do not assert,
        // because that operand_index incrementation
        // depends on this to break out of the loop.
    }

    if( theRule == for_noiter ||
        theRule == for_vardecl_noiter )
    {
        Reached();
        if( instruction->operand_index == 2 ) // the init clause.
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 4 ) // the condition clause.
        {
            Reached();
            instruction->operand_index = 6; // jump just before the body ..
            if( !PcStackPush(
                    &pc,
                    instruction->node_body
                    ->terms[4].production) ) // .. after evaluating cond.
            {
                goto func_exec_abort;
            }
            else
            {
                goto start_eval_1term;
            }
        }
        else if( instruction->operand_index == 7 ) // the body clause.
        {
            if( ValueNativeObj2Logic(valreg) )
            {
                Reached();
                PcStack_PushOrAbandon();
            }
            else instruction->operand_index ++;
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            Reached();
            if( instruction->flags == ast_node_action_break )
            {
                goto finish_eval_1term;
            }
            else
            {
                instruction->flags = ast_node_action_continue;
                instruction->operand_index = 3; // proceed to just before incr.
                goto start_eval_1term;
            }
        }
        // else do not assert,
        // because that operand_index incrementation
        // depends on this to break out of the loop.
    }

finish_eval_1term:
    if( pc.spind == 0 )
    {
        Reached();
        valreg = pc.instructions[0].ax;
        if( instruction->vardecls )
        {
            Reached();
            s2obj_release(instruction->vardecls->pobj);
        }
        Reached();
        free(pc.instructions);
        if( !valreg.type )
        {
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }
        else
        {
            if( varreg.key )
                // This was a new rvalue.
                return ValueCopy(valreg);
            else
                // This wasn't an lvalue.
                return valreg;
        }
    }

    // [Automatic-Resource-Management]:
    // All lvalues (i.e. those varreg with non-NULL `key`)
    // are assumed transient and not explicitly destroyed.
    if( !varreg.key )
    {
        CapturePoint();
        // 2026-01-18:
        // rvalues are destroyed after use.
        ValueDestroy(valreg);
    }
    else if( instruction->opts != ast_node_lvalue_register )
    {
        CapturePoint();
        // 2026-01-18:
        // The production rule that consumed the the lvalue
        // doesn't produce further lvalues.
        varreg.key = NULL;
    }

    valreg = PcStackPop(&pc);
    pc.instructions[pc.spind].operand_index ++;
    goto start_eval_1term;

func_exec_abort:
    PcStackAbandon(&pc);
    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth,
    };
}
