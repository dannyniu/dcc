/* DannyNiu/NJF, 2026-02-02. Public Domain. */

#define PassResultBack(...) (                                   \
        eprintf("[objdef] PassResultBack %d.\n", __LINE__),     \
        instruction->ax = __VA_ARGS__)

#ifdef CXING_IMPLEMENT_FUNC_EXEC

#if CXING_INTERP_TRACING_LEVEL > 0
ReachesHere = 1;
#endif // CXING_INTERP_TRACING_LEVEL > 0 //

//
// Type Definition and Object Initialization Syntax

if( theRule == objdefstartcomma_genrule ) //>RULEIMPL<//
{
    Reached();
    if( instruction->operand_index == 0 )
    {
        Reached();
        PcStack_PushOrAbandon();
    }
    else
    {
        instruction[-1].opts = instruction->opts;
        instruction[-1].bx = instruction->bx;
    }
}

if( theRule == objdefstartnocomma_base || //>RULEIMPL<//
    theRule == objdefstartnocomma_genrule ) //>RULEIMPL<//
{
    unsigned opind = theRule == objdefstartnocomma_genrule ? 0 : 1;
    Reached();
    if( instruction->operand_index == 0 )
    {
        Reached();
        PcStack_PushOrAbandon();
    }

    else if( instruction->operand_index == opind+1 )
    {
        Reached();
        if( theRule == objdefstartnocomma_base )
        {
            // for the duration of obj-def,
            // `instruction->bx` is the type object.
            instruction->bx = valreg; // To PassResultBack later.

            if( varreg.key )
            {
                instruction->opts = ast_node_insulate_bx_after_call;
            }
            else
            {
                instruction->opts = ast_node_release_bx_after_call;
            }
        }

        PcStack_PushOrAbandon();
    }

    else if( instruction->operand_index == opind+3 )
    {
        Reached();
        DemoteLValue();
        PassResultBack(valreg);
        PcStack_PushOrAbandon();
    }

    else if( instruction->operand_index ==
             instruction->node_body->terms_count )
    {
        struct value_nativeobj InitSetMethod;
        DemoteLValue();

        if( evalmode == cxing_func_eval_mode_execute )
        {
            InitSetMethod =
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
                                  "is not a method as required by "
                                  "object definition notation.\n",
                                  instruction->node_body->terms[0].terminal->lineno,
                                  instruction->node_body->terms[0].terminal->column);
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
            ValueDestroy(instruction->ax);
            ValueDestroy(valreg);
        }

        instruction[-1].opts = instruction->opts;
        instruction[-1].bx = instruction->bx;

        // PassResultBack.
        instruction->ax = (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    else assert( 0 );
}

if( theRule == objdef_some1 || //>RULEIMPL<//
    theRule == objdef_some2 || //>RULEIMPL<//
    theRule == objdef_empty ) //>RULEIMPL<//
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
        struct value_nativeobj InitSetMethod;

        if( evalmode == cxing_func_eval_mode_execute )
        {
            InitSetMethod =
                // A possibility is considered where
                // `__initset__` member is replaced
                // during the evaluation of the notation.
                GetValProperty(
                    instruction->bx,
                    CxingPropName_InitSet).value;

            if( InitSetMethod.type->typeid != valtyp_method )
            {
                CxingDiagnose("The postfix expression used in "
                              "completing the object definition notation "
                              "was not a method.");
            }
            else
            {
                struct value_nativeobj args[3] = {
                    instruction->bx,
                    CxingPropName_Proto,
                    instruction->bx };
                ((cxing_call_proto)InitSetMethod.proper.p)(3, args);
                Reached();
            }

            if( instruction->opts == ast_node_insulate_bx_after_call )
            {
                instruction->bx = ValueCopy(instruction->bx);
            }
        }
        PassResultBack(instruction->bx);
    }

    else assert( 0 );
}

// auto-indexed arrays. // 2026-03-14: TODO.

if( theRule == array_piece_base ) //>RULEIMPL<//
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

        if( varreg.key )
        {
            instruction->opts = ast_node_insulate_bx_after_call;
        }
        else
        {
            instruction->opts = ast_node_release_bx_after_call;
        }

        // zero-based indexing.
        PassResultBack((struct value_nativeobj){
                .proper.l = 0,
                .type = (const void *)&type_nativeobj_long });

        PcStack_PushOrAbandon();
    }

    else if( instruction->operand_index ==
             instruction->node_body->terms_count )
    {
        struct value_nativeobj InitSetMethod;
        DemoteLValue();

        if( evalmode == cxing_func_eval_mode_execute )
        {
            InitSetMethod =
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
                              "is not a method as required by "
                              "object definition notation.\n",
                              instruction->node_body->terms[0].terminal->lineno,
                              instruction->node_body->terms[0].terminal->column);
            }
            else
            {
                struct value_nativeobj key = Key2Str(instruction->ax);
                struct value_nativeobj args[3] = {
                    instruction->bx, key, valreg };
                ((cxing_call_proto)InitSetMethod.proper.p)(3, args);
                ValueDestroy(key);
            }
            ValueDestroy(valreg);
        }

        instruction[-1].opts = instruction->opts;
        instruction[-1].bx = instruction->bx;
    }

    else assert( 0 );
}

if( theRule == array_piece_genrule ) //>RULEIMPL<//
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
        PassResultBack(valreg);
        instruction->ax.proper.l ++;

        PcStack_PushOrAbandon();
    }

    else if( instruction->operand_index ==
             instruction->node_body->terms_count )
    {
        struct value_nativeobj InitSetMethod;
        DemoteLValue();

        if( evalmode == cxing_func_eval_mode_execute )
        {
            InitSetMethod =
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
                struct value_nativeobj key = Key2Str(instruction->ax);
                struct value_nativeobj args[3] = {
                    instruction->bx, key, valreg };
                ((cxing_call_proto)InitSetMethod.proper.p)(3, args);
                ValueDestroy(key);
            }
            ValueDestroy(valreg);
        }
        instruction[-1].opts = instruction->opts;
        instruction[-1].bx = instruction->bx;
    }

    else assert( 0 );
}

if( theRule == array_streamline ) //>RULEIMPL<//
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
        PassResultBack(valreg);
        instruction->ax.proper.l ++;

        PcStack_PushOrAbandon();
    }

    else if( instruction->operand_index ==
             instruction->node_body->terms_count )
    {
        struct value_nativeobj InitSetMethod;
        DemoteLValue();

        if( evalmode == cxing_func_eval_mode_execute )
        {
            InitSetMethod =
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
                struct value_nativeobj key = Key2Str(instruction->ax);
                struct value_nativeobj args[3] = {
                    instruction->bx, key, valreg };
                ((cxing_call_proto)InitSetMethod.proper.p)(3, args);
                ValueDestroy(key);

                // a 2nd call for end of list
                args[1] = CxingPropName_Proto;
                args[2] = instruction->bx;
                ((cxing_call_proto)InitSetMethod.proper.p)(3, args);
            }
            ValueDestroy(valreg);

            if( instruction->opts == ast_node_insulate_bx_after_call )
            {
                instruction->bx = ValueCopy(instruction->bx);
            }
        }
        PassResultBack(instruction->bx);
    }

    else assert( 0 );
}

if( theRule == array_complete ) //>RULEIMPL<//
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
        struct value_nativeobj InitSetMethod;

        if( evalmode == cxing_func_eval_mode_execute )
        {
            InitSetMethod =
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
                ((cxing_call_proto)InitSetMethod.proper.p)(3, args);
            }
            //ValueDestroy(valreg);

            if( instruction->opts == ast_node_insulate_bx_after_call )
            {
                instruction->bx = ValueCopy(instruction->bx);
            }
        }
        PassResultBack(instruction->bx);
    }

    else assert( 0 );
}

#endif /* CXING_IMPLEMENT_FUNC_EXEC */

#undef PassResultBack
