/* DannyNiu/NJF, 2026-02-02. Public Domain. */

#ifdef CXING_IMPLEMENT_FUNC_EXEC

// body finish, set operand index to
// the incrementation or the condition clause.
#define forloop_bodyfin(oi_inc)                         \
    Reached();                                          \
    if( instruction->flags == ast_node_action_break )   \
    {                                                   \
        goto finish_eval_1term;                         \
    }                                                   \
    else                                                \
    {                                                   \
        instruction->flags = ast_node_action_continue;  \
        instruction->operand_index = oi_inc;            \
        goto start_eval_1term;                          \
    }

// same as above, but without setting continue action.
// ---
// The continue action needed to be set because otherwise
// the incrementation clause won't be evaluated.
// So for forms without the incrementation clause,
// use `forloop_bodyfin_simple` whenever possible.
#define forloop_bodyfin_simple(oi_inc)                  \
    Reached();                                          \
    if( instruction->flags == ast_node_action_break ||  \
        evalmode == cxing_func_eval_mode_dryrun )       \
    {                                                   \
        goto finish_eval_1term;                         \
    }                                                   \
    else                                                \
    {                                                   \
        instruction->operand_index = oi_inc;            \
        goto start_eval_1term;                          \
    }

#define forloop_increment(oi_inc, oi_cond)                      \
    Reached();                                                  \
    instruction->operand_index = oi_cond;                       \
    if( instruction->flags == ast_node_action_continue )        \
    {                                                           \
        if( !PcStackPush(                                       \
                &pc,                                            \
                instruction->node_body                          \
                ->terms[oi_inc].production) )                   \
        {                                                       \
            goto func_exec_abort;                               \
        }                                                       \
        Reached();                                              \
    }                                                           \
    if( evalmode == cxing_func_eval_mode_dryrun )               \
        instruction->opts = ast_node_dryrun_done;               \
    goto start_eval_1term;

#define forloop_condition(oi_cond, oi_body)             \
    Reached();                                          \
    instruction->operand_index = oi_body;               \
    if( instruction->opts == ast_node_dryrun_done )     \
        goto finish_eval_1term;                         \
    if( !PcStackPush(                                   \
            &pc,                                        \
            instruction->node_body                      \
            ->terms[oi_cond].production) )              \
    {                                                   \
        goto func_exec_abort;                           \
    }                                                   \
    else                                                \
    {                                                   \
        if( evalmode == cxing_func_eval_mode_dryrun )   \
            instruction->opts = ast_node_dryrun_done;   \
        goto start_eval_1term;                          \
    }

if( evalmode == cxing_func_eval_mode_execute )
{
    if( theRule == for_forever ) //>RULEIMPL<//
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
            forloop_bodyfin_simple(0);
        }
        else assert( 0 );
    }

    if( theRule == for_initonly || //>RULEIMPL<//
        theRule == for_vardecl ) //>RULEIMPL<//
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
            forloop_bodyfin_simple(5);
        }
        else assert( 0 );
    }

    if( theRule == for_iterated ) //>RULEIMPL<//
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
            forloop_bodyfin(3);
        }
        else assert( 0 );
    }

    if( theRule == for_conditioned ) //>RULEIMPL<//
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
            forloop_bodyfin_simple(0);
        }
        else
        {
            (void)0;
            // do not assert,
            // because that operand_index incrementation
            // depends on this to break out of the loop.
        }
    }

    if( theRule == for_controlled ) //>RULEIMPL<//
    {
        Reached();
        if( instruction->operand_index == 3 ) // the condition clause.
        {
            forloop_condition(3, 6);
        }
        else if( instruction->operand_index == 5 ) // the increment clause.
        {
            forloop_increment(5, 2);
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
            forloop_bodyfin(4);
        }
        else
        {
            (void)0;
            // do not assert,
            // because that operand_index incrementation
            // depends on this to break out of the loop.
        }
    }

    if( theRule == for_classic || //>RULEIMPL<//
        theRule == for_vardecl_controlled ) //>RULEIMPL<//
    {
        Reached();
        if( instruction->operand_index == 2 ) // the init clause.
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 4 ) // the condition clause.
        {
            forloop_condition(4, 7);
        }
        else if( instruction->operand_index == 6 ) // the increment clause.
        {
            forloop_increment(6, 3);
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
            forloop_bodyfin(5);
        }
        else
        {
            (void)0;
            // do not assert,
            // because that operand_index incrementation
            // depends on this to break out of the loop.
        }
    }

    if( theRule == for_nocond || //>RULEIMPL<//
        theRule == for_vardecl_nocond ) //>RULEIMPL<//
    {
        Reached();
        if( instruction->operand_index == 2 ) // the init clause.
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 5 ) // the increment clause.
        {
            forloop_increment(5, 6);
        }
        else if( instruction->operand_index == 7 ) // the body clause.
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index ==
                 instruction->node_body->terms_count )
        {
            forloop_bodyfin(4);
        }
        else assert( 0 );
    }

    if( theRule == for_noiter || //>RULEIMPL<//
        theRule == for_vardecl_noiter ) //>RULEIMPL<//
    {
        Reached();
        if( instruction->operand_index == 2 ) // the init clause.
        {
            Reached();
            PcStack_PushOrAbandon();
        }
        else if( instruction->operand_index == 4 ) // the condition clause.
        {
            forloop_condition(4, 6);
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
            forloop_bodyfin_simple(3);
        }
        else
        {
            (void)0;
            // do not assert,
            // because that operand_index incrementation
            // depends on this to break out of the loop.
        }
    }
}
else if( evalmode == cxing_func_eval_mode_dryrun )
{
    if( theRule == for_forever ||
        theRule == for_iterated ||
        theRule == for_conditioned ||
        theRule == for_controlled ||
        theRule == for_initonly ||
        theRule == for_nocond ||
        theRule == for_noiter ||
        theRule == for_classic ||
        theRule == for_vardecl ||
        theRule == for_vardecl_nocond ||
        theRule == for_vardecl_noiter ||
        theRule == for_vardecl_controlled )
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
            goto finish_eval_1term;
        }
        else assert( 0 );
    }
}
else assert( 0 );

#endif /* CXING_IMPLEMENT_FUNC_EXEC */
