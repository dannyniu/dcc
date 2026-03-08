/* DannyNiu/NJF, 2026-02-02. Public Domain. */

#ifdef CXING_IMPLEMENT_FUNC_EXEC

assert( evalmode == cxing_func_eval_mode_dryrun ||
        evalmode == cxing_func_eval_mode_execute );

//
// lexicons for literals.

// scalar literals.

if( evalmode == cxing_func_eval_mode_execute )
{
    if( theRule == const_true ||
        theRule == const_false ||
        theRule == const_null ||
        theRule == const_declit ||
        theRule == const_octlit ||
        theRule == const_hexlit ||
        theRule == const_decfplit ||
        theRule == const_hexfplit ||
        theRule == const_charlit )
    {
        instruction->ax = CXConstDefParse(instruction->node_body);
    }
    else
    {
        assert( theRule != const_true &&
                theRule != const_false &&
                theRule != const_null &&
                theRule != const_declit &&
                theRule != const_octlit &&
                theRule != const_hexlit &&
                theRule != const_decfplit &&
                theRule != const_hexfplit &&
                theRule != const_charlit );
    }

    if( theRule == const_strlit ) //>RULEIMPL<//
    {
        // Unquoting implemented in the lexer
        // along with adjacency concatenation
        // (retro note 2026-02-25).
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

    if( theRule == const_r64lit ) //>RULEIMPL<//
    {
        Reached();
        instruction->ax = (struct value_nativeobj){
            .proper.u = Radix64Literal(s2data_weakmap(
                                           instruction->node_body
                                           ->terms[0].terminal->str)),
            .type = (const void *)&type_nativeobj_ulong };
    }
}

//
// other primary expressions.

if( theRule == primary_paren ) //>RULEIMPL<//
{
    Reached();
    if( instruction->operand_index <
        instruction->node_body->terms_count )
    {
        Reached();
        PcStack_PushOrAbandon();
    }
}

//
// identifiers resolution.

if( theRule == ident_ident ) //>RULEIMPL<//
{
    Reached();
    if( evalmode == cxing_func_eval_mode_dryrun )
    {
        if( !CxingFuncLocalVar_IsSetImpl0(
                &localvars0, instruction->node_body->terms[0].terminal->str) )
        {
            CxingSemanticErr(
                module, "Undeclared identifier: `%s` at line %d column %d.\n",
                s2data_weakmap(instruction->node_body->terms[0].terminal->str),
                instruction->node_body->terms[0].terminal->lineno,
                instruction->node_body->terms[0].terminal->column);
        }
        varreg.key = instruction->node_body->terms[0].terminal->str;
    }
    else
    {
        varreg = GetValProperty(
            localvars, (struct value_nativeobj){
                .proper.p = instruction->node_body->terms[0].terminal->str,
                .type = (const void *)&type_nativeobj_s2impl_str });
        instruction->ax = valreg;
        s2obj_keep(varreg.key);
    }
    instruction->opts = ast_node_lvalue_register;
    instruction->operand_index = instruction->node_body->terms_count;
}

if( theRule == postfix_member ) //>RULEIMPL<//
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
        if( evalmode == cxing_func_eval_mode_dryrun )
        {
            varreg.key =
                instruction->node_body
                ->terms[2].production
                ->terms[0].terminal->str;
        }
        else
        {
            if( varreg.key ) s2obj_leave(varreg.key);
            varreg = GetValProperty(
                valreg, (struct value_nativeobj){
                    .proper.p = instruction->node_body
                    ->terms[2].production
                    ->terms[0].terminal->str,
                    .type = (const void *)&type_nativeobj_s2impl_str });
            instruction->ax = valreg;
            s2obj_keep(varreg.key);
        }
        instruction->opts = ast_node_lvalue_register;
        instruction->operand_index = instruction->node_body->terms_count;
    }
}

if( theRule == postfix_indirect ) //>RULEIMPL<//
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
    else // TODO 2026-02-28. Resume Here.
    {
        Reached();
        if( evalmode == cxing_func_eval_mode_dryrun )
        {
            varreg.key = (void *)1;
        }
        else
        {
            if( varreg.key ) s2obj_leave(varreg.key);
            varreg = GetValProperty(instruction->ax, valreg);
            instruction->ax = valreg;
        }
        instruction->opts = ast_node_lvalue_register;
        instruction->operand_index = instruction->node_body->terms_count;
    }
}

//
// function calls.

if( theRule == funcinvokenocomma_base ) //>RULEIMPL<//
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
        /* if( evalmode == cxing_func_eval_mode_execute && varreg.key &&
           instruction->opts != ast_node_lvalue_register )
           ; // s2obj_leave(varreg.key); // 2026-03-08 couldn't justify this, */
        PcStack_PushOrAbandon();
    }
    else if( instruction->operand_index ==
             instruction->node_body->terms_count )
    {
        Reached();
        if( evalmode == cxing_func_eval_mode_execute )
        {
            instruction->fargs = calloc(2, sizeof(struct value_nativeobj));
            if( !instruction->fargs )
            {
                CxingFatal("Call to calloc failed when trying to allocate "
                           "space for function arguments.\n");
                goto func_exec_abort;
            }

            instruction->fargn = 2;
            // reserve 1 slot for `this` argument to methods,
            // pointer arithmetic to ignore it for subroutines.
            //- instruction->fargs[0] = ...;

            valreg = ValueCopy(valreg);
            instruction->fargs[1] = valreg;
        }
    }
    else assert( 0 );
}

if( theRule == funcinvokenocomma_genrule ) //>RULEIMPL<//
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
        if( evalmode == cxing_func_eval_mode_execute )
        {
            void *tmp = realloc(
                instruction->fargs,
                (instruction->fargn + 1) *
                sizeof(struct value_nativeobj));

            if( !tmp )
            {
                CxingFatal("Call to realloc failed when trying to allocate "
                           "space for function arguments.\n");
                goto func_exec_abort;
            }

            Reached();
            instruction->fargs = tmp;

            valreg = ValueCopy(valreg);
            instruction->fargs[instruction->fargn ++] = valreg;
        }
    }
    else assert( 0 );
}

if( theRule == funccall_somearg ) //>RULEIMPL<//
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
        if( evalmode == cxing_func_eval_mode_execute )
        {
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
                           "a non-function value 01.\n");
            }
        }
    }
    else assert( 0 );
}

if( theRule == funccall_noarg ) //>RULEIMPL<//
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
        if( evalmode == cxing_func_eval_mode_execute )
        {
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
                           "a non-function value 02.\n");
            }
        }
    }
    else assert( 0 );
}

//
// expressions.

if( theRule == unary_dec || //>RULEIMPL<//
    theRule == postfix_dec) //>RULEIMPL<//
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
        if( evalmode == cxing_func_eval_mode_execute )
            instruction->ax = DecrementExpr(
                varreg, theRule == postfix_dec);
    }
    else assert( 0 );
}

if( theRule == unary_inc || //>RULEIMPL<//
    theRule == postfix_inc ) //>RULEIMPL<//
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
        if( evalmode == cxing_func_eval_mode_execute )
            instruction->ax = IncrementExpr(
                varreg, theRule == postfix_inc);
    }
    else assert( 0 );
}

if( theRule == unary_positive || //>RULEIMPL<//
    theRule == unary_negative || //>RULEIMPL<//
    theRule == unary_bitcompl || //>RULEIMPL<//
    theRule == unary_logicnot ) //>RULEIMPL<//
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
        if( evalmode == cxing_func_eval_mode_execute )
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
    }
    else assert( 0 );
}

if( theRule == bitor_bitor || //>RULEIMPL<//
    theRule == bitxor_bitxor || //>RULEIMPL<//
    theRule == bitand_bitand || //>RULEIMPL<//

    theRule == eqops_eq || //>RULEIMPL<//
    theRule == eqops_ne || //>RULEIMPL<//
    theRule == eqops_ideq || //>RULEIMPL<//
    theRule == eqops_idne || //>RULEIMPL<//

    theRule == relops_lt || //>RULEIMPL<//
    theRule == relops_gt || //>RULEIMPL<//
    theRule == relops_le || //>RULEIMPL<//
    theRule == relops_ge || //>RULEIMPL<//

    theRule == shiftexpr_lshift || //>RULEIMPL<//
    theRule == shiftexpr_arshift || //>RULEIMPL<//
    theRule == shiftexpr_rshift || //>RULEIMPL<//

    theRule == addexpr_add || //>RULEIMPL<//
    theRule == addexpr_subtract || //>RULEIMPL<//
    theRule == mulexpr_multiply || //>RULEIMPL<//
    theRule == mulexpr_divide || //>RULEIMPL<//
    theRule == mulexpr_remainder ) //>RULEIMPL<//
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
            // The right operand is managed by
            // the `finish_eval_1term` epilogue.
            instruction->opts = ast_node_lvalue_operand;
        }

        instruction->ax = valreg;
        PcStack_PushOrAbandon();
    }
    else if( instruction->operand_index ==
             instruction->node_body->terms_count )
    {
        if( evalmode == cxing_func_eval_mode_execute )
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
}

if( theRule == postfix_nullcoalesce || //>RULEIMPL<//
    theRule == logicor_nullcoalesce ) //>RULEIMPL<//
{
    Reached();
    if( instruction->operand_index == 0 )
    {
        PcStack_PushOrAbandon();
    }
    else if( instruction->operand_index == 2 )
    {
        if( IsNullish(valreg) ||
            evalmode == cxing_func_eval_mode_dryrun )
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

if( theRule == logicand_then ) //>RULEIMPL<//
{
    Reached();
    if( instruction->operand_index == 0 )
    {
        PcStack_PushOrAbandon();
    }
    else if( instruction->operand_index == 2 )
    {
        if( !IsNullish(valreg) ||
            evalmode == cxing_func_eval_mode_dryrun )
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

if( theRule == logicor_logicor || //>RULEIMPL<//
    theRule == logicand_logicand ) //>RULEIMPL<//
{
    Reached();
    if( instruction->operand_index == 0 )
    {
        PcStack_PushOrAbandon();
    }
    else if( instruction->operand_index == 2 )
    {
        if( (theRule == logicand_logicand) ==
            ValueNativeObj2Logic(valreg) ||
            evalmode == cxing_func_eval_mode_dryrun )
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

if( theRule == assignment_directassign || //>RULEIMPL<//
    theRule == assignment_mulassign || //>RULEIMPL<//
    theRule == assignment_divassign || //>RULEIMPL<//
    theRule == assignment_remassign || //>RULEIMPL<//
    theRule == assignment_addassign || //>RULEIMPL<//
    theRule == assignment_subassign || //>RULEIMPL<//
    theRule == assignment_lshiftassign || //>RULEIMPL<//
    theRule == assignment_arshiftassign || //>RULEIMPL<//
    theRule == assignment_rshiftassign || //>RULEIMPL<//
    theRule == assignment_andassign || //>RULEIMPL<//
    theRule == assignment_xorassign || //>RULEIMPL<//
    theRule == assignment_orassign ) //>RULEIMPL<//
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
                       "is not an lvalue!\n");
            goto finish_eval_1term;
        }
        else
        {
            eprintf("The key is: %s.\n",
                    (const char *)s2data_weakmap(varreg.key));
        }

        eprintf("asn: %llu t=%lli\n",
                instruction->ax.proper.u,
                instruction->ax.type->typeid);

        if( evalmode == cxing_func_eval_mode_execute )
        {
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
        }
        goto finish_eval_1term;
    }
}

if( theRule == exprlist_exprlist ) //>RULEIMPL<//
{
    Reached();
    if( instruction->operand_index <
        instruction->node_body->terms_count )
    {
        PcStack_PushOrAbandon();
    }
    else instruction->ax = valreg;
}

#endif /* CXING_IMPLEMENT_FUNC_EXEC */
