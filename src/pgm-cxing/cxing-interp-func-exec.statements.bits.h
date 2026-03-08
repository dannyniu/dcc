/* DannyNiu/NJF, 2026-02-02. Public Domain. */

#ifdef CXING_IMPLEMENT_FUNC_EXEC

//
// statements.

assert( evalmode == cxing_func_eval_mode_dryrun ||
        evalmode == cxing_func_eval_mode_execute );

if( theRule == stmt_decl || //>RULEIMPL<//
    theRule == stmt_brace || //>RULEIMPL<//
    theRule == stmtlist_base ) //>RULEIMPL<//
{
    Reached();
    if( instruction->operand_index <
        instruction->node_body->terms_count )
    {
        Reached();
        PcStack_PushOrAbandon();
    }
}

if( theRule == stmtlist_genrule ) //>RULEIMPL<//
{
    lalr_prod_t *subterm = NULL;

    Reached();

    if( instruction->node_body->terms_count > 0 )
        subterm = instruction->node_body->terms[0].production;

    if( instruction->unfolded == 0 )
    {
        if( !s2_is_prod(subterm) ||
            rules(subterm->semantic_rule) != stmtlist_genrule )
        {
            // Reached bottom.
            instruction->unfolded ++;
            instruction->operand_index = 0;
            Reached();
            PcStack_PushOrAbandon();
        }
        else
        {
            // Not yet traversing backwards.
            instruction->folded_depth ++;
            instruction->node_body = subterm;
            goto start_eval_1term;
        }
    }
    else
    {
        // Traversing backwards now.
        instruction->unfolded ++;
        instruction->operand_index = 1;
        Reached();

        subterm = instruction->node_body;
        instruction->node_body = instruction->node_body->parent;

        if( instruction->folded_depth >= instruction->unfolded
            - 2 ) // 1 when reaching bottom, 1 while traversing back.
        {
            if( !PcStackPush(
                    &pc, subterm->terms[
                        instruction->operand_index].production) )
            {
                goto func_exec_abort;
            }
            else goto start_eval_1term;
        }
        else
        {
            instruction->operand_index = 2;
            goto finish_eval_1term;
        }
    }
}

if( theRule == stmt_labelled ) //>RULEIMPL<//
{
    Reached();
    if( instruction->operand_index < 2 )
    {
        instruction->operand_index = 2;
        PcStack_PushOrAbandon();
    }
    else goto finish_eval_1term;
}

if( theRule == predclause_base ) //>RULEIMPL<//
{
    Reached();
    if( instruction->operand_index == 2 )
    {
        Reached();
        PcStack_PushOrAbandon();
    }
    else if( instruction->operand_index == 4 )
    {
        if( ValueNativeObj2Logic(valreg) ||
            evalmode == cxing_func_eval_mode_dryrun )
        {
            PcStack_PushOrAbandon();
            instruction->flags = ast_node_action_noelse;
        }
    }
    else if( instruction->operand_index ==
             instruction->node_body->terms_count )
    {
        goto finish_eval_1term;
    }
    else assert( 0 );
}

if( theRule == predclause_genrule ) //>RULEIMPL<//
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
        if( instruction->flags == ast_node_action_noelse &&
            evalmode == cxing_func_eval_mode_execute )
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
        if( ValueNativeObj2Logic(valreg) ||
            evalmode == cxing_func_eval_mode_dryrun )
        {
            PcStack_PushOrAbandon();
            instruction->flags = ast_node_action_noelse;
        }
    }
    else if( instruction->operand_index ==
             instruction->node_body->terms_count )
    {
        goto finish_eval_1term;
    }
    else assert( 0 );
}

if( theRule == condstmt_else ) //>RULEIMPL<//
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
        if( instruction->flags == ast_node_action_noelse &&
            evalmode == cxing_func_eval_mode_execute )
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
    else if( instruction->operand_index ==
             instruction->node_body->terms_count )
    {
        goto finish_eval_1term;
    }
    else assert( 0 );
}

if( theRule == while_rule ) //>RULEIMPL<//
{
    Reached();
    if( instruction->operand_index == 2 )
    {
        Reached();
        PcStack_PushOrAbandon();
    }
    else if( instruction->operand_index == 4 )
    {
        if( ValueNativeObj2Logic(valreg) ||
            evalmode == cxing_func_eval_mode_dryrun )
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
        if( instruction->flags == ast_node_action_break ||
            evalmode == cxing_func_eval_mode_dryrun )
        {
            goto finish_eval_1term;
        }
        else
        {
            instruction->operand_index = 0;
            goto start_eval_1term;
        }
    }
    else
    {
        (void)0;
        // do not assert,
        // because that operand_index incrementation
        // depends on this to break out of the loop.
    }
}

if( theRule == dowhile_rule ) //>RULEIMPL<//
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

        if( instruction->flags == ast_node_action_break )
            goto finish_eval_1term;

        PcStack_PushOrAbandon();
    }
    else if( instruction->operand_index ==
             instruction->node_body->terms_count )
    {
        Reached();

        if( instruction->flags == ast_node_action_break ||
            evalmode == cxing_func_eval_mode_dryrun )
            goto finish_eval_1term;

        if( !ValueNativeObj2Logic(valreg) )
            goto finish_eval_1term;

        instruction->operand_index = 0;
        goto start_eval_1term;
    }
    else assert( 0 );
}

#endif /* CXING_IMPLEMENT_FUNC_EXEC */
