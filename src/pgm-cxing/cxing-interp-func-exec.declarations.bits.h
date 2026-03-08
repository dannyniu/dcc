/* DannyNiu/NJF, 2026-02-02. Public Domain. */

#ifdef CXING_IMPLEMENT_FUNC_EXEC

//
// declarations.

if( theRule == decl_singledecl || //>RULEIMPL<//
    theRule == decl_singledeclinit || //>RULEIMPL<//
    theRule == decl_declarelist1 || //>RULEIMPL<//
    theRule == decl_declarelist2 ) //>RULEIMPL<//
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
        pc.instructions[sfind].vardecls = rfdict_create();

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

    if( rfdict_set(
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

#endif /* CXING_IMPLEMENT_FUNC_EXEC */
