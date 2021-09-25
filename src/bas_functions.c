/*** BASIC language functions code. eg: instr() ***/

#include "globals.h"

#define DEGS_PER_RADIAN 57.29578
#define SYS_MAP_ADD     100

static void clearArgs(st_value *args, int cnt);

static st_var *func_arg_var;

static char *getset_field[NUM_GETSET_FIELDS] = 
{
	/* 0 */
	"YIELD",
	"FILTER",
	"FREQ",
	"NOTE",
	"SCALE",

	/* 5 */
	"PLAY",
	"MESG",
	"IO_MESG",
	"TITLE",
	"KEY_START_NOTE",

	/* 10 */
	"MAIN_OSC",
	"MOUSE_TAIL",
	"FREEZE",
	"FILL",
	"DIR",

	/* 15 */
	"TIMER_CATCHUP",
	"EFFECTS_SEQ",
	"WIN_WIDTH",
	"WIN_HEIGHT"
};




/*** Called from init() ***/
void setupGSFields()
{
	ENTRY e;
	int i;
	char name[50];

	for(i=0;i < NUM_GETSET_FIELDS;++i)
	{
		sprintf(name,"SYS:%s",getset_field[i]);
		e.key = strdup(name);
		e.data = (void *)(long)(i + SYS_MAP_ADD);
		assert(hsearch(e,ENTER));
	}
}




/*** Entry point for calling functions ***/
int callFunction(int *pc, int end, st_value *result)
{
	st_value args[MAX_FUNC_PARAMS];
	st_token *func_token;
	st_token *var_token;
	st_token *token;
	int params;
	int ptype;
	int i;
	int ret;

	func_token = &token_list[*pc];
	params = func_info[func_token->token_num].num_params;

	if (!func_token->checked)
	{
		/* Point at '(' */
		++(*pc);

		/* Check we have brackets following function name */
		if (*pc > end || !IS_OP_TYPE((&token_list[*pc]),OP_L_RND_BRACKET))
		{
			basicError(ERR_SYNTAX,func_token);
			return 0;
		}
		token = &token_list[*pc];
		assert(token->u.close_bracket_loc);

		/* Point at start of expression inside brackets */
		++(*pc);
		if (!params && token->u.close_bracket_loc != *pc)
		{
			basicError(ERR_TOO_MANY_PARAMS,func_token);
			return 0;
		}

		func_token->checked = 1;
	}
	else
	{
		token = &token_list[++(*pc)];
		++(*pc);
	}

	/* Init first arg as a result arg for functions with no params */
	if (!params)
	{
		initValue(&args[0]);
		++(*pc);
	}

	/* Get value(s) inside brackets */
	for(i=0;(params < 0 || i < params) && 
	        *pc < token->u.close_bracket_loc;++i)
	{
		initValue(&args[i]);
		if (token_list[*pc].comma_loc)
		{
			if (i == MAX_FUNC_PARAMS - 1 || i == params - 1)
			{
				ret = ERR_TOO_MANY_PARAMS;
				goto ERROR2;
			}
			end = token_list[*pc].comma_loc - 1;
		}
		else end = token->u.close_bracket_loc - 1;

		/* Currently for variadic functions, all argument types must
		   be the same */
		if (params < 0)
			ptype = func_info[func_token->token_num].param_type[0];
		else
			ptype = func_info[func_token->token_num].param_type[i];

		/* The key functions and arrsize are exceptions - first 
		   parameter is a variable, not an expression value */
		if (ptype == PARAM_VAR)
		{
			/* Only for 1st argument */
			assert(!i);

			var_token = &token_list[*pc];
			if (var_token->token_type != TOK_VAR)
			{
				ret = 0;
				goto ERROR1;
			}
			if (!var_token->var &&
			    !(var_token->var = getVariable(var_token->str)))
			{
				ret = ERR_UNDEFINED_VAR;
				goto ERROR2;
			}
			func_arg_var = var_token->var;
		}
		else if (evalExpression(*pc,end,&args[i]) == NO_RESULT)
		{
			/* evalExpression() will have done error */
			clearArgs(args,i+1);
			return 0;
		}
		else 
		{
			ret = 0;

			if ((ptype == PARAM_STR && !args[i].strval) ||
			    (ptype == PARAM_NUM && args[i].strval))
			{
				++i;  /* Because eval has set top arg */
				goto ERROR1;
			}
		}
		*pc = end + 2;
	}

	/* Variadics min parameter count negative, hence abs */
	if (i < abs(params))
	{
		basicError(ERR_MISSING_PARAMS,func_token);
		clearArgs(args,i);
		return 0;
	}

	/* arg contains the values inside the function brackets but args[0] 
	   also acts as the return result */
	ret = (*func_info[func_token->token_num].func)(
		func_token->token_num,i,args);
	if (ret > 0)
	{
		if (func_token->negative)
		{
			if (args[0].strval) 
			{
				basicError(ERR_INVALID_NEGATIVE,func_token);
				clearArgs(args,i);
				return 0;
			}
			args[0].val = -args[0].val;
		}
		setValueByValue(result,&args[0]);
		clearArgs(args,i);
		return 1;
	}
	/* Fall through */

	/* If ret < 0 then its the negative error code */
	ERROR1:
	basicError(ret < 0 ? -ret : ERR_INVALID_ARGUMENT,func_token);
	clearArgs(args,i);
	return 0;

	ERROR2:
	basicError(ret,func_token);
	clearArgs(args,i);
	return 0;
}




void clearArgs(st_value *args, int cnt)
{
	int i;
	for(i=0;i < cnt;++i) clearValue(&args[i]);
}


/**************************** SUPPORT FUNCTIONS ******************************/

/*** Get the count of the number of fields ***/
int getFieldCntInString(st_value *args, int onesep)
{
	char *str;
	char *s;
	char *e;
	char sep;
	int cnt;

	str = args[0].strval;

	/* Need string, seperator char */
	if (strlen(args[1].strval) != 1) return 0;

	sep = args[1].strval[0];
	s = str;
	cnt = 0;

	/* If we're looking at strictly one seperator between fields then
	   if the string is non empty then thats a minumum of 1 field */
	if (onesep && *s) ++cnt;

	while(1)
	{
		/* Find start of word */
		for(;*s && *s == sep;++s) cnt += onesep;
		if (!*s) break;

		/* Find end of word */
		for(e=s;*e && *e != sep;++e);

		if (!onesep) ++cnt;
		if (!*e) break;
		s = e;
	}
	setValue(&args[0],cnt,NULL);
	return 1;
}




/*** Get a word from a string passing the string, word number (starting from
     zero) and the delimiter. If onesep set then only 1 seperator character
     is allowed between words, else an unlimited amount. ***/
int getFieldFromString(st_value *args, int onesep)
{
	char *str;
	char *s;
	char *e;
	char sep;
	int pos;
	int cnt;

	pos = args[1].val;

	/* Need string, field position, seperator char */
	if (pos < 0 || strlen(args[2].strval) != 1) return 0;

	/* Duplicate it - can't set it from itself */
	str = strdup(args[0].strval);

	sep = args[2].strval[0];
	s = str;

	for(cnt=0;;)
	{
		/* Find start of word */
		for(;*s && *s == sep && cnt <= pos;++s) cnt += onesep;

		if (!*s || cnt > pos) break;

		/* Find end of word */
		for(e=s;*e && *e != sep;++e);

		if (cnt == pos)
		{
			*e = 0;
			setValue(&args[0],0,s);
			free(str);
			return 1;
		}
		if (!*e) break;
		s = e;
		if (!onesep) ++cnt;
	}
	setValue(&args[0],0,"");
	free(str);
	return 1;
}


/**************************** FUNCTION FUNCTIONS *****************************/

/*** Get various system values, eg buttons ***/
int funcGet(int func, int arg_cnt, st_value *args)
{
	int but;

	if ((but = getButton(args[0].strval)) == -1)
		return -ERR_INVALID_GET_FIELD;

	if (but < SYS_MAP_ADD)
	{
		setValue(&args[0],getButtonValue(but),NULL);
		return 1;
	}
	but -= SYS_MAP_ADD;

	switch(but)
	{
	case GS_YIELD:
		setValue(&args[0],yield_after,NULL);
		break;

	case GS_FILTER:
		setValue(&args[0],shm->filter_val,NULL);
		break;

	case GS_FREQ:
		setValue(&args[0],shm->freq,NULL);
		break;

	case GS_NOTE:
		setValue(&args[0],shm->note,NULL);
		break;

	case GS_SCALE:
		setValue(&args[0],shm->note_scale,NULL);
		break;

	case GS_PLAY:
		setValue(&args[0],shm->play,NULL);
		break;

	case GS_MESG:
		setValue(&args[0],auto_messages,NULL);
		break;

	case GS_IO_MESG:
		setValue(&args[0],io_messages,NULL);
		break;

	case GS_TITLE:
		setValue(&args[0],0,title_str);
		break;

	case GS_KEY_START_NOTE:
		setValue(&args[0],params.key_start_note,NULL);
		break;

	case GS_MAIN_OSC:
		setValue(&args[0],shm->sound,NULL);
		break;

	case GS_MOUSE_TAIL:
		setValue(&args[0],draw_tail,NULL);
		break;

	case GS_FREEZE:
		setValue(&args[0],freeze_waveform,NULL);
		break;

	case GS_FILL:
		setValue(&args[0],params.fill_waveform,NULL);
		break;

	case GS_DIR:
		setValue(&args[0],0,getcwd(dirbuff,sizeof(dirbuff)));
		break;

	case GS_TIMER_CATCHUP:
		setValue(&args[0],do_timer_catchup,NULL);
		break;
		
	case GS_EFFECTS_SEQ:
		setValue(&args[0],0,effects_seq_str);
		break;

	case GS_WIN_WIDTH:
		setValue(&args[0],win_width,NULL);
		break;

	case GS_WIN_HEIGHT:
		setValue(&args[0],win_height,NULL);
		break;

	default:
		assert(0);
	}
	return 1;
}




/*** Set runtime synth parameters ***/
int funcSet(int func, int arg_cnt, st_value *args)
{
	char *strval;
	double val;
	int but;
	int tmp;
	int ret;

	val = args[1].val;
	strval = args[1].strval;
	call_basic = 0;

	/* Check for ON/OFF or YES/NO strings */
	if (strval)
	{
		if (!strcasecmp(strval,"ON") || !strcasecmp(strval,"YES"))
		{
			val = 1;
			strval = NULL;
		}
		else if (!strcasecmp(strval,"OFF") || !strcasecmp(strval,"NO"))
		{
			val = 0;
			strval = NULL;
		}
	}

	if ((but = getButton(args[0].strval)) == -1)
		return -ERR_INVALID_SET_FIELD;

	if (but < SYS_MAP_ADD)
	{
		/* Returns button number else negative error */
		if ((but = setButtonField(but,strval,val)) < 0) return but;

		setValue(&args[0],getButtonValue(but),NULL);
		call_basic = 1;
		return 1;
	}

	but -= SYS_MAP_ADD;

	/* Some fields allow string values other than ON/OFF or YES/NO */
	if (strval)
	{
		switch(but)
		{
		case GS_TITLE:
		case GS_NOTE:
		case GS_SCALE:
		case GS_MAIN_OSC:
		case GS_KEY_START_NOTE:
		case GS_DIR:
		case GS_EFFECTS_SEQ:
			break;

		default:
			goto INVALID_VALUE;
		}
	}

	/* call_basic isn't just set to 1 here because various functions
	   called below in turn call runEventSection() and if it was set to 1 
	   already we'd get recursion */
	switch(but)
	{
	case GS_YIELD:
		if (val < 0) goto INVALID_VALUE;
		yield_after = val;
		setValue(&args[0],yield_after,NULL);
		break;

	case GS_FILTER:
		setFilter(val,0);
		setValue(&args[0],shm->filter_val,NULL);
		break;

	case GS_FREQ:
		setFreq(val);
		setValue(&args[0],shm->freq,NULL);
		break;

	case GS_NOTE:
		/* If val < 0 then its an error */
		if (strval && (val = getNoteFromName(strval,shm->note_scale)) < 0)
		{
			call_basic = 1;
			return val;
		}
		setNoteAndFreq(val);
		setValue(&args[0],shm->note,NULL);
		break;

	case GS_SCALE:
		if (!setScale(strval,val))
		{
			call_basic = 1;
			return -ERR_INVALID_SCALE;
		}
		setValue(&args[0],shm->note_scale,NULL);
		break;

	case GS_PLAY:
		if (val)
			playOn(0);
		else
			playOff(0);
		setValue(&args[0],shm->play,NULL);
		break;

	case GS_MESG:
		auto_messages = (val != 0);
		setValue(&args[0],auto_messages,NULL);
		break;

	case GS_IO_MESG:
		io_messages = (val != 0);
		setValue(&args[0],io_messages,NULL);
		break;

	case GS_TITLE:
		if (!strval) goto INVALID_VALUE;

		/* If empty string set title to default */
		if (strval[0])
			setTitleBar(strval);
		else
			setTitleBar(NULL);
		setValue(&args[0],0,title_str);
		break;

	case GS_KEY_START_NOTE:
		/* If val < 0 then its an error */
		if (strval && (val = getNoteFromName(strval,shm->note_scale)) < 0)
		{
			call_basic = 1;
			return val;
		}
		setKeyStartNote((char)val);
		setValue(&args[0],params.key_start_note,NULL);
		break;

	case GS_MAIN_OSC:
		/* If its a string check its a sound type. OFF not
		   allowed for main oscillator */
		if (strval) val = getSoundNumberFromName(strval);
		if (val <= SND_OFF || val > SND_SAMPLE)
		{
			call_basic = 1;
			return -ERR_INVALID_WAVEFORM;
		}
		if (val == SND_SAMPLE && !do_sampling)
		{
			call_basic = 1;
			return -ERR_SAMPLING_UNAVAILABLE;
		}
		setMainOsc((u_char)val);
		updateMainOscButtons();
		setValue(&args[0],val,NULL);
		break;
		
	case GS_MOUSE_TAIL:
		draw_tail = (val != 0);
		setValue(&args[0],draw_tail,NULL);
		break;

	case GS_FREEZE:
		setFreezeWaveform((int)val);
		setValue(&args[0],freeze_waveform,NULL);
		break;

	case GS_FILL:
		setFillWaveform((int)val);
		setValue(&args[0],params.fill_waveform,NULL);
		break;

	case GS_DIR:
		/* Don't fail - let BASIC program find out by checking
		   before and after values */
		if (chdir(args[1].strval) == -1)
		{
			tmp = do_messages;
			do_messages = io_messages;
			message("ERROR: chdir(): %s",strerror(errno));
			do_messages = tmp;
		}
		setValue(&args[0],0,getcwd(dirbuff,sizeof(dirbuff)));
		break;

	case GS_TIMER_CATCHUP:
		do_timer_catchup = (val != 0);
		setValue(&args[0],do_timer_catchup,NULL);
		break;
		
	case GS_EFFECTS_SEQ:
		call_basic = 1;
		ret = setEffectsSeqByString(strval);
		return (ret == OK) ? 1 : -ret;

	case GS_WIN_WIDTH:
		if (val < 1) goto INVALID_VALUE;
		if (val > display_width) val = display_width;
	        XResizeWindow(display,win,val,win_height);
		return 1;

	case GS_WIN_HEIGHT:
		if (val < 1) goto INVALID_VALUE;
		if (val > display_height) val = display_height;
	        XResizeWindow(display,win,win_width,val);
		return 1;
		
	default:
		/* Not all get() fields are valid set() fields. eg
		   can't set mode. */
		call_basic = 1;
		return -ERR_INVALID_SET_FIELD;
	}
	call_basic = 1;
	return 1;

	INVALID_VALUE:
	call_basic = 1;
	return -ERR_INVALID_SET_VALUE;
}




int funcTrig(int func, int arg_cnt, st_value *args)
{
	double radians;

	/* Convert to radians */
	radians = args[0].val / DEGS_PER_RADIAN;

	switch(func)
	{
	case FUNC_SIN:
		args[0].val = sin(radians);
		break;

	case FUNC_COS:
		args[0].val = cos(radians);
		break;

	case FUNC_TAN:
		args[0].val = tan(radians);
		break;

	default:
		assert(0);
	}
	return 1;
}




int funcArcTrig(int func, int arg_cnt, st_value *args)
{
	switch(func)
	{
	case FUNC_ARCSIN:
		if (args[0].val < -1 || args[0].val > 1)
			return -ERR_OUT_OF_RANGE;
		args[0].val = asin(args[0].val);
		break;

	case FUNC_ARCCOS:
		if (args[0].val < -1 || args[0].val > 1)
			return -ERR_OUT_OF_RANGE;
		args[0].val = acos(args[0].val);
		break;

	case FUNC_ARCTAN:
		args[0].val = atan(args[0].val);
		break;

	default:
		assert(0);
	}

	/* Convert to degrees */
	args[0].val *= DEGS_PER_RADIAN;
	return 1;
}




int funcSqrt(int func, int arg_cnt, st_value *args)
{
	if (args[0].val < 0) return 0;
	args[0].val = sqrt(args[0].val);
	return 1;
}




int funcAbs(int func, int arg_cnt, st_value *args)
{
	args[0].val = fabs(args[0].val);
	return 1;
}




int funcSgn(int func, int arg_cnt, st_value *args)
{
	args[0].val = SGN(args[0].val);
	return 1;
}




int funcRandom(int func, int arg_cnt, st_value *args)
{
	/* +1 on mod so value goes from 0 -> val inclusive */
	args[0].val = floor((args[0].val + 1) * 
			    (double)(random() % RAND_MAX) / RAND_MAX);
	return 1;
}




int funcPow(int func, int arg_cnt, st_value *args)
{
	args[0].val = pow(args[0].val,args[1].val);
	return 1;
}




int funcLog(int func, int arg_cnt, st_value *args)
{
	if (args[0].val <= 0) return 0;

	switch(func)
	{
	case FUNC_LOG:
		args[0].val = log(args[0].val);
		break;

	case FUNC_LOG2:
#ifdef NO_LOG2
		return -ERR_FUNCTION_UNAVAILABLE;
#else
		args[0].val = log2(args[0].val);
#endif
		break;

	case FUNC_LOG10:
		args[0].val = log10(args[0].val);
		break;

	default:
		assert(0);
	}

	return 1;
}




/*** Some systems seem to have problems compiling in the built in round so
     lets role our own here ***/
int funcRound(int func, int arg_cnt, st_value *args)
{
	double val;

	val = fabs(args[0].val);
	if (val - (int)val >= 0.5) 
		val = (int)val + 1;
	else
		val = (int)val;

	args[0].val = val * SGN(args[0].val);
	return 1;
}




int funcFloor(int func, int arg_cnt, st_value *args)
{
	args[0].val = (double)floor(args[0].val);
	return 1;
}




int funcCeil(int func, int arg_cnt, st_value *args)
{
	args[0].val = (double)ceil(args[0].val);
	return 1;
}




int funcInt(int func, int arg_cnt, st_value *args)
{
	args[0].val = (double)(int)args[0].val;
	return 1;
}




int funcInStr(int func, int arg_cnt, st_value *args)
{
	char *haystack;
	char *needle;
	char *ptr;
	int start;
	int len;

	haystack = args[0].strval;
	needle = args[1].strval;
	start = (int)args[2].val;

	if (start < 0) return 0;

	len = strlen(haystack);

	if (start >= len)
	{
		setValue(&args[0],-1,NULL);
		return 1;
	}
	if (!(ptr = strstr(haystack+start,needle)))
		setValue(&args[0],-1,NULL);
	else
		setValue(&args[0],(double)(ptr - haystack),NULL);
	return 1;
}




int funcSubStr(int func, int arg_cnt, st_value *args)
{
	char *str;
	char *substr;
	int from;
	int sublen;
	int len;

	str = args[0].strval;
	from = (int)args[1].val;
	sublen = (int)args[2].val;

	if (from < 0 || sublen < 0) return 0;

	len = strlen(str);
	if (from >= len)
	{
		substr = strdup("");
		assert(substr);
	}
	else
	{
		len -= from;
		if (sublen > len) sublen = len;
		substr = (char *)malloc(sublen+1);
		assert(substr);
		if (sublen) strncpy(substr,str+from,sublen);
		substr[sublen] = 0;
	}

	/* For efficiency. Don't need setValue() to copy substr as its already 
	   malloc'd */
	clearValue(&args[0]);
	args[0].strval = substr;

	return 1;
}




int funcNumStr(int func, int arg_cnt, st_value *args)
{
	char str[30];

	switch(func)
	{
	case FUNC_TOSTR:
		sprintf(str,"%f",args[0].val);
		break;

	case FUNC_INTSTR:
		sprintf(str,"%d",(int)args[0].val);
		break;

	case FUNC_OCTSTR:
		sprintf(str,"%o",(u_int)args[0].val);
		break;

	case FUNC_HEXSTR:
		sprintf(str,"%X",(u_int)args[0].val);
		break;

	default:
		assert(0);
	}
	setValue(&args[0],0,str);
	return 1;
}




int funcBinStr(int func, int arg_cnt, st_value *args)
{
	char str[33];
	u_int val;
	int bit;
	int bitval;
	int strpos;
	int add;

	if (!(val = (u_int)args[0].val))
	{
		setValue(&args[0],0,"0");
		return 1;
	}
	add = 0;
	strpos = 0;

	/* Start from the top and work down */
	for(bit=31;bit >= 0;--bit)
	{
		bitval = !!(val & (1 << bit));

		/* Don't add to string until we hit first 1 */
		if (bitval) add = 1;
		if (add)
		{
			str[strpos] = '0' + bitval;
			++strpos;
		}
	}
	str[strpos] = 0;
	setValue(&args[0],0,str);
	return 1;
}




/*** Converts a string to a number ***/
int funcToNum(int func, int arg_cnt, st_value *args)
{ 
	switch(isNumber(args[0].strval,1))
	{
	case NOT_NUM:
		setValue(&args[0],0,NULL);
		break;

	case NUM_OCT:
		setValue(&args[0],strtoll(args[0].strval,NULL,8),NULL);
		break;

	case NUM_DEC:
		setValue(&args[0],atof(args[0].strval),NULL);
		break;

	case NUM_HEX:
		setValue(&args[0],strtoll(args[0].strval,NULL,16),NULL);
		break;

	default:
		assert(0);
	}
	return 1;
}




/*** Returns 1 if the string is a valid number ***/
int funcIsNumStr(int func, int arg_cnt, st_value *args)
{
	setValue(&args[0],isNumber(args[0].strval,1),NULL);
	return 1;
}




/*** Return 1 if the variable is a number type, not a string ***/
int funcIsNumType(int func, int arg_cnt, st_value *args)
{
	setValue(&args[0],args[0].strval == NULL,NULL);
	return 1;
}




int funcUpperLowerStr(int func, int arg_cnt, st_value *args)
{
	int (*fptr)(int);
	char *s;

	fptr = (func == FUNC_UPPERSTR) ? toupper : tolower;
	for(s=args[0].strval;*s;++s) *s = (*fptr)(*s);
	return 1;
}




int funcStrLen(int func, int arg_cnt, st_value *args)
{
	setValue(&args[0],strlen(args[0].strval),NULL);
	return 1;
}




/*** Returns 1 if the variable has the string key in its dictionary. Uses
     args[1] because args[0] is the variable and thats already been checked
     in callFunction()  ***/
int funcHasKey(int func, int arg_cnt, st_value *args)
{
	if (getDictionaryElementByKey(func_arg_var,args[1].strval))
		setValue(&args[0],1,NULL);
	else
		setValue(&args[0],0,NULL);
	return 1;
}




/*** Attempts to delete element from variable dictionary with given key.
     Returns 1 if key found else 0 ***/
int funcDelKey(int func, int arg_cnt, st_value *args)
{
	if (deleteDictionaryElement(func_arg_var,args[1].strval))
		setValue(&args[0],1,NULL);
	else
		setValue(&args[0],0,NULL);
	return 1;
}




/*** Returns the key at given key position ***/
int funcGetKey(int func, int arg_cnt, st_value *args)
{
	st_dict *dict;

	if ((dict = getDictionaryElementByNumber(
		func_arg_var,(int)args[1].val)))
	{
		setValue(&args[0],0,dict->key);
		return 1;
	}
	return -ERR_ARR_INDEX_OOB;
}




/*** Return the number of keys the variable has ***/
int funcKeyCnt(int func, int arg_cnt, st_value *args)
{
	setValue(&args[0],func_arg_var->dict_size,NULL);
	return 1;
}




/*** Get the field from the string ***/
int funcField(int func, int arg_cnt, st_value *args)
{
	/* if 2nd argument is true then only allow 1 seperator */
	return getFieldFromString(args,func == FUNC_FIELD1);
}




int funcFieldCnt(int func, int arg_cnt, st_value *args)
{
	return getFieldCntInString(args,func == FUNC_FIELDCNT1);
}




/*** Return the size of the variables array. This does NOT include any
     dictionary keys ***/
int funcArrSize(int func, int arg_cnt, st_value *args)
{
	setValue(&args[0],func_arg_var->array_size,NULL);
	return 1;
}




/*** Load or save a patch ***/
int funcLoadSave(int func, int arg_cnt, st_value *args)
{
	int tmp;

	if (strlen(args[0].strval) > PATH_MAX - 4) 
		return -ERR_FILENAME_TOO_LONG;
	sprintf(disk.filename,"%s.amp",args[0].strval);
	
	tmp = do_messages;
	do_messages = io_messages;
	setValue(&args[0],func == FUNC_LOAD ? loadFile(1) : saveFile(1),NULL);
	do_messages = tmp;

	return 1;
}




/*** Return the ASCII code for a character ***/
int funcAsc(int func, int arg_cnt, st_value *args)
{
	if (strlen(args[0].strval) == 1) 
	{
		setValue(&args[0],args[0].strval[0],NULL);
		return 1;
	}
	return 0;
}




/*** Return the character for a given ASCII code ***/
int funcChr(int func, int arg_cnt, st_value *args)
{
	char c[2];

	if (args[0].val >= 0 && args[0].val <= 255)
	{
		c[0] = (char)args[0].val;
		c[1] = 0;
		setValue(&args[0],0,c);
		return 1;
	}
	return 0;
}




int funcMaxMin(int func, int arg_cnt, st_value *args)
{
	double val;
	int i;

	val = args[0].val;
	for(i=1;i < arg_cnt;++i)
	{
		if ((func == FUNC_MAX && args[i].val > val) ||
		    (func == FUNC_MIN && args[i].val < val))
			val = args[i].val;
	}
	setValue(&args[0],val,NULL);
	return 1;
}




int funcGetScaleName(int func, int arg_cnt, st_value *args)
{
	int scale;

	scale = args[0].val;
	if (scale < 0 || scale >= NUM_SCALES) return -ERR_OUT_OF_RANGE;

	setValue(
		&args[0],0,
		func == FUNC_GETLONGSCALENAME ? scale_name[scale] : 
		                                scale_short_name[scale]);
	return 1;
}




int funcGetScaleByName(int func, int arg_cnt, st_value *args)
{
	int scale = getScaleFromName(args[0].strval);

	if (scale == -1) return -ERR_INVALID_SCALE;
	setValue(&args[0],scale,NULL);
	return 1;
}




int funcGetNoteInfo(int func, int arg_cnt, st_value *args)
{
	int note;
	int scale;

	note = args[0].val;
	scale = args[1].val;
	if (note < 0 || note >= NUM_NOTES || scale < 0 || scale >= NUM_SCALES)
		return -ERR_OUT_OF_RANGE;

	switch(func)
	{
	case FUNC_GETNOTENAME:
		setValue(&args[0],0,getNoteName(note,scale));
		break;

	case FUNC_GETNOTEFREQ:
		setValue(&args[0],note_freq_scale[scale][note],NULL);
		break;

	default:
		assert(0);
	}
	return 1;
}




int funcGetNoteByName(int func, int arg_cnt, st_value *args)
{
	int scale;
	int note;

	scale = args[1].val;
	if (scale < 0 || scale >= NUM_SCALES) return -ERR_OUT_OF_RANGE;
	if ((note = getNoteFromName(args[0].strval,scale)) < 0) return note;
	setValue(&args[0],note,NULL);
	return 1;
}




int funcGetNoteByFreq(int func, int arg_cnt, st_value *args)
{
	int freq;
	int scale;
	int note;
	int curr_freq;
	int prev_freq;

	freq = args[0].val;
	scale = args[1].val;
	if (scale < 0 || scale >= NUM_SCALES) return -ERR_OUT_OF_RANGE;

	/* Go through the list of frequencies and find the nearest */
	for(note=0;note < NUM_NOTES;++note)
	{
		curr_freq = note_freq_scale[scale][note];
		if (freq < curr_freq)
		{
			/* See if we're closer to current or previous */
			if (note && freq - prev_freq < curr_freq - freq)
				--note;
			break;
		}
		prev_freq = curr_freq;
	}
	setValue(&args[0],note,NULL);
	return 1;
}




int funcGetSoundName(int func, int arg_cnt, st_value *args)
{
	int snd = args[0].val;

	if (snd < 0 || snd >= NUM_SND_TYPES) return -ERR_OUT_OF_RANGE;
	setValue(&args[0],0,sound_name[snd]);
	return 1;
}




int funcGetSoundByName(int func, int arg_cnt, st_value *args)
{
	int snd;

	for(snd=0;snd < NUM_SND_TYPES;++snd)
		if (!strcasecmp(sound_name[snd],args[0].strval)) break;

	setValue(&args[0],snd < NUM_SND_TYPES ? snd : -1,NULL);
	return 1;
}




int funcGetChordName(int func, int arg_cnt, st_value *args)
{
	int chord = args[0].val;

	if (chord < 0 || chord >= NUM_CHORDS) return -ERR_OUT_OF_RANGE;
	setValue(&args[0],0,chord_name[chord]);
	return 1;
}




int funcGetChordByName(int func, int arg_cnt, st_value *args)
{
	int chord;

	// Don't need strcasecmp because chord names don't have letters
	for(chord=0;chord < NUM_CHORDS;++chord)
		if (!strcmp(chord_name[chord],args[0].strval)) break;

	setValue(&args[0],chord < NUM_CHORDS ? chord : -1,NULL);
	return 1;
}




int funcGetARPSeqName(int func, int arg_cnt, st_value *args)
{
	int seq = args[0].val;

	if (seq < 0 || seq >= NUM_ARP_SEQS) return -ERR_OUT_OF_RANGE;
	setValue(&args[0],0,arp_seq_name[seq]);
	return 1;
}




int funcGetARPSeqByName(int func, int arg_cnt, st_value *args)
{
	int seq;

	for(seq=0;seq < NUM_ARP_SEQS;++seq)
		if (!strcasecmp(arp_seq_name[seq],args[0].strval)) break;
	setValue(&args[0],seq < NUM_ARP_SEQS ? seq : -1,NULL);
	return 1;
}




int funcGetResModeName(int func, int arg_cnt, st_value *args)
{
	int mode = args[0].val;

	if (mode < 0 || mode >= NUM_RES_MODES) return -ERR_OUT_OF_RANGE;
	setValue(&args[0],0,resonance_mode[mode]);
	return 1;
}




int funcGetResModeByName(int func, int arg_cnt, st_value *args)
{
	int mode;

	for(mode=0;mode < NUM_RES_MODES;++mode)
		if (!strcasecmp(resonance_mode[mode],args[0].strval)) break;
	setValue(&args[0],mode < NUM_RES_MODES ? mode : -1,NULL);
	return 1;
}




int funcGetPhasingModeName(int func, int arg_cnt, st_value *args)
{
	int mode = args[0].val;

	if (mode < 0 || mode >= NUM_PHASING_MODES) return -ERR_OUT_OF_RANGE;
	setValue(&args[0],0,phasing_mode[mode]);
	return 1;
}




int funcGetPhasingModeByName(int func, int arg_cnt, st_value *args)
{
	int mode;

	for(mode=0;mode < NUM_PHASING_MODES;++mode)
		if (!strcasecmp(phasing_mode[mode],args[0].strval)) break;
	setValue(&args[0],mode < NUM_PHASING_MODES ? mode : -1,NULL);
	return 1;
}




int funcGetRingRangeName(int func, int arg_cnt, st_value *args)
{
	int range = args[0].val;

	if (range < 0 || range >= NUM_RING_RANGES) return -ERR_OUT_OF_RANGE;
	setValue(&args[0],0,ring_range[range]);
	return 1;
}




int funcGetRingRangeByName(int func, int arg_cnt, st_value *args)
{
	int range;

	for(range=0;range < NUM_RING_RANGES;++range)
		if (!strcasecmp(ring_range[range],args[0].strval)) break;
	setValue(&args[0],range < NUM_RING_RANGES ? range : -1,NULL);
	return 1;
}




int funcGetRingModeName(int func, int arg_cnt, st_value *args)
{
	int mode = args[0].val;

	if (mode < 0 || mode >= NUM_RING_MODES) return -ERR_OUT_OF_RANGE;
	setValue(&args[0],0,ring_mode[mode]);
	return 1;
}




int funcGetRingModeByName(int func, int arg_cnt, st_value *args)
{
	int mode;

	for(mode=0;mode < NUM_RING_MODES;++mode)
		if (!strcasecmp(ring_mode[mode],args[0].strval)) break;
	setValue(&args[0],mode < NUM_RING_MODES ? mode : -1,NULL);
	return 1;
}
