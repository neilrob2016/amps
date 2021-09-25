/*** Code for BASIC commands. eg: for-next, print ***/

#include "globals.h"

static int getNextDataLoc(int sect, int *data_end);
static void appendMesg(char **str, char *fmt, ...);


/*** Create the variables. Can have a list. eg: var a[10],b = 123,c = "abc" ***/
int comVar(int sect, int pc, st_token *token)
{
	st_token *token2;
	st_var *var;
	st_value result;
	int is_array;
	int end;
	int i;

	/* If we have the actual command "var" rather than just "a = ..." 
	   then its declaring a var so we need to create it */
	for(i=pc+1;i <= token->line_end_loc;)
	{
		/* Can't just check var->array_size to see if its an array
		   because user could have done var a[1] */
		if (!(var = createVariable(&i,&is_array))) return -1;
		token2 = &token_list[i];

		/* Check for initialisation. Can't init arrays, only simple
		   variables */
		if (IS_OP_TYPE(token2,OP_EQUALS))
		{
			if (is_array)
			{
				basicError(ERR_CANT_INIT_ARRAY,token);
				return -1;
			}
			/* Look for a comma that will mean more var defs follow
			   the expression */
			token2 = &token_list[i+1];
			if (token2->comma_loc)
				end = token2->comma_loc - 1;
			else
				end = token->line_end_loc;

			/* Set the value */
			switch(evalExpression(i+1,end,&result))
			{
			case NO_RESULT:
				return -1;

			case RESULT_NUM:
			case RESULT_STR:
				if (!setVariableByValue(var,NULL,0,&result,token))
					return -1;

				clearValue(&result);
				break;

			default:
				assert(0);
			}
			if (token2->comma_loc)
				i = token2->comma_loc + 1;
			else
				break;
		}
		else i += IS_OP_TYPE(token2,OP_COMMA);
	}
	return token->next_line_loc;
}




/*** Assign to a variable. Actual command is optional. eg: "let a = 1" or
     "a = 1" ***/
int comLet(int sect, int pc, st_token *token)
{
	st_value result;
	st_value index;
	st_var *var;

	/* Skip "let" */
	if (token->token_type == TOK_COM) ++pc;
	if (pc >= token->line_end_loc) goto ERROR;

	/* Get variable and index */
	initValue(&index);
	if (!(var = getVariableAndIndex(&pc,&index))) return -1;
	
	if (!token->checked)
	{
		/* Need = <expr> */
		if (++pc >= token->line_end_loc) goto ERROR;
		if (!IS_OP_TYPE(&token_list[pc],OP_EQUALS)) goto ERROR;
		++pc;
		token->checked = 1;
	}
	else pc += 2;

	/* Set the value */
	switch(evalExpression(pc,token->line_end_loc,&result))
	{
	case NO_RESULT:
		return -1;

	case RESULT_NUM:
	case RESULT_STR:
		if (!setVariableByValue(var,&index,0,&result,token))
			return -1;
		break;

	default:
		assert(0);
	}
	clearValue(&index);
	clearValue(&result);

	return token->next_line_loc;

	ERROR:
	basicError(ERR_SYNTAX,token);
	return -1;
}




int comIf(int sect, int pc, st_token *token)
{
	st_value result;
	st_if *if_block;

	if_block = token->u.if_block;
	assert(if_block);

	/* Get the expression result */
	if (evalExpression(pc+1,if_block->then_loc-1,&result) == NO_RESULT)
		return -1;

	/* If result is non zero or a non empty string then jump to first token
	   past THEN, else jump to first token past ELSE unless no ELSE in 
	   which case jump past FI */
	if (trueValue(&result))
		pc = if_block->then_loc+1;
	else
		pc = (if_block->else_loc ? if_block->else_loc+1 : if_block->fi_loc);
	clearValue(&result);
	return pc;
}




/*** Should never execute a THEN directly ***/
int comThen(int sect, int pc, st_token *token)
{
	basicError(ERR_UNEXPECTED_THEN,token);
	return -1;
}




/*** Skip ELSE section ***/
int comElse(int sect, int pc, st_token *token)
{
	if (!token->u.if_block)
	{
		basicError(ERR_UNEXPECTED_ELSE,token);
		return -1;
	}
	return token->u.if_block->fi_loc+1;
}




int comFor(int sect, int pc, st_token *token)
{
	st_loop *loop_block;
	st_value result;
	double val;
	int implied_step;
	int result_type;
	int end;

	loop_block = token->u.loop_block;
	assert(loop_block);

	/* If variable isn't set then get it. Can't use indexed variables.
	   eg: "for i = " but "for i[3] = " is not because it would be hassle
	   to store the index number then check in "next" */
	if (!token->var && !(token->var = getVariable(token_list[pc+1].str)))
	{
		basicError(ERR_UNDEFINED_VAR,token);
		return -1;
	}

	/* If we're already looping just update variable and check */
	if (loop_block->looped)
	{
		/* Get variable value, inc by step, check against TO value.
		   Don't just set var->arr[0].val directly in case a string
		   has been assigned to the var in the meantime */
		val = token->var->arr[0].val + loop_block->step_val;

		/* See if we've got there */
		if ((loop_block->step_val < 0 && val < loop_block->to_val) ||
		    (loop_block->step_val > 0 && val > loop_block->to_val))
		{
			pc = token_list[loop_block->end_loc].next_line_loc;
		}
		else
		{
			setVariable(token->var,NULL,0,val,NULL,token);	
			pc = token->next_line_loc;
		}
		loop_block->looped = 0;
		return pc;
	}

	/* Fresh entry into loop. Get "from" value */
	result_type = evalExpression(pc+3,loop_block->to_loc-1,&result);
	if (result_type == NO_RESULT) return 0;
	if (result_type == RESULT_STR) goto ERROR;
	loop_block->from_val = result.val;

	/* Get "to" value */
	end = (loop_block->step_loc ? 
	       loop_block->step_loc - 1 : token->line_end_loc);
	result_type = evalExpression(loop_block->to_loc+1,end,&result);
	if (result_type == NO_RESULT) return 0;
	if (result_type == RESULT_STR) goto ERROR;

	loop_block->to_val = result.val;

	/* Get implied stepping value */
	implied_step = SGN(loop_block->to_val - loop_block->from_val);
	if (!implied_step) implied_step = 1;

	/* Get optional step value */
	if (loop_block->step_loc)
	{
		result_type = evalExpression(
			loop_block->step_loc+1,token->line_end_loc,&result);

		if (result_type == NO_RESULT) return 0;
		if (result_type == RESULT_STR) goto ERROR;

		loop_block->step_val = result.val;
	}
	else loop_block->step_val = 1;

	/* Set the loop variable initial value regardless of whether
	   we execute the loop */
	setVariable(token->var,NULL,0,loop_block->from_val,NULL,token);	

	/* If the implied stepping value is not the same as the loop direction
	   then jump straight to end of loop. Keep looping of zero however */
	if (loop_block->step_val && SGN(loop_block->step_val) != implied_step)
		pc = token_list[loop_block->end_loc].next_line_loc;
	else
		pc = token->next_line_loc;

	clearValue(&result);
	return pc;

	ERROR:
	clearValue(&result);
	basicError(ERR_INVALID_ARGUMENT,token);
	return -1;
}




int comNext(int sect, int pc, st_token *token)
{
	if (token->u.loop_block)
	{
		token->u.loop_block->looped = 1;
		return token->u.loop_block->start_loc;
	}
	basicError(ERR_UNEXPECTED_NEXT,token);
	return -1;
}




int comWhile(int sect, int pc, st_token *token)
{
	st_loop *loop_block;
	st_value result;

	loop_block = token->u.loop_block;
	assert(loop_block);

	/* Get the expression result */
	if (evalExpression(pc+1,token->line_end_loc,&result) == NO_RESULT)
		return -1;

	pc = (trueValue(&result) ? token->next_line_loc : loop_block->end_loc + 1);
	clearValue(&result);
	return pc;
}




int comWend(int sect, int pc, st_token *token)
{
	if (token->u.loop_block) return token->u.loop_block->start_loc;
	basicError(ERR_UNEXPECTED_WEND,token);
	return -1;
}




int comDo(int sect, int pc, st_token *token)
{
	return token->next_line_loc;
}




int comUntil(int sect, int pc, st_token *token)
{
	st_loop *loop_block;
	st_value result;

	if (!(loop_block = token->u.loop_block))
	{
		basicError(ERR_UNEXPECTED_UNTIL,token);
		return -1;
	}

	/* Get the expression result */
	if (evalExpression(pc+1,token->line_end_loc,&result) == NO_RESULT)
		return -1;

	pc = (trueValue(&result) ? token->next_line_loc : loop_block->start_loc);
	clearValue(&result);

	return pc;
}




int comLoop(int sect, int pc, st_token *token)
{
	st_loop *loop_block;
	st_value result;

	loop_block = token->u.loop_block;

	/* New entry into loop - get count */
	if (!loop_block->looped)
	{
		switch(evalExpression(pc+1,token->line_end_loc,&result))			{
		case NO_RESULT:
			return -1;

		case RESULT_STR:
			basicError(ERR_INVALID_ARGUMENT,token);
			return -1;
		}
		loop_block->from_val = 0;
		loop_block->to_val = result.val;

		clearValue(&result);
	}

	/* See if we've looped the given number of times */
	if (loop_block->from_val == loop_block->to_val)
		pc = token_list[loop_block->end_loc].next_line_loc;
	else
	{
		++loop_block->from_val;
		pc = token->next_line_loc;
	}

	loop_block->looped = 0;
	return pc;
}




int comLend(int sect, int pc, st_token *token)
{
	if (token->u.loop_block)
	{
		token->u.loop_block->looped = 1;
		return token->u.loop_block->start_loc;
	}
	basicError(ERR_UNEXPECTED_LEND,token);
	return -1;
}




int comBreak(int sect, int pc, st_token *token)
{
	/* Jump to line following last loop command. Can't just use end_loc
	   on its own because UNTIL has an expression following it */
	if (token->u.loop_block)
		return token_list[token->u.loop_block->end_loc].next_line_loc;
	basicError(ERR_UNEXPECTED_BREAK_CONT,token);
	return -1;
}




int comContinue(int sect, int pc, st_token *token)
{
	/* Jump to loop end command because if its NEXT it has to set looped 
	   flag before jumping back to FOR */
	if (token->u.loop_block) return token->u.loop_block->end_loc;
	basicError(ERR_UNEXPECTED_BREAK_CONT,token);
	return -1;
}




int comGotoGosub(int sect, int pc, st_token *token)
{
	st_section *secptr;
	st_value result;
	int goto_loc;

	/* Get the label name to jump to */
	switch(evalExpression(++pc,token->line_end_loc,&result))
	{
	case NO_RESULT:
		return -1;

	case RESULT_NUM:
		/* Label names are strings */
		basicError(ERR_INVALID_ARGUMENT,token);
		return -1;

	case RESULT_STR:
		if (!(goto_loc = getLabelLocation(result.strval)))
		{
			clearValue(&result);
			basicError(ERR_UNDEFINED_LABEL,token);
			return -1;
		}
		clearValue(&result);

		/* If we're a GOTO just return with the jump location */
		if (token->token_num == COM_GOTO) return goto_loc;

		/* We're a GOSUB so store the return location */
		secptr = &section[sect];

		/* Need to set some sort of limit or process could run out of 
		   memory and crash */
		if (secptr->num_returns == 100000)
		{
			basicError(ERR_MAX_RECURSION,token);
			return -1;
		}
		if (secptr->returns_alloc <= secptr->num_returns)
		{
			secptr->returns_alloc += RETURNS_ALLOC;
			secptr->return_loc = (int *)realloc(
				secptr->return_loc,
				sizeof(int)*secptr->returns_alloc);
			assert(secptr->return_loc);
		}
		secptr->return_loc[secptr->num_returns++] = token->next_line_loc;
		return goto_loc;
	}
	assert(0);
	return -1;
}




int comReturn(int sect, int pc, st_token *token)
{
	if (!section[sect].num_returns)
	{
		basicError(ERR_UNEXPECTED_RETURN,token);
		return -1;
	}
	return section[sect].return_loc[--section[sect].num_returns];
}




int comRead(int sect, int pc, st_token *token)
{
	st_var *var;
	st_value result;
	st_value index;
	int end;
	int read_from;
	int read_to;
	int next_i;
	int i;

	/* Go through all the variables following READ that we need to read
	   into */
	for(i=pc+1;i <= token->line_end_loc;i = next_i)
	{
		if (token_list[i].comma_loc)
		{
			end = token_list[i].comma_loc - 1;
			next_i = end + 2;
		}
		else
		{
			end = token->line_end_loc;
			next_i = end + 1;
		}
		if (!(var = getVariableAndIndex(&i,&index))) return -1;

		if (!(read_from = getNextDataLoc(sect,&read_to)))
		{
			basicError(ERR_DATA_EXHAUSTED,token);
			return 0;
		}

		/* Evaluate expression at read location */
		switch(evalExpression(read_from,read_to,&result))
		{
		case NO_RESULT:
			return -1;

		case RESULT_NUM:
		case RESULT_STR:
			break;

		default:
			assert(0);
		}

		if (!setVariableByValue(var,&index,0,&result,token))
		{
			clearValue(&result);
			return -1;
		}
		clearValue(&result);
	}
	return token->next_line_loc;
}




/*** Returns the program counter for the next expression stored in DATA or 0
     if there isn't one ***/
int getNextDataLoc(int sect, int *read_to)
{
	st_token *read_token;
	int read_loc;

	read_loc = section[sect].read_loc;
	read_token = &token_list[read_loc];

	/* If we've onto a new line check its a DATA line */
	if (read_token->line_start_loc == read_loc)
	{
		/* If its not then see if we're doing auto restore */
		if (!IS_COM_TYPE(read_token,COM_DATA)) 
		{
			if (!section[sect].restore_loc) return 0;
			read_loc = section[sect].restore_loc;
		}

		/* Point to first bit of data */
		read_token = &token_list[++read_loc];
	}

	if (read_token->comma_loc)
	{
		*read_to = read_token->comma_loc - 1;
		section[sect].read_loc = *read_to + 2;
	}
	else
	{
		*read_to = read_token->line_end_loc;
		section[sect].read_loc = read_token->next_line_loc;
	}
	return read_loc;
}




int comRestore(int sect, int pc, st_token *token)
{
	st_value result;
	int label_loc;

	switch(evalExpression(++pc,token->line_end_loc,&result))
	{
	case NO_RESULT:
		return -1;

	case RESULT_NUM:
		basicError(ERR_INVALID_ARGUMENT,token);
		return -1;

	case RESULT_STR:
		break;

	default:
		assert(0);
	}
	if (!(label_loc = getLabelLocation(result.strval)))
	{
		clearValue(&result);
		basicError(ERR_UNDEFINED_LABEL,token);
		return -1;
	}
	clearValue(&result);

	/* Make sure there's a data statement immediately after the label */
	++label_loc;
	if (!IS_COM_TYPE(&token_list[label_loc],COM_DATA))
	{
		basicError(ERR_NO_DATA,token);
		return -1;
	}
	section[sect].read_loc = label_loc + 1;
	if (token->token_num == COM_AUTORESTORE)
		section[sect].restore_loc = label_loc;
	else
		section[sect].restore_loc = 0;

	return token->next_line_loc;
}




/*** Print to stdout and/or write in X window using message system ***/
int comPrint(int sect, int pc, st_token *token)
{
	st_token *expr_tok;
	st_value result;
	char *str[2];
	int strnum;
	int end;
	int tmp;
	int com;
	int no_nl;

	str[0] = NULL;
	str[1] = NULL;
	strnum = 0;
	com = token->token_num;

	/* Semi colon at the end means don't print a newline */
	no_nl = IS_OP_TYPE(&token_list[token->line_end_loc],OP_SEMI_COLON);

	for(++pc;pc <= token->line_end_loc - no_nl;)
	{
		expr_tok = &token_list[pc];
		if (expr_tok->comma_loc)
			end = expr_tok->comma_loc - 1;
		else
			end = token->line_end_loc - no_nl;
		
		/* Set the value */
		switch(evalExpression(pc,end,&result))
		{
		case NO_RESULT:
			return -1;

		case RESULT_NUM:
			if (com == COM_PRINT || com == COM_PRINTMESG)
			{
				printf("%f",result.val);
				fflush(stdout);
			}
			
			/* Check x_mode since we don't want to print the same
			   thing twice if in console mode */
			if (com == COM_MESG || (com == COM_PRINTMESG && x_mode))
			{
				appendMesg(&str[!strnum],"%s%f",
					str[strnum] ? str[strnum] : "",
					result.val);
				FREE(str[strnum]);
				strnum = !strnum;
			}
			break;

		case RESULT_STR:
			/* Could use write() but then the stdout order can
			   get mixed up if buffer not flushed */
			if (com == COM_PRINT || com == COM_PRINTMESG)
			{
				printf("%s",result.strval);
				fflush(stdout);
			}
			if (com == COM_MESG || (com == COM_PRINTMESG && x_mode))
			{
				appendMesg(&str[!strnum],"%s%.*s",
					str[strnum] ? str[strnum] : "",
					(int)strlen(result.strval),
					result.strval);
				FREE(str[strnum]);
				strnum = !strnum;
			}
			clearValue(&result);
			break;

		default:
			assert(0);
		}
		pc = end + 2;
	}
	if (com == COM_PRINT || com == COM_PRINTMESG)
	{
		if (!no_nl) putchar('\n');
		fflush(stdout);
	}
	if ((com == COM_MESG || (com == COM_PRINTMESG && x_mode)) && str[strnum])
	{
		tmp = do_messages;
		do_messages = 1;

		message(str[strnum]);
		free(str[strnum]);

		do_messages = tmp;
	}
	return token->next_line_loc;
}




void appendMesg(char **str, char *fmt, ...)
{
	va_list args;

	va_start(args,fmt);
	vasprintf(str,fmt,args);
	va_end(args);
}




int comSleep(int sect, int pc, st_token *token)
{
	st_value result;
	u_int secs;
	u_int usecs;
	double val;
	int ret;

	/* Sleep and timer both use sleep_until variable. Even if they used
	   seperate variables, using sleep would potentially cause a timing
	   mess if allowed in the TIMER */
	if (IS_TIMER_SECTION(sect))
	{
		basicError(ERR_SLEEP_IN_TIMER,token);
		return -1;
	}

	ret = evalExpression(pc+1,token->line_end_loc,&result);
	val = result.val;
	clearValue(&result);

	if (ret != RESULT_NUM) return -1;
	if (val <= 0) return token->next_line_loc;

	/* If its the main section then set sleep_until Otherwise actually 
	   sleep before we return */
	if (IS_MAIN_SECTION(sect))
		section[sect].sleep_until = getUsecTimeAsDouble() + val;
	else
	{
		secs = (u_int)val;
		usecs = (val - secs) * 1000000;

		/* 2 seperate calls because we could easily blow u_int max if 
		   we do it all in usecs */
		if (secs) sleep(secs);
		if (usecs) usleep(usecs);
	}

	return token->next_line_loc;
}




/*** Set the section to non runnable unless "program" option given ***/
int comExit(int sect, int pc, st_token *token)
{
	switch(token->line_end_loc - pc)
	{
	case 0:
		stopSection(sect,0);
		break;

	case 1:
		if (IS_COM_TYPE(&token_list[pc+1],COM_SUB_PROG)) 
		{
			stopProgram();
			dualMessage("BASIC: *** Program exited ***");
			break;
		}
		/* Fall through */

	default:
		basicError(ERR_SYNTAX,token);
		return -1;
	}
	return num_tokens;
}




/*** Quit AMPS entirely ***/
int comQuit(int sect, int pc, st_token *token)
{
	quit(QUIT_BASIC);
	return 0;
}




/*** Pause the BASIC program ***/
int comPause(int sect, int pc, st_token *token)
{
	if (token->line_end_loc != pc)
	{
		basicError(ERR_SYNTAX,token);
		return -1;
	}
	pauseProgram(1);
	return token->next_line_loc;
}




/*** Restart the BASIC program ***/
int comRestart(int sect, int pc, st_token *token)
{
	if (sect == SECTION_INIT)
	{
		basicError(ERR_RESTART_IN_INIT,token);
		return -1;
	}
	if (token->line_end_loc != pc)
	{
		basicError(ERR_SYNTAX,token);
		return -1;
	}
	restartProgram();
	return 0;
}




/*** Set yield after value ***/
int comYield(int sect, int pc, st_token *token)
{
	st_value result;

	switch(evalExpression(++pc,token->line_end_loc,&result))
	{
	case NO_RESULT:
		return -1;

	case RESULT_STR:
		clearValue(&result);
		goto ERROR;
		return -1;

	case RESULT_NUM:
		if (result.val < 0) goto ERROR;
		yield_after = (int)result.val;
		break;

	default:
		assert(0);
	}
	return token->next_line_loc;
	clearValue(&result);

	ERROR:
	basicError(ERR_INVALID_ARGUMENT,token);
	return -1;
}




/*** Block other sections from running ***/
int comBlock(int sect, int pc, st_token *token)
{
	if (token->line_end_loc != pc)
	{
		basicError(ERR_SYNTAX,token);
		return -1;
	}
	blocking_section = sect;
	return token->next_line_loc;
}




/*** Unblock other sections ***/
int comUnblock(int sect, int pc, st_token *token)
{
	if (token->line_end_loc != pc)
	{
		basicError(ERR_SYNTAX,token);
		return -1;
	}
	blocking_section = SECTION_INIT;
	return token->next_line_loc;
}




/*** Completely clear a variable of all key-value pairs and reset all array
     indexes to zero ***/
int comClear(int sect, int pc, st_token *token)
{
	st_token *vartok;

	if (token->line_end_loc == pc) goto ERROR;

	for(++pc;pc <= token->line_end_loc;++pc)
	{
		vartok = &token_list[pc];
		if (!vartok->checked)
		{
			if (vartok->token_type != TOK_VAR) goto ERROR;

			if (!vartok->var && 
			    !(vartok->var = getVariable(vartok->str)))
			{
				basicError(ERR_UNDEFINED_VAR,token);
				return -1;
			}
			vartok->checked = 1;
		}
		clearVariable(vartok->var);
		if (++pc > token->line_end_loc) break;
		if (!IS_OP_TYPE(&token_list[pc],OP_COMMA)) goto ERROR;
	}
	return token->next_line_loc;

	ERROR:
	basicError(ERR_SYNTAX,token);
	return -1;
}




/*** A cleaner way of doing set("RST",1) ***/
int comReset(int sect, int pc, st_token *token)
{
	if (token->line_end_loc != pc)
	{
		basicError(ERR_SYNTAX,token);
		return -1;
	}

	/* set to zero - don't want section button called */
	call_basic = 0;
	reset(0);
	call_basic = 1;

	return token->next_line_loc;
}




/*** Errors if we execute the command directly. For sub commands like TO, STEP,
     LOCK or UNLOCK ***/
int comSubCommand(int sect, int pc, st_token *token)
{
	basicError(ERR_SYNTAX,token);
	return -1;
}




/*** Default command function that simply jumps to next line ***/
int comDefault(int sect, int pc, st_token *token)
{
	return token->next_line_loc;
}
