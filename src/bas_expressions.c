/*** BASIC expression parser. eg: 2 + sin(100 - 10) * (3+1) ***/

#include "globals.h"

/* Doesn't need to be any bigger than the number of operator precedences but
   add a few on just in case */
#define MAX_STACK 7


static int getValue(int *pc, int end, st_value *result);
static int evalStack(
	int min_prec,
	st_value *val_stack,
	int *op_stack, int *val_stack_top, int *op_stack_top, st_token *token);
static void clearStack(st_value *val_stack);
static void evalPreOpStack(
	int *pre_op_stack, int pre_op_stack_cnt, st_value *result);



/*** Evaluate an expression. eg: 2 + 2. Returns result type. The calling 
     function MUST clear st_value itself. ***/
int evalExpression(int start, int end, st_value *result)
{
	st_value val_stack[MAX_STACK];
	int op_stack[MAX_STACK];
	int pre_op_stack[MAX_STACK];
	st_token *token;
	st_token *optok;
	int result_type;
	int pc2;
	int val_stack_top;
	int op_stack_top;
	int pre_op_stack_cnt;
	int prec;
	int pc;
	int i;

	initValue(result);
	for(i=0;i < MAX_STACK;++i) initValue(&val_stack[i]);

	/* These point to next positions to fill */
	val_stack_top = 0;
	op_stack_top = 0;
	pre_op_stack_cnt = 0;

	/* Can happen if there is no expression present */
	if (end < start)
	{
		basicError(ERR_SYNTAX,&token_list[end]);
		return NO_RESULT;
	}

	for(pc=start;pc <= end;++pc)
	{
		if (val_stack_top >= MAX_STACK)
		{
			basicError(ERR_STACK_OVERFLOW,&token_list[pc]);
			clearStack(val_stack);
			return NO_RESULT;
		}

		token = &token_list[pc];

		/* Check for NOT operator */
		if (IS_OP_TYPE(token,OP_NOT) || IS_OP_TYPE(token,OP_BIT_COMPL))
		{
			if (pc >= end)
			{
				basicError(ERR_SYNTAX,token);
				clearStack(val_stack);
				return NO_RESULT;
			}
			if (pre_op_stack_cnt == MAX_STACK)
			{
				basicError(ERR_STACK_OVERFLOW,&token_list[pc]);
				clearStack(val_stack);
				return NO_RESULT;
			}
			pre_op_stack[pre_op_stack_cnt++] = token->token_num;
			continue;
		}

		/* Check for sub expression */
		if (IS_OP_TYPE(token,OP_L_RND_BRACKET))
		{
			assert(token->u.close_bracket_loc);
			pc2 = token->u.close_bracket_loc-1;
			if (++pc > pc2)
			{
				basicError(ERR_EMPTY_EXPRESSION,token);
				clearStack(val_stack);
				return NO_RESULT;
			}
			result_type = evalExpression(pc,pc2,result);
			pc = token->u.close_bracket_loc + 1;
		}
		else result_type = getValue(&pc,end,result);

		switch(result_type)
		{
		case NO_RESULT:
			clearStack(val_stack);
			return NO_RESULT;	

		case RESULT_STR:
			if (token->negative)
			{
				basicError(ERR_INVALID_NEGATIVE,token);
				clearStack(val_stack);
				return NO_RESULT;
			}
			if (pre_op_stack_cnt) 
			{
				basicError(ERR_INVALID_ARGUMENT,token);
				clearStack(val_stack);
				return NO_RESULT;
			}
			break;

		case RESULT_NUM:
			if (token->negative) result->val = -result->val;
			evalPreOpStack(pre_op_stack,pre_op_stack_cnt,result);
			pre_op_stack_cnt = 0;
			break;

		default:
			assert(0);
		}
		setValueByValue(&val_stack[val_stack_top++],result);

		/* Have we read last value? */
		if (pc > end) break;
		
		optok = &token_list[pc];

		/* Must have an operator but can't end the expression on it */
		if (pc == end || optok->token_type != TOK_OP)
		{
			basicError(ERR_SYNTAX,optok);
			clearStack(val_stack);
			return NO_RESULT;
		}

		/* If precendence of current operator is <= than previous
		   operator(s) then evaluate stack down to equivalent level */
		prec = op_info[optok->token_num].prec;
		if (op_stack_top && prec <= op_info[op_stack[op_stack_top-1]].prec)
		{
			if (evalStack(
				prec,
				val_stack,
				op_stack,
				&val_stack_top,
				&op_stack_top,optok) == NO_RESULT)
			{
				clearStack(val_stack);
				return NO_RESULT;
			}
		}

		op_stack[op_stack_top++] = optok->token_num;
	}

	/* Evaluate reset of stack */
	if (evalStack(
		-1,
		val_stack,
		op_stack,
		&val_stack_top,&op_stack_top,&token_list[start]))
	{
		setValueByValue(result,&val_stack[0]);
		clearStack(val_stack);
		return result->strval ? RESULT_STR : RESULT_NUM;
	}
	return NO_RESULT;
}




/*** Get the value of the given token. Returns result type ***/
int getValue(int *pc, int end, st_value *result)
{
	st_token *token = &token_list[*pc];

	switch(token->token_type)
	{
	case TOK_NUM:
		setValue(result,token->val,NULL);
		++(*pc);
		return RESULT_NUM;

	case TOK_STR:
		setValue(result,0,token->str);
		++(*pc);
		return RESULT_STR;

	case TOK_VAR:
		/* Sets pc and result */
		if (getVariableValue(pc,result)) 
			return result->strval ? RESULT_STR : RESULT_NUM;
		break;

	case TOK_FUNC:
		/* callFunction() sets pc and result */
		if (callFunction(pc,end,result))
			return result->strval ? RESULT_STR : RESULT_NUM;
		break;

	default:
		basicError(ERR_SYNTAX,token);
	}
	return NO_RESULT;
}




/*** Evaluate entire stack down where whats left has lower precendence levels. 
     Returns 0 or 1 ***/
int evalStack(
	int min_prec,
	st_value *val_stack, 
	int *op_stack, int *val_stack_top, int *op_stack_top, st_token *token)
{
	st_value *val1;
	st_value *val2;
	int vp;
	int op;

	vp = *val_stack_top - 1;
	op = *op_stack_top - 1;
	for(;vp && op_info[op_stack[op]].prec >= min_prec;--op)
	{
		assert(vp > 0);
		assert(op >= 0);
		val2 = &val_stack[vp];
		val1 = &val_stack[--vp];

		/* Logical operators can work on mix of strings and numbers */
		switch(op_stack[op])
		{
		case OP_AND:
			setValue(val1,trueValue(val1) && trueValue(val2),NULL);
			continue;

		case OP_OR:
			setValue(val1,trueValue(val1) || trueValue(val2),NULL);
			continue;

		case OP_XOR:
			setValue(val1,trueValue(val1) ^ trueValue(val2),NULL);
			continue;
		}

		/* val1 and val2 must be the same types - ie both numbers or
		   both strings */
		if (!val1->strval != !val2->strval && op_stack[op] != OP_MULT)
			goto MISMATCH_ERROR;

		/* Only certain operators work with on strings */
		if (val1->strval)
		{
			/* Use setValue so it clears val1->strval */
			switch(op_stack[op])
			{
			case OP_EQUALS:
				setValue(val1,!strcmp(val1->strval,val2->strval),NULL);
				break;

			case OP_NOT_EQUALS:
				setValue(val1,!!strcmp(val1->strval,val2->strval),NULL);
				break;

			case OP_GREATER:
				setValue(val1,strcmp(val1->strval,val2->strval) > 0,NULL);
				break;

			case OP_GREATER_EQUALS:
				setValue(val1,strcmp(val1->strval,val2->strval) >= 0,NULL);
				break;

			case OP_LESS:
				setValue(val1,strcmp(val1->strval,val2->strval) < 0,NULL);
				break;

			case OP_LESS_EQUALS:
				setValue(val1,strcmp(val1->strval,val2->strval) <= 0,NULL);
				break;

			case OP_ADD:
				appendStringValue(val1,val2);
				break;

			case OP_MULT:
				if (val2->strval) goto MISMATCH_ERROR;
				if (val2->strval || val2->val < 0)
					goto ARG_ERROR;

				setDirectStringValue(
					val1,multStringValue(val1,val2->val));
				break;

			case OP_SUB:
			case OP_DIV:
			case OP_MOD:
			case OP_BIT_AND:
			case OP_BIT_OR:
			case OP_BIT_XOR:
			case OP_LEFT_SHIFT:
			case OP_RIGHT_SHIFT:
				goto ARG_ERROR;

			default:
				goto SYNTAX_ERROR;
			}
		}
		/* Numeric operators */
		else switch(op_stack[op])
		{
		case OP_EQUALS:
			val1->val = (val1->val == val2->val);
			break;

		case OP_NOT_EQUALS:
			val1->val = (val1->val != val2->val);
			break;

		case OP_GREATER:
			val1->val = (val1->val > val2->val);
			break;

		case OP_GREATER_EQUALS:
			val1->val = (val1->val >= val2->val);
			break;

		case OP_LESS:
			val1->val = (val1->val < val2->val);
			break;

		case OP_LESS_EQUALS:
			val1->val = (val1->val <= val2->val);
			break;

		case OP_ADD:
			val1->val += val2->val;
			break;

		case OP_SUB:
			val1->val -= val2->val;
			break;

		case OP_MULT:
			if (val2->strval)
			{
				if (val1->val < 0) goto ARG_ERROR;
				setDirectStringValue(
					val1,multStringValue(val2,val1->val));
			}
			else val1->val *= val2->val;
			break;

		case OP_DIV:
			if (!val2->val) goto DIV_ERROR;
			val1->val /= val2->val;
			break;

		case OP_BIT_AND:
			val1->val = (u_int)val1->val & (u_int)val2->val;
			break;

		case OP_BIT_OR:
			val1->val = (u_int)val1->val | (u_int)val2->val;
			break;

		case OP_BIT_XOR:
			val1->val = (u_int)val1->val ^ (u_int)val2->val;
			break;

		case OP_LEFT_SHIFT:
			/* Result of a left shift of >= 32 with int types is 
			   undefined in C so zero it */
			if ((u_int)val2->val >= 32)
				val1->val = 0;
			else
				val1->val = (u_int)val1->val << (u_int)val2->val;
			break;

		case OP_RIGHT_SHIFT:
			/* Right shifting isn't wrapped. Go figure */
			val1->val = (u_int)val1->val >> (u_int)val2->val;
			break;

		case OP_MOD:
			if (!val2->val) goto DIV_ERROR;
			val1->val = (u_int)val1->val % (u_int)val2->val;
			break;

		default:
			goto SYNTAX_ERROR;
		}
	} /* End for() */

	*op_stack_top = op + 1;
	*val_stack_top = vp + 1;
	return 1;

	DIV_ERROR:
	basicError(ERR_DIVIDE_BY_ZERO,token);
	return 0;

	ARG_ERROR:
	basicError(ERR_INVALID_ARGUMENT,token);
	return 0;

	MISMATCH_ERROR:
	basicError(ERR_ARGUMENT_MISMATCH,token);
	return 0;

	SYNTAX_ERROR:
	basicError(ERR_SYNTAX,token);
	return 0;
}




void clearStack(st_value *val_stack)
{
	int i;
	for(i=0;i < MAX_STACK;++i) clearValue(&val_stack[i]);
}




/*** We've evaluated the main expression, now evaluate with the pre operators
     NOT and bitwise complement. There is no precedence here, its first come
     first serve. eg: not ~ 1+2 ***/
void evalPreOpStack(int *pre_op_stack, int pre_op_stack_cnt, st_value *result)
{
	int i;

	for(i=0;i < pre_op_stack_cnt;++i)
	{
		if (pre_op_stack[i] == OP_NOT)
			result->val = !result->val;
		else if (pre_op_stack[i] == OP_BIT_COMPL)
			result->val = ~(u_int)result->val;
		else assert(0);
	}
}
