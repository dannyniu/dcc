/* DannyNiu/NJF, 2026-02-02. Public Domain. */

#ifdef CXING_IMPLEMENT_FUNC_EXEC

//
// phrases.

assert( evalmode == cxing_func_eval_mode_dryrun ||
        evalmode == cxing_func_eval_mode_execute );

if( theRule == ctrl_flow_ion_op_or || //>RULEIMPL<//
    theRule == ctrl_flow_ion_op_nc || //>RULEIMPL<//
    theRule == ctrl_flow_ion_labelledop_or || //>RULEIMPL<//
    theRule == ctrl_flow_ion_labelledop_nc || //>RULEIMPL<//
    theRule == ctrl_flow_molecule_op || //>RULEIMPL<//
    theRule == ctrl_flow_molecule_labelledop ) //>RULEIMPL<//
{
    Reached();
    lalr_rule_t rs = theRule;

    int32_t optype =
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

    if( evalmode == cxing_func_eval_mode_dryrun )
    {
        size_t bpind = pc.spind;

        while( true )
        {
            if( !labelledop )
            {
                // 2026-02-07:
                // The dry-run for control-flow phrases only needs to
                // find undefined statement labels.
                break;
            }

            if( bpind <= 0 )
            {
                CxingSemanticErr(
                    module,
                    "Undefined statement label: `%s` "
                    "at line %d column %d.\n",
                    s2data_weakmap(label->str),
                    label->lineno, label->column);
                break;
            }

            bpind --;

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
                if( rules(pc.instructions[bpind].node_body
                          ->semantic_rule) == stmt_labelled )
                {
                    if( s2data_cmp(pc.instructions[bpind].node_body
                                   ->terms[0].production
                                   ->terms[0].terminal->str,
                                   label->str) == 0 )
                        break;
                }
            }
            rs = rules(pc.instructions[bpind].node_body->semantic_rule);
        }
    }
    else if( evalmode == cxing_func_eval_mode_execute )
    {
        while( true )
        {
            Reached();
            if( labelledop )
            {
                // Assume it won't survive semantic dry-run.
                assert( pc.spind >= 1 );

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

            // Collects garbage - i.e. values that went out of scope.
            ValueDestroy(
                // Traverses the braces outwards.
                PcStackPop(&pc)
                );

            // updates the '`instruction` pointer'.
            instruction = &pc.instructions[pc.spind];

            // and the local register.
            rs = theRule;
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
    else assert( 0 );
}

if( theRule == ctrl_flow_ion_returnnull_or || //>RULEIMPL<//
    theRule == ctrl_flow_ion_returnnull_nc || //>RULEIMPL<//
    theRule == ctrl_flow_molecule_returnnull ) //>RULEIMPL<//
{
    Reached();
    pc.instructions[0].ax = (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
    pc.instructions[0].flags = ast_node_action_return;
    goto finish_eval_1term;
}

if( theRule == ctrl_flow_ion_returnexpr_or || //>RULEIMPL<//
    theRule == ctrl_flow_ion_returnexpr_nc || //>RULEIMPL<//
    theRule == ctrl_flow_molecule_returnexpr ) //>RULEIMPL<//
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
        pc.instructions[0].ax = ValueCopy(valreg);
        pc.instructions[0].flags = ast_node_action_return;
        goto finish_eval_1term;
    }
}

if( theRule == and_phrase_ion_and || //>RULEIMPL<//
    theRule == and_phrase_ion_then || //>RULEIMPL<//
    theRule == or_phrase_ion_or || //>RULEIMPL<//
    theRule == or_phrase_ion_nc ) //>RULEIMPL<//
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

if( theRule == phrase_stmt_base ) //>RULEIMPL<//
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

if( theRule == or_phrase_atom_atomize || //>RULEIMPL<//
    theRule == and_phrase_atom_atomize || //>RULEIMPL<//
    theRule == phrase_stmt_conj_ctrl_flow || //>RULEIMPL<//
    theRule == phrase_stmt_disj_ctrl_flow ) //>RULEIMPL<//
{
    Reached();
    if( instruction->operand_index == 0 )
    {
        Reached();
        PcStack_PushOrAbandon();
    }
    else if( instruction->operand_index == 1 )
    {
        if( (ast_node_action_stop == instruction->flags ||
             ast_node_action_return == pc.instructions[0].flags) &&
            evalmode == cxing_func_eval_mode_execute )
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

if( theRule == or_phrase_ion_ctrl_flow ) //>RULEIMPL<//
{
    Reached();
    if( instruction->operand_index == 0 )
    {
        Reached();
        PcStack_PushOrAbandon();
    }
    else if( instruction->operand_index == 1 )
    {
        if( (ast_node_action_stop == instruction->flags ||
             ast_node_action_return == pc.instructions[0].flags) &&
            evalmode == cxing_func_eval_mode_execute )
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

#endif /* CXING_IMPLEMENT_FUNC_EXEC */
