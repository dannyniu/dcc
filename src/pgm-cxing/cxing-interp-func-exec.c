/* DannyNiu/NJF, 2025-12-30. Public Domain. */

#include "cxing-grammar.h"
#include "cxing-interp.h"
#include "runtime.h"
#include "langsem.h"
#include "expr.h"
#include "../lex-common/lex.h"
#include "../infra/rfdict.h"

#define CXING_INTERP_TRACING_LEVEL_MSG 1
#define CXING_INTERP_TRACING_LEVEL_SRC 2
#define CXING_INTERP_TRACING_LEVEL_FSM 3

#define CXING_INTERP_TRACING_LEVEL 0
#if CXING_INTERP_TRACING_LEVEL >= CXING_INTERP_TRACING_LEVEL_MSG
#define eprintf(...) fprintf(stderr, __VA_ARGS__)

#if CXING_INTERP_TRACING_LEVEL >= CXING_INTERP_TRACING_LEVEL_FSM
#define Reached() if( ReachesHere ) eprintf("Reached: flags: %d, opts: %d, spind: %zd, ri: %i; opind: %u. fd: %u, uf: %u. %llx/<%s>/%llx %llx/ax %llx/bx %d (%s:%d!)\n", instruction->flags, instruction->opts, pc.spind, instruction->node_body->semantic_rule, instruction->operand_index, instruction->folded_depth, instruction->unfolded, valreg.proper.l, varreg.key?(const char *)s2data_weakmap(varreg.key):"", varreg.scope.proper.u, instruction->ax.proper.u, instruction->bx.proper.u, metascope, strrchr(__FILE__, '/'), __LINE__);

#define Visited() // if( ReachesHere ) eprintf("Visited: flags: %d, opts: %d, spind: %zd, ri: %i; opind: %u. fd: %u, uf: %u. %llx/<%s>/%llx %llx/ax %llx/bx %d (%s:%d!)\n", instruction->flags, instruction->opts, pc.spind, instruction->node_body->semantic_rule, instruction->operand_index, instruction->folded_depth, instruction->unfolded, valreg.proper.l, varreg.key?(const char *)s2data_weakmap(varreg.key):"", varreg.scope.proper.u, instruction->ax.proper.u, instruction->bx.proper.u, metascope, strrchr(__FILE__, '/'), __LINE__);

#else

#define Reached()
#define Visited()

#endif /* CXING_INTERP_TRACING_LEVEL >= CXING_INTERP_TRACING_LEVEL_FSM */
#else
#define eprintf(...) 0
#define Reached()
#define Visited()
#endif /* CXING_INTERP_TRACING_LEVEL >= CXING_INTERP_TRACING_LEVEL_MSG */

static inline lalr_rule_t rules(int32_t r)
{
    return cxing_grammar_rules[r];
}

#define theRule rules(instruction->node_body->semantic_rule)

//
// MARK: Program Counter (a.k.a. Instruction Pointer) and RPN Stack.

typedef struct cxing_ast_node {
    // evaluation result.
    struct value_nativeobj ax;

    // Usages:
    // 1. `this` argument for method calls.
    // 2. object to be defined by `__initset__`.
    struct value_nativeobj bx;

    // arguments to a function call.
    struct value_nativeobj *fargs;

    lalr_prod_t *node_body; // unowned.
    rfdict_t *vardecls; // owned.

    // Moved here for data structure alignment.
    unsigned fargn;
    unsigned operand_index;

    // 2026-03-04:
    // optimization for statement lists to not to deepen the stack.
    unsigned folded_depth;
    unsigned unfolded;

    enum {
        ast_node_action_default = 0,

        // logic determined, short-circuit.
        ast_node_action_stop,

        // for conditionals, passed-down.
        ast_node_action_noelse,

        // for loops.
        ast_node_action_break,
        ast_node_action_continue,

        // for the function body.
        ast_node_action_return,

        // Flags are inherited across AST nodes.
    } flags;

    enum {
        ast_node_opt_default = 0,

        // 2026-04-07:
        // manually passed down in function calls with arguments.
        ast_node_release_bx_after_call, // calls ValueDestroy.
        ast_node_insulate_bx_after_call, // no ValueDestroy call,

        // 2026-04-08:
        // makes decision for one of the 2 above flags.
        ast_node_scope_was_lvalue,
        ast_node_scope_was_rvalue,

        // preserve the value of `bx` in this frame,
        // and instruct `PcStackPop` not to overwrite it for any reason.
        ast_node_preserve_bx,
        // ast_node_passdown_bx,

        // for conditionals, sticky.
        ast_node_opt_noelse,

        // Options are cleared across AST nodes.
    } opts;
} cxing_ast_node_t;

typedef struct {
    size_t capacity;
    size_t spind; // stack pointer/index.
    cxing_ast_node_t *instructions; // owned.
    enum types_enum function_kind;
} cxing_program_counter_t;

#define STACK_CAPACITY_INCREMENTATION 16

static bool PcStackPush(
    cxing_program_counter_t *pc, lalr_prod_t *instructions)
{
    assert( pc->spind < pc->capacity );

    if( ++pc->spind == pc->capacity )
    {
        void *T = realloc(
            pc->instructions,
            (pc->capacity+STACK_CAPACITY_INCREMENTATION) *
            sizeof(*pc->instructions));
        if( !T )
        {
            --pc->spind;
            CxingFatal("Stack Growing Failed!");
            return false;
        }

        pc->capacity += STACK_CAPACITY_INCREMENTATION;
        pc->instructions = T;
    }

    pc->instructions[pc->spind] = (cxing_ast_node_t){ 0 };
    pc->instructions[pc->spind].node_body = instructions;
    pc->instructions[pc->spind].ax = (struct value_nativeobj){
        .proper.p = NULL, .type = (const void *)&type_nativeobj_morgoth };
    pc->instructions[pc->spind].bx = (struct value_nativeobj){
        .proper.p = NULL, .type = (const void *)&type_nativeobj_morgoth };
    pc->instructions[pc->spind].flags = ast_node_action_default;
    pc->instructions[pc->spind].opts = ast_node_opt_default;
    return true;
}

static struct value_nativeobj PcStackPop(cxing_program_counter_t *pc)
{
    cxing_ast_node_t *instruction;
    assert( pc->spind > 0 );

    instruction = &pc->instructions[pc->spind];

    if( instruction->vardecls )
    {
        s2obj_release(instruction->vardecls->pobj);
        instruction->vardecls = NULL;
    }

    // 2026-04-04: resource management issues handled by rule handlers.
    if( pc->instructions[pc->spind - 1].opts != ast_node_preserve_bx &&
        pc->instructions[pc->spind - 1].opts != ast_node_release_bx_after_call &&
        pc->instructions[pc->spind - 1].opts != ast_node_insulate_bx_after_call )
        pc->instructions[pc->spind - 1].bx = pc->instructions[pc->spind].bx;

    // letting funccall rule processor(s) handle resource management.
    pc->instructions[pc->spind - 1].flags = pc->instructions[pc->spind].flags;
    pc->instructions[pc->spind - 1].fargs = pc->instructions[pc->spind].fargs;
    pc->instructions[pc->spind - 1].fargn = pc->instructions[pc->spind].fargn;
    (void)pc->instructions[pc->spind - 1].opts;

    eprintf("bx was: %llx t=%lli\n",
            pc->instructions[pc->spind].bx.proper.u,
            pc->instructions[pc->spind].bx.type->typeid);
    eprintf("popped: %llx t=%lli\n",
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

//
// MARK: Local Variables Bookkeeping.

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

    // check that it is a method where `this` is referenced.
    if( strcmp(s2data_weakmap(key), "this") == 0 )
    {
        if( localvars->pc->function_kind == valtyp_subr )
        {
            CxingSemanticErr(
                localvars->module,
                "Subroutines cannot reference `this` like methods.\n");
            return (struct value_nativeobj) {
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }
        else return localvars->args[0];
    }

    // first try looking on the stack frame.
    for( ; sfind <= localvars->pc->spind; sfind -- )
    {
        eprintf("%d sfind: %zd\n", __LINE__, sfind);

        if( !localvars->pc->instructions[sfind].vardecls )
            continue;

        accessed = rfdict_get_T(s2cxing_value_t)(
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
        if( localvars->pc->function_kind == valtyp_method )
            // `this` occupies a slot.
            argind ++;
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
        else if( entRule(de) == entdecl_constdef )
        {
            return CXConstDefParse(de->terms[2].production);
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

    // Finally, in the linked and built-in definitions.

    accessed = s2dict_get_T(s2cxing_value_t)(
        localvars->module->linked, key, &ret_wrapped);

    if( accessed == s2_access_success )
    {
        assert( accessed == s2_access_success );
        return ret_wrapped->cxing_value;
    }
    else if( accessed == s2_access_error )
    {
        CxingFatal("Error while getting local variables 03.");
        // Fail as gracefully as possible.
        return (struct value_nativeobj) {
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    accessed = s2dict_get_T(s2cxing_value_t)(CxingBuiltins, key, &ret_wrapped);

    if( accessed == s2_access_success )
    {
        assert( accessed == s2_access_success );
        return ret_wrapped->cxing_value;
    }
    else if( accessed == s2_access_error )
    {
        CxingFatal("Error while getting local variables 04.");
        // Fail as gracefully as possible.
        return (struct value_nativeobj) {
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }
    else
    {
        assert( accessed == s2_access_nullval );
        CxingDebug(
            "The program is trying to read from a non-existing variable: "
            "`%s` Please inform the developer to debug the program.\n",
            (const char *)s2data_weakmap(key));

        return (struct value_nativeobj) {
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
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

        accessed = rfdict_get_T(s2cxing_value_t)(
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

        if( !(ret_wrapped = s2cxing_value_create(ValueCopy(val))) )
        {
            CxingFatal("Error while setting local variables 2.");
            // Fail as gracefully as possible.
            return (struct value_nativeobj) {
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }

        accessed = rfdict_set(
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

    if( localvars->pc->function_kind == valtyp_method )
        // `this` occupies a slot.
        argind ++;

    ValueDestroy(localvars->args[argind]);
    localvars->args[argind] = ValueCopy(val);

    // 2026-01-17 TODO: Modifying an argument parameter. DONE
    // 2026-02-06 TODO (new): Test resource management for modifying arg params.
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

static struct value_nativeobj CxingFuncLocalVar_Get = {
    .proper.p = CxingFuncLocalVar_GetImpl,
    .type = (const void *)&type_nativeobj_method,
};

static struct value_nativeobj CxingFuncLocalVar_Set = {
    .proper.p = CxingFuncLocalVar_SetImpl,
    .type = (const void *)&type_nativeobj_method,
};

static bool CxingFuncLocalVar_IsSetImpl0(
    struct CxingFuncLocalVars *localvars, s2data_t *key)
{
    size_t sfind = localvars->pc->spind;
    s2cxing_value_t *ret_wrapped;
    lalr_prod_t *de; // the definition for the entity.
    int argind = -1, accessed;

    // check that it is a method where `this` is referenced.
    if( strcmp(s2data_weakmap(key), "this") == 0 )
    {
        if( localvars->pc->function_kind == valtyp_subr )
        {
            return false;
        }
        else return true;
    }

    // first try looking on the stack frame.
    for( ; sfind <= localvars->pc->spind; sfind -- )
    {
        eprintf("%d sfind: %zd\n", __LINE__, sfind);

        if( !localvars->pc->instructions[sfind].vardecls )
            continue;

        accessed = rfdict_get_T(s2cxing_value_t)(
            localvars->pc->instructions[sfind].vardecls,
            key, &ret_wrapped);

        if( accessed == s2_access_error )
        {
            CxingFatal("Error while looking up local variables 01.");
            // Fail as gracefully as possible.
            return false;
        }

        if( accessed == s2_access_nullval )
            continue;

        return true;
    }

    // Otherwise, in the arguments.

    argind = CxingFunc_ArgumentIndex(localvars->params, key);

    eprintf("Looked at arguments: %d\n", argind);

    if( argind >= 0 ) // no actual argument: //- `&& argind < localvars->argn`
    {
        // found in arguments.
        return true;
    }

    // Next, in the set of entities.

    accessed = s2dict_get_T(lalr_prod_t)(
        localvars->module->entities, key, &de);

    if( accessed == s2_access_error )
    {
        CxingFatal("Error while looking up local variables 02.");
        // Fail as gracefully as possible.
        return false;
    }

    if( accessed == s2_access_success )
    {
        return true;
    }

    // Finally, in the linked definitions.

    accessed = s2dict_get_T(s2cxing_value_t)(
        localvars->module->linked, key, &ret_wrapped);

    if( accessed == s2_access_error )
    {
        CxingFatal("Error while getting local variables 03.");
        // Fail as gracefully as possible.
        return false;
    }
    else if( accessed == s2_access_nullval )
    {
        return false;
    }
    else
    {
        assert( accessed == s2_access_success );
        return true;
    }
}

static struct TYPE_NATIVEOBJ_STRUCT(3) type_nativeobj_localvars = {
    .typeid = valtyp_obj,
    .n_entries = 2,
    .static_members = {
        { .name = "__get__", .member = &CxingFuncLocalVar_Get },
        { .name = "__set__", .member = &CxingFuncLocalVar_Set },
    },
};

#define ValueCopy(x) (eprintf("VC:%d/%p. %s\n", __LINE__, x.proper.p, strrchr(__FILE__, '/')), (ValueCopy)(x))
#define ValueDestroy(x) (eprintf("VD:%d/%p. %s\n", __LINE__, x.proper.p, strrchr(__FILE__, '/')), (ValueDestroy)(x))

// Added 2026-04-07.

// Ephemeral entities that're not scoped anywhere can be destroyed.
// Others (mostly lvalues) are not touched by this.
#define DiscardRValue() do {                            \
        eprintf("XLV (%p): Discard RValue: %d/%s\n",    \
                varreg.key, __LINE__, __FILE__);        \
        DiscardRValue_Impl(&varreg, evalmode);          \
    } while( false )

void DiscardRValue_Impl(
    struct lvalue_nativeobj *var,
    cxing_func_eval_mode_t evalmode)
{
    if( !var->key && evalmode == cxing_func_eval_mode_execute )
    {
        eprintf("Bang!\n");
        ValueDestroy(var->value);
        var->value.proper.p = NULL;
        var->value.type = (void *)&type_nativeobj_morgoth;
    }
}

// This guarantees that it'd be safe to call `ValueDestroy` on them.
#define HoldAndClearLValue() do {                               \
        eprintf("XLV (%p): Hold 'n Clear LValue: %d/%s\n",      \
                varreg.key, __LINE__, __FILE__);                \
        HoldAndClearLValue_Impl(&varreg, evalmode);             \
    } while( false )

void HoldAndClearLValue_Impl(
    struct lvalue_nativeobj *var,
    cxing_func_eval_mode_t evalmode)
{
    if( var->key && evalmode == cxing_func_eval_mode_execute )
    {
        eprintf("Bang!\n");
        var->value = ValueCopy(var->value);
        if( var->key != (void *)1 ) s2obj_leave(var->key);
        var->key = NULL;
        var->scope.proper.p = NULL;
        var->scope.type = (void *)&type_nativeobj_morgoth;
    }
}

// Strips the 'l' from values. Idempotent like `HoldAndClearLValue`.
#define DemoteLValue() do {                             \
        eprintf("XLV (%p): Demote LValue: %d/%s\n",     \
                varreg.key, __LINE__, __FILE__);        \
        DemoteLValue_Impl(&varreg, evalmode);           \
    } while( false )

void DemoteLValue_Impl(
    struct lvalue_nativeobj *var,
    cxing_func_eval_mode_t evalmode)
{
    if( var->key && evalmode == cxing_func_eval_mode_execute )
    {
        eprintf("Bang!\n");
        if( var->key != (void *)1 ) s2obj_leave(var->key);
        var->key = NULL;
        var->scope.proper.p = NULL;
        var->scope.type = (void *)&type_nativeobj_morgoth;
    }
}

// 2025-12-30: subroutines first, methods next.

struct value_nativeobj CxingFuncEval(
    cxing_module_t *restrict module,
    lalr_prod_t *restrict textsegment,
    lalr_prod_t *restrict params,
    int argn, struct value_nativeobj args[],
    enum types_enum function_kind,
    cxing_func_eval_mode_t evalmode)
{
    cxing_program_counter_t pc = {
        .capacity = 0,
        .spind = 0,
        .instructions = NULL,
        .function_kind = function_kind,
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
    int ReachesHere = 0;

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

start_eval_1term:;
    static long cnt = 0; if( ++cnt >= 7654321 ) exit(12); // time limiter, TODO: remove after debugging.
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
            instruction->operand_index].production;

#if CXING_INTERP_TRACING_LEVEL >= CXING_INTERP_TRACING_LEVEL_SRC

        lex_token_t *tok = instruction->node_body->terms[
            instruction->operand_index].terminal;

        // Prints out the currently evaluating production subterm tree.
        if( s2_is_prod(term) )
        {
            // fprint_prod(stderr, instruction->node_body->terms[instruction->operand_index].production, 0, ns_rules_cxing);

            fprintf(stderr, "\n%*s %d:%s<%d:%s>\n\n",
                    (int)pc.spind * 2 + 3, "",
                    term->rule,
                    strvec_i2str(ns_rules_cxing, term->production),
                    term->semantic_rule,
                    strvec_i2str(ns_rules_cxing, term->semantic_production));

            break;
        }
        else
        {
            fprintf(stderr, "\n%*s lineno=%d, column=%d; ",
                    (int)pc.spind * 2 + 3, "", tok->lineno, tok->column);
            fprint_token(stderr, tok, 3);
            fprintf(stderr, "\n");
        }
#else /* not a verbose debugging interpreter, but a production one. */
        if( s2_is_prod(term) )
            break;
#endif /* CXING_INTERP_TRACING_LEVEL >= CXING_INTERP_TRACING_SRC */

        instruction->operand_index ++;
    }

    assert( instruction->operand_index <=
            instruction->node_body->terms_count );

    // 2026-02-05:
    // Put "//>RULEIMPL<//" at every line that recognizes an AST rule,
    // so that a simple grep will be able to find un-implemented rules.
#define CXING_IMPLEMENT_FUNC_EXEC 1
#include "cxing-interp-func-exec.expressions.bits.h"
#include "cxing-interp-func-exec.phrases.bits.h"
#include "cxing-interp-func-exec.declarations.bits.h"
#include "cxing-interp-func-exec.objdef.bits.h"
#include "cxing-interp-func-exec.statements.bits.h"
#include "cxing-interp-func-exec.for-loops.bits.h"

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
            eprintf("FuncEvalRet %d\n", __LINE__);
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }
        else
        {
            eprintf("FuncEvalRet %d\n", __LINE__);
            return valreg;
        }
    }

    Reached();
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
