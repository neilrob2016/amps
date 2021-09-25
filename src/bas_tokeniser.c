/*** BASIC tokeniser. Reads in the program and converts it into tokens ***/

#include "globals.h"

static void addCharToWord(char c);
static int addWordToTokenList();
static void addPaddingTokens();
static int setStartAndEndPositions();
static int findSectionStarts();

static void setLineStartAndEnd(int start, int end, int next);
static int parseIF(st_token *start_token, int start);
static int parseLoop(st_token *start_token, int start);
static int parseFORLoop(st_token *start_token, int start);
static int parseBrackets(st_token *start_token, int start, int lb, int rb);
static void lookForComma(st_token *start_token, int start);

static st_token *newToken();
static void deleteToken();
static int isSubtract();
static int isValidName(char *name);

static char *word;
static int word_len;
static int word_alloc;
static int file_linenum;
static int basic_linenum;
static int negative;


/*** Load the program file and convert it into tokens ***/
int tokenise(char *filename)
{
	char bas_filename[PATH_MAX+1];
	st_token *prev_token; 
	FILE *fp;
	int in_string;
	int in_comment;
	int line_cont;
	int len;
	char *opchars = "()[]=<>+-*/%,:&|^~;";
	char c2_op[3];
	char c;
	int i;

	len = strlen(filename);
	if (len < 4 || strcmp(filename+len-4,BASIC_SUFFIX))
		sprintf(bas_filename,"%s.bas",filename);
	else
		strcpy(bas_filename,filename);

	if (!(fp = fopen(bas_filename,"r")))
	{
		dualMessage("BASIC: ERROR: Can't open file '%s': %s",
			bas_filename,strerror(errno));
		return 0;
	}

	word = NULL;
	word_len = 0;
	word_alloc = 0;
	file_linenum = 1;
	basic_linenum = 1;
	negative = 0;

	in_string = 0;
	in_comment = 0;
	line_cont = 0;

	c = getc(fp);
	while(!feof(fp))
	{
		switch(c)
		{
		case ':':
			if (!in_string && !in_comment)
			{
				if (!word_len)
				{
					prev_token = &token_list[num_tokens-1];
					if (basic_linenum != prev_token->basic_linenum ||
					    IS_OP_TYPE(prev_token,OP_COLON))
					{
						basicError(ERR_SYNTAX,prev_token);
						return 0;
					}
				}
				if (!addWordToTokenList()) return 0;
			}
			break;

		case '#':
			if (!in_string && !in_comment)
			{
				in_comment = 1;
				goto NEXT;
			}
			break;

		case '\\':
			if (in_string || in_comment) break;

			/* Must follow with \n for line continuation otherwise
			   just ignore it */
			if ((c = getc(fp)) != '\n') goto NEXT;
			line_cont = 1;
			/* Fall through */

		case '\n':
			if (in_string) addCharToWord(c);
			else 
			{
				if (in_comment) in_comment = 0;
				else if (!addWordToTokenList()) return 0;

				if (!line_cont) ++basic_linenum;
			}
			line_cont = 0;
			++file_linenum;
			goto NEXT;

		case '"':
			if (in_comment) break;
			if (in_string)
			{
				in_string = 0;
				if (!addWordToTokenList()) return 0;
			}
			else
			{
				/* Add " to start of word so we know if its
				   a string later */
				addCharToWord(c);
				in_string = 1;
			}
			goto NEXT;

		case ' ':
		case TAB:
			if (in_string)
				addCharToWord(c);
			else if (!addWordToTokenList())
				return 0;
			goto NEXT;
		}

		if (in_comment) goto NEXT;
		if (in_string)
		{
			addCharToWord(c);
			goto NEXT;
		}
		if (strchr(opchars,c))
		{
			/* Add current word to list */
			if (!addWordToTokenList()) return 0;

			/* Put op in new word */
			addCharToWord(c);

			/* Peek at next char to see if its a double op.
			   ie: <>, ==, <=, >=, <<, >> */
			if (c == '<' || c == '>' || c == '=')
			{
				c2_op[0] = c;
				c2_op[1] = getc(fp);
				c2_op[2] = 0;

				for(i=0;i < NUM_OPS;++i)
				{
					if (op_info[i].str[1] && 
					    !op_info[i].str[2] &&
					    !strcmp(c2_op,op_info[i].str))
					{
						addCharToWord(c2_op[1]);
						c = c2_op[1];
						break;
					}
				}
				if (i == NUM_OPS) ungetc(c2_op[1],fp);
			}
			
			/* Add this to list */
			if (!addWordToTokenList()) return 0;
			goto NEXT;
		}
		else addCharToWord(c);

		NEXT:
		c = getc(fp);
	}
	if (in_string) 
	{
		basicError(
			ERR_MISSING_END_QUOTES,
			&token_list[num_tokens-1]);
		return 0;
	}
	if (!addWordToTokenList()) return 0;

	/* We add these so when parsing we don't have to constantly check if 
	   we've gone off the end. These provide a safety buffer */
	addPaddingTokens();

	fclose(fp);
	if (!setStartAndEndPositions()) return 0;
	dualMessage(
		"BASIC: Loaded %d file lines, %d BASIC lines",
		file_linenum-1,basic_linenum-1);
	return findSectionStarts();
}




void addCharToWord(char c)
{
	/* -1 so there's always room for \0 on end */
	if (word_len >= word_alloc - 1)
	{
		word_alloc += WORD_STR_REALLOC;
		word = (char *)realloc(word,sizeof(char) * word_alloc);
		assert(word);
	}
	word[word_len++] = c;
}




/*** Add work to token list and try and figure out what it is. 
     Returns 1 or 0 ***/
int addWordToTokenList()
{
	st_token *token; 
	int num_type;
	int i;

	if (!word) return 1;

	word[word_len] = 0;

	token = newToken();
	token->file_linenum = file_linenum;
	token->basic_linenum = basic_linenum;
	token->str = word;

	switch(word[0])
	{
	case '@':
		if (num_tokens == 1) goto NOT_SECTION_ERROR;
		if (negative)
		{
			basicError(ERR_INVALID_NEGATIVE,token);
			return 0;
		}
		if (!isValidName(word+1))
		{
			basicError(ERR_INVALID_LABEL,token);
			return 0;
		}
		token->token_type = TOK_LABEL;
		addLabel(word,num_tokens-1);
		goto DONE;

	case '"':
		if (num_tokens == 1) goto NOT_SECTION_ERROR;
		if (negative)
		{
			basicError(ERR_INVALID_NEGATIVE,token);
			return 0;
		}
		memmove(word,word+1,word_len);
		token->token_type = TOK_STR;
		goto DONE;
	}
	if ((num_type = isNumber(word,0)) != NOT_NUM)
	{
		if (num_tokens == 1) goto NOT_SECTION_ERROR;

		token->token_type = TOK_NUM;

		switch(num_type)
		{
		case NUM_DEC:
			token->val = atof(word);
			break;

		case NUM_OCT:
			token->val = strtoll(word,NULL,8);
			break;

		case NUM_HEX:
			token->val = strtoll(word,NULL,16);
			break;

		default:
			assert(0);
		}
		token->negative = negative;
		goto DONE;
	}
	for(i=0;i < NUM_COMMANDS;++i)
	{
		if (!strcasecmp(command[i].name,word))
		{
			if (negative)
			{
				basicError(ERR_SYNTAX,token);
				return 0;
			}
			if (i == COM_SECTION) token->token_type = TOK_SECTION;
			else
			{
				if (num_tokens == 1) goto NOT_SECTION_ERROR;
				token->token_type = TOK_COM;
				token->token_num = i;
			}
			goto DONE;
		}
	}

	if (num_tokens == 1) goto NOT_SECTION_ERROR;

	/* For backwards compat. Will be removed eventually */
	if (!strcasecmp(word,"tointstr"))
		strcpy(word,func_info[FUNC_INTSTR].name);
	else if (!strcasecmp(word,"tohexstr"))
		strcpy(word,func_info[FUNC_HEXSTR].name);
	else if (!strcasecmp(word,"tooctstr"))
		strcpy(word,func_info[FUNC_OCTSTR].name);

	/* Check against function name */
	for (i=0;i < NUM_FUNCTIONS;++i)
	{
		if (!strcasecmp(func_info[i].name,word))
		{
			token->token_type = TOK_FUNC;
			token->token_num = i;
			token->negative = negative;
			goto DONE;
		}
	}
	for(i=0;i < NUM_OPS;++i)
	{
		if (!strcasecmp(op_info[i].str,word))
		{
			/* Check if we have a subtraction or a potential
			   negative value coming up */
			if (i == OP_SUB)
			{
				if (isSubtract()) negative = 0;
				else
				{
					negative = 1;
					deleteToken();
					goto END;
				}
			}
			token->token_type = TOK_OP;
			token->token_num = i;
			token->negative = negative;
			goto DONE;
		}
	}
	if (!isValidName(word))
	{
		basicError(ERR_INVALID_VAR,token);
		return 0;
	}
	token->token_type = TOK_VAR;
	token->negative = negative;

	DONE:
	negative = 0;

	END:
	word = NULL;
	word_len = 0;
	word_alloc = 0;
	return 1;

	NOT_SECTION_ERROR:
	basicError(ERR_SECTION_NOT_FIRST,token);
	return 0;
}




/*** Add padding tokens to the end of the list ***/
void addPaddingTokens()
{
	st_token *token; 
	int i;

	for(i=0;i < NUM_PAD_TOKENS;++i)
	{
		token = newToken();
		token->token_type = TOK_PADDING;
		token->file_linenum = file_linenum + 1;
		token->basic_linenum = basic_linenum + 1;
		token->str = "PADDING";
	}
}




/*** Set the end positions of the lines and brackets ***/
int setStartAndEndPositions()
{
	st_token *token;
	int line_start;
	int i;

	line_start = 0;

	for(i=0;i < num_tokens;++i)
	{
		token = &token_list[i];

		/* Look for IF or loop blocks */
		if (token->token_type == TOK_COM)
		{
			switch(token->token_num)
			{
			case COM_IF:
				if (!parseIF(token,i)) return 0;
				break;

			case COM_WHILE:
			case COM_DO:
			case COM_LOOP:
				if (!parseLoop(token,i)) return 0;
				break;

			case COM_FOR:
				if (!parseFORLoop(token,i)) return 0;
				break;

			default:
				break;
			}
		}

		/* See if we're at the end of the line */
		if (token->token_type == TOK_COM)
		{
			switch(token->token_num)
			{
			/* Sub commands are not end of lines */
			case COM_SUB_TO:
			case COM_SUB_STEP:
			case COM_SUB_THEN:
			case COM_SUB_LOCK:
			case COM_SUB_UNLOCK:
			case COM_SUB_PROG:
				break;

			default:
				setLineStartAndEnd(line_start,i-1,i);
				line_start = i;
				continue;
			}
		}
		if (i && token->basic_linenum != token_list[i-1].basic_linenum)
		{
			setLineStartAndEnd(line_start,i-1,i);
			line_start = i;
			continue;
		}


		/* Look for colon starting a new line and skip it */
		if (IS_OP_TYPE(token,OP_COLON))
		{
			setLineStartAndEnd(line_start,i-1,i+1);
			line_start = i;
			continue;
		}

		/* Look for matching brackets */
		if (IS_OP_TYPE(token,OP_L_RND_BRACKET))
		{
			if (!parseBrackets(token,i,OP_L_RND_BRACKET,OP_R_RND_BRACKET))
				return 0;
			lookForComma(token,i);
		}
		else if (IS_OP_TYPE(token,OP_L_SQR_BRACKET))
		{
			if (!parseBrackets(token,i,OP_L_SQR_BRACKET,OP_R_SQR_BRACKET))
				return 0;
			lookForComma(token,i);
		}
		else switch(token->token_type)
		{
		case TOK_VAR:
		case TOK_FUNC:
		case TOK_STR:
		case TOK_NUM:
			lookForComma(token,i);
		}
	}

	/* If not set for last line then do so */
	if (!token_list[line_start].line_end_loc)
		setLineStartAndEnd(line_start,num_tokens-1,num_tokens);
	return 1;
}




void setLineStartAndEnd(int start, int end, int next)
{
	int i;
	for(i=start;i <= end;++i)
	{
		token_list[i].line_start_loc = start;
		token_list[i].line_end_loc = end;
		token_list[i].next_line_loc = next;
	}
}




/*** Find the locations of THEN, ELSE and FI/FIALL for an IF statement ***/
int parseIF(st_token *start_token, int start)
{
	st_token *token;
	st_if *if_block;
	int nesting;
	int i;

	nesting = 1;

	if_block = (st_if *)malloc(sizeof(st_if));
	assert(if_block);
	bzero(if_block,sizeof(st_if));
	start_token->u.if_block = if_block;

	for(i=start+1;i < num_tokens;++i)
	{
		token = &token_list[i];

		if (token->token_type == TOK_SECTION) goto MISSING_FI;
		if (token->token_type != TOK_COM) continue;

		switch(token->token_num)
		{
		case COM_IF:
			++nesting;
			break;

		case COM_SUB_THEN:
			if (nesting != 1) break;
			/* Can only have 1 THEN */
			if (if_block->then_loc)
			{
				basicError(ERR_UNEXPECTED_THEN,start_token);
				return 0;
			}
			/* Must have an expression between IF-THEN */
			if (i - start < 2) 
			{
				basicError(ERR_SYNTAX,start_token);
				return 0;
			}
			if_block->then_loc = i;
			break;
			
		case COM_ELSE:
			if (nesting != 1) break;
			if (if_block->else_loc || !if_block->then_loc)
			{
				basicError(ERR_UNEXPECTED_ELSE,start_token);
				return 0;
			}
			if_block->else_loc = i;
			token_list[i].u.if_block = if_block;
			break;

		case COM_FI:
			if (!--nesting)
			{
				/* Found end of block */
				if_block->fi_loc = i;
				goto CHECK;
			}
			if (nesting < 0)
			{
				basicError(ERR_UNEXPECTED_FI,start_token);
				return 0;
			}
			break;

		case COM_FIALL:
			if_block->fi_loc = i;
			goto CHECK;
		}
	}

	CHECK:
	if (!if_block->then_loc) 
	{
		basicError(ERR_MISSING_THEN,start_token);
		return 0;
	}
	if (if_block->fi_loc) return 1;

	MISSING_FI:
	basicError(ERR_MISSING_FI,start_token);
	return 0;
}




/*** Find the ends of WHILE-WEND, DO-UNTIL and LOOP-LEND loops ***/
int parseLoop(st_token *start_token, int start)
{
	st_token *token;
	st_loop *loop_block;
	int nesting;
	int end_com;
	int err;
	int i;

	loop_block = (st_loop *)malloc(sizeof(st_loop));
	assert(loop_block);
	bzero(loop_block,sizeof(st_loop));
	loop_block->start_loc = start;
	
	start_token->u.loop_block = loop_block;

	switch(start_token->token_num)
	{
	case COM_WHILE:
		end_com = COM_WEND;
		err = ERR_MISSING_WEND;
		break;

	case COM_DO:
		end_com = COM_UNTIL;
		err = ERR_MISSING_UNTIL;
		break;

	case COM_LOOP:
		end_com = COM_LEND;
		err = ERR_MISSING_LEND;
		break;

	default:
		assert(0);
	}
	nesting = 1;

	for(i=start+1;i < num_tokens;++i)
	{
		token = &token_list[i];

		if (token->token_type == TOK_SECTION) break;
		if (token->token_type != TOK_COM) continue;

		/* If the break and continue don't belong to this loop the
		   loop_block be overwritten later */
		if (token->token_num == COM_BREAK || 
		    token->token_num == COM_CONTINUE)
			token->u.loop_block = loop_block;
		else if (token->token_num == start_token->token_num)
			++nesting;
		else if (token->token_num == end_com && !--nesting)
		{
			loop_block->end_loc = i;
			token->u.loop_block = loop_block;
			return 1;
		}
	}

	basicError(err,start_token);
	return 0;
}




/*** Find the locations of TO, STEP & NEXT for FOR-NEXT loops ***/
int parseFORLoop(st_token *start_token, int start)
{
	st_loop *loop_block;
	st_token *token;
	char *varname;
	int parsed;
	int i;

	loop_block = (st_loop *)malloc(sizeof(st_loop));
	assert(loop_block);
	bzero(loop_block,sizeof(st_loop));
	loop_block->start_loc = start;
	
	start_token->u.loop_block = loop_block;

	/* Should start with "FOR <var> = ". Don't allow indexed arrays since
	   we wouldn't be able to find correct NEXT if index value is an
	   expression */
	if (token_list[start+1].token_type != TOK_VAR ||
	    !IS_OP_TYPE(&token_list[start+2],OP_EQUALS)) goto ERROR;

	varname = token_list[start+1].str;

	/* Find TO, optional STEP and correct NEXT. Ie has correct following
	   variable */
	parsed = 0;
	for(i=start+3;i < num_tokens;++i)
	{
		token = &token_list[i];
		if (token->token_type == TOK_SECTION) break;
		if (token->token_type != TOK_COM) continue;

		switch(token->token_num)
		{
		case COM_SUB_TO:	
			if (!parsed)
			{
				if (loop_block->step_loc) goto ERROR;
				loop_block->to_loc = i;
			}
			break;

		case COM_SUB_STEP:
			if (!parsed)
			{
				if (!loop_block->to_loc) 
				{
					basicError(ERR_MISSING_TO,start_token);
					return 0;
				}
				loop_block->step_loc = i;
			}
			break;

		case COM_NEXT:
			if (token_list[i+1].token_type != TOK_VAR)
			{
				basicError(ERR_SYNTAX,&token_list[i]);
				return 0;
			}
			if (!strcmp(token_list[i+1].str,varname))
			{
				loop_block->end_loc = i;
				token->u.loop_block = loop_block;
				return 1;
			}
			parsed = 1;
			break;

		case COM_BREAK:
		case COM_CONTINUE:
			token->u.loop_block = loop_block;
			break;
			
		default:
			if (!loop_block->to_loc)
			{
				basicError(ERR_MISSING_TO,start_token);
				return 0;
			}
			parsed = 1;
		}
	}
	if (!loop_block->end_loc)
	{
		basicError(ERR_MISSING_NEXT,start_token);
		return 0;
	}
	return 1;

	ERROR:
	basicError(ERR_SYNTAX,start_token);
	return 0;
}




/*** Check for matching close brackets ***/
int parseBrackets(st_token *start_token, int start, int lb, int rb)
{
	st_token *token;
	int nesting;
	int i;

	nesting = 0;

	for(i=start+1;i < num_tokens;++i)
	{
		token = &token_list[i];

		switch(token->token_type)
		{
		case TOK_SECTION:
		case TOK_COM:
		case TOK_LABEL:
			/* If we haven't found it by the start of the next
			   line then there's an error */
			goto ERROR;
		}

		if (IS_OP_TYPE(token,OP_COLON)) goto ERROR;

		if (IS_OP_TYPE(token,lb)) ++nesting;
		else if (IS_OP_TYPE(token,rb))
		{
			if (nesting) --nesting;
			else 
			{
				start_token->u.close_bracket_loc = i;
				break;
			}
		}
	}
	if (i < num_tokens) return 1;

	ERROR:
	basicError(ERR_MISSING_BRACKET,start_token);
	return 0;
}




/*** Look for a comma that may terminate the expression ***/
void lookForComma(st_token *start_token, int start)
{
	st_token *token;
	int rnd_nesting;
	int sqr_nesting;
	int i;

	rnd_nesting = 0;
	sqr_nesting = 0;

	if (IS_OP_TYPE(start_token,OP_L_RND_BRACKET)) ++rnd_nesting;
	else if (IS_OP_TYPE(start_token,OP_L_SQR_BRACKET)) ++sqr_nesting;

	/* Look for any comma terminating an expression */
	for(i=start+1;i < num_tokens;++i)
	{
		token = &token_list[i];

		switch(token->token_type)
		{
		case TOK_SECTION:
		case TOK_COM:
		case TOK_LABEL:
			/* Stop searching at start of the next the line */
			return;
		}

		if (IS_OP_TYPE(token,OP_COLON)) return;

		if (IS_OP_TYPE(token,OP_L_RND_BRACKET))
			++rnd_nesting;
		if (IS_OP_TYPE(token,OP_L_SQR_BRACKET))
			++sqr_nesting;
		else if (IS_OP_TYPE(token,OP_R_RND_BRACKET))
			--rnd_nesting;
		else if (IS_OP_TYPE(token,OP_R_SQR_BRACKET))
			--sqr_nesting;

		/* If we've hit a close bracket (but didn't see the open) then
		   we're at the end of an expression or array index so stop */
		if (rnd_nesting < 0 || sqr_nesting < 0) break;

		if (!rnd_nesting && !sqr_nesting && IS_OP_TYPE(token,OP_COMMA))
		{
			start_token->comma_loc = i;
			break;
		}
	}
}




int findSectionStarts()
{
	st_token *token;
	st_token *int_token;
	char *name;
	int pos;
	int sect;
	int sect_cnt;

	sect_cnt = 0;

	for(pos=0;pos < num_tokens;++pos)
	{
		token = &token_list[pos];

		if (token->token_type != TOK_SECTION) continue;

		if (pos >= num_tokens - NUM_PAD_TOKENS - 1 || 
		    pos == token->line_end_loc)
		{
			basicError(ERR_SYNTAX,token);
			return 0;
		}

		token = &token_list[pos+1];
		name = token->str;

		/* MAIN is an alias for MAIN0 */
		if (!strcasecmp(name,"MAIN")) name = "MAIN0";

		/* TIMER is an alias for TIMER0 */
		if (!strcasecmp(name,"TIMER")) name = "TIMER0";

		for(sect=0;sect < NUM_SECTIONS;++sect)
		{
			if (strcasecmp(name,section[sect].name)) continue;

			if (section[sect].start_loc)
			{
				basicError(ERR_DUPLICATE_SECTION,token);
				return 0;
			}

			dualMessage("BASIC: Initialising section '%s'...",
				section[sect].name);

			pos += 2;

			/* If its a TIMER section so we need the interval */
			if (IS_TIMER_SECTION(sect))
			{
			    	if (token->line_end_loc < pos)
				{
					basicError(ERR_PERIOD_MISSING,token);
					return 0;
				}
				int_token = &token_list[pos];
				if (int_token->token_type != TOK_NUM ||
				    int_token->val <= 0)
				{
					basicError(ERR_INVALID_PERIOD,int_token);
					return 0;
				}
				if (int_token->val < (double)USEC_LOOP_DELAY / 1000000)
				{
					basicError(ERR_PERIOD_TOO_SMALL,int_token);
					return 0;
				}
				section[sect].timer_interval = int_token->val;
				section[sect].start_loc = ++pos;
			}
			else
			{
				if (token->line_end_loc == pos)
				{
					basicError(ERR_SYNTAX,&token_list[pos]);
					return 0;
				}
				section[sect].start_loc = pos;
			}

			section[sect].pc = section[sect].start_loc;
			section[sect].runnable = 1;
			++runnable_sections;	
			++sect_cnt;
			break;
		}
		if (sect == NUM_SECTIONS)
		{
			basicError(ERR_INVALID_SECTION,token);
			return 0;
		}
	}
	dualMessage("BASIC: Initialised %d sections",sect_cnt);
	return 1;
}



/***************************** SUPPORT FUNCTIONS ****************************/

/*** Create a new token in the list and return it ***/
st_token *newToken()
{
	st_token *token;

	if (num_tokens >= num_tokens_alloc)
	{
		num_tokens_alloc += WORD_LIST_REALLOC;
		token_list = (st_token *)realloc(
			token_list,sizeof(st_token) * num_tokens_alloc);
		assert(token_list);
	}
	token = &token_list[num_tokens++];
	bzero(token,sizeof(st_token));
	return token;
}




/*** Delete current word ***/
void deleteToken()
{
	--num_tokens;
	FREE(token_list[num_tokens].str);
}




/*** If a minus follows certain types then its a subtract op, otherwise a
     negative op ***/
int isSubtract()
{
	st_token *prev_token;

	if (num_tokens < 2) return 0;

	prev_token = &token_list[num_tokens-2];

	/* If one of the following is before the '-' then its not a negative 
	   number:
	      number
	      variable
	      right round bracket
	      colon
	*/
	switch(prev_token->token_type)
	{
	case TOK_VAR:
	case TOK_NUM:
		return 1;

	case TOK_OP:
		if (prev_token->token_num == OP_R_RND_BRACKET ||
		    prev_token->token_num == OP_COLON) return 1;
	}
	return 0;
}




int isValidName(char *name)
{
	char *s;

	if (!*name) return 0;
	if (*name == '$') ++name;
	for(s=name;*s;++s)
		if (!isalpha(*s) && !isdigit(*s) && *s != '_') return 0;
	return 1;
}
