/* DannyNiu/NJF, 2026-02-02. Public Domain. */

#define PassResultBack(...) (                                   \
        eprintf("[objdef] PassResultBack %d.\n", __LINE__),     \
        instruction->ax = __VA_ARGS__)

#ifdef CXING_IMPLEMENT_FUNC_EXEC

ReachesHere = 1;

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

    // nothing else to do here.
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
            HoldAndClearLValue();
            // for the duration of obj-def,
            // `instruction->bx` is the type object.
            instruction->bx = valreg; // To PassResultBack later.
        }
        instruction->opts = ast_node_preserve_bx;
        PcStack_PushOrAbandon();
    }

    else if( instruction->operand_index == opind+3 )
    {
        Reached();
        HoldAndClearLValue();
        PassResultBack(valreg);
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

        HoldAndClearLValue();
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
        ValueDestroy(instruction->ax);
        ValueDestroy(valreg);

        // PassResultBack.
        instruction->ax = (struct value_nativeobj){
            .proper.p = NULL,
            .type = (void *)&type_nativeobj_morgoth };
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
            ((cxing_call_proto)InitSetMethod.proper.p)(3, args);
            Reached();
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

        // zero-based indexing.
        PassResultBack((struct value_nativeobj){
                .proper.l = 0,
                .type = (void *)&type_nativeobj_long });

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

        HoldAndClearLValue();
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
        ValueDestroy(instruction->ax);
        ValueDestroy(valreg);
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
        struct value_nativeobj InitSetMethod =
            // A possibility is considered where
            // `__initset__` member is replaced
            // during the evaluation of the notation.
            GetValProperty(
                instruction->bx,
                CxingPropName_InitSet).value;

        HoldAndClearLValue();
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
        ValueDestroy(valreg);
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
        struct value_nativeobj InitSetMethod =
            // A possibility is considered where
            // `__initset__` member is replaced
            // during the evaluation of the notation.
            GetValProperty(
                instruction->bx,
                CxingPropName_InitSet).value;

        HoldAndClearLValue();
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

#endif /* CXING_IMPLEMENT_FUNC_EXEC */

#undef PassResultBack
