/*** Support for BASIC language variables. eg: var a = 123 ***/

#include "globals.h"

static st_var *allocateVariable(char *name, int arr_size);
static int getIndex(int pc, int end, st_value *index, int creating);
static int setDictionaryValue(
	st_var *var, st_value *key, double num, char *str);
static void sysTime(st_value *value);
static void sysUptime(st_value *value);


/*** Set up initial stuff ***/
void initVariableArrays()
{
	bzero(first_var,sizeof(first_var));
	bzero(last_var,sizeof(last_var));
}




/*** Variables that are read only and begin with '$' ***/
void createSystemVariables()
{
	char *varname[NUM_SYSTEM_VARS] =
	{
		/* 0 */
		"",

		/** Non event variables  - ie always set to some value **/
		"$version",
		"$build_date",
		"$build_options",
		"$time",

		/* 5 */
		"$uptime",
		"$pi",
		"$e",
		"$num_snd_types",
		"$num_chords",

		/* 10 */
		"$num_arp_seqs",
		"$num_phasing_modes",
		"$num_res_modes",
		"$num_freq_modes",
		"$num_notes",

		/* 15 */
		"$key_start_note",
		"$num_scales",
		"$num_ring_ranges",
		"$num_ring_modes",
		"$num_timers",

		/* 20 */
		"$timer_interval",
		"$arg",
		"$true",
		"$false",
		"$event_type",

		/* 25 */
		"$dial",
		"$dial_value",
		"$button",
		"$button_value",
		"$button_value_name",

		/* 30 */
		"$freq",
		"$note",
		"$note_name",
		"$scale",
		"$scale_name",

		/* 35 */
		"$function_key",
		"$section",
		"$restart_num",
		"$span",
		"$span_max_index",

		/* 40 */
		"$filter",
		"$effects_seq",
		"$win_mapped",
		"$win_width",
		"$win_height",

		/* 45 */
		"$mode",
		"$sound_enabled",
		"$sound_system",
		"$sampling_enabled",
		"$com_exec_cnt",
	};
	st_value index;
	int arr_size;
	int i;
	int j;

	bzero(system_var,sizeof(system_var));

	for(i=1;i < NUM_SYSTEM_VARS;++i)
	{
		if (i == SVAR_TIMER_INTERVAL) arr_size = 5;
		else if (i == SVAR_SPAN) arr_size = MAX_UCHAR;
		else arr_size = 1;

		system_var[i] = allocateVariable(varname[i],arr_size);
		system_var[i]->sysvar_num = i;

		/* A lot of these are fixed at boot time and never change,
		   others have values dynamically returned in getVariableValue()
		   and yet more are dynamically set around the program */
		switch(i)
		{
		case SVAR_VERSION:
			setVariable(system_var[i],NULL,1,0,VERSION,0);
			break;

		case SVAR_BUILD_DATE:
			setVariable(system_var[i],NULL,1,0,BUILD_DATE,0);
			break;

		case SVAR_BUILD_OPTIONS:
			setVariable(system_var[i],NULL,1,0,build_options,0);
			break;

		case SVAR_PI:
			setVariable(system_var[i],NULL,1,3.14159265358979323846,NULL,0);
			break;

		case SVAR_E:
			setVariable(system_var[i],NULL,1,2.71828182845904523536,NULL,0);
			break;

		case SVAR_NUM_SND_TYPES:
			setVariable(system_var[i],NULL,1,NUM_SND_TYPES,NULL,0);
			break;

		case SVAR_NUM_CHORDS:
			setVariable(system_var[i],NULL,1,NUM_CHORDS,NULL,0);
			break;

		case SVAR_NUM_ARP_SEQS:
			setVariable(system_var[i],NULL,1,NUM_ARP_SEQS,NULL,0);
			break;

		case SVAR_NUM_PHASING_MODES:
			setVariable(system_var[i],NULL,1,NUM_PHASING_MODES,NULL,0);
			break;

		case SVAR_NUM_RES_MODES:
			setVariable(system_var[i],NULL,1,NUM_RES_MODES,NULL,0);
			break;

		case SVAR_NUM_FREQ_MODES:
			setVariable(system_var[i],NULL,1,NUM_FREQ_MODES,NULL,0);
			break;

		case SVAR_NUM_NOTES:
			setVariable(system_var[i],NULL,1,NUM_NOTES,NULL,0);
			break;

		case SVAR_NUM_SCALES:
			setVariable(system_var[i],NULL,1,NUM_SCALES,NULL,0);
			break;

		case SVAR_NUM_RING_RANGES:
			setVariable(system_var[i],NULL,1,NUM_RING_RANGES,NULL,0);
			break;

		case SVAR_NUM_RING_MODES:
			setVariable(system_var[i],NULL,1,NUM_RING_MODES,NULL,0);
			break;

		case SVAR_NUM_TIMERS:
			setVariable(system_var[i],NULL,1,NUM_TIMERS,NULL,0);
			break;

		case SVAR_KEY_START_NOTE:
			setVariable(system_var[i],NULL,1,params.key_start_note,NULL,0);
			break;

		case SVAR_RESTART_NUM:
			setVariable(system_var[i],NULL,1,restart_cnt,NULL,0);
			break;

		case SVAR_ARG:
			setVariable(system_var[i],NULL,1,0,basic_arg,0);
			break;

		case SVAR_TRUE:
			setVariable(system_var[i],NULL,1,1,NULL,0);
			break;

		case SVAR_FALSE:
			setVariable(system_var[i],NULL,1,0,NULL,0);
			break;

		case SVAR_TIMER_INTERVAL:
			initValue(&index);

			for(j=0;j < NUM_TIMERS;++j)
			{
				setValue(&index,j,NULL);
				setVariable(
					system_var[i],&index,1,
					section[SECTION_TIMER0+j].timer_interval,
					NULL,0);
			}
			break;

		case SVAR_MODE:
			setVariable(
				system_var[i],
				NULL,1,-1,x_mode ? "GUI" : "CONSOLE",0);
			break;

		case SVAR_SOUND_ENABLED:
			setVariable(system_var[i],NULL,1,do_sound,NULL,0);
			break;

		case SVAR_SOUND_SYSTEM:
			setVariable(system_var[i],NULL,1,0,sound_system,0);
			break;
		}
	}
}




/*** Program counter should be pointing at "var" command. Sets pc to token
     following var definition ***/
st_var *createVariable(int *pc, int *is_array)
{
	st_token *token;
	st_token *var_token;
	st_token *br_token;
	st_value arr_size;
	int end;

	/* Check with have a variable name following "var" */
	token = &token_list[*pc];
	if (token->token_type != TOK_VAR || *pc > token->line_end_loc)
	{
		basicError(ERR_SYNTAX,token);
		return NULL;
	}

	/* Check to see if variable is already defined and get array size */
	var_token = &token_list[*pc];
	if (var_token->token_type != TOK_VAR)
	{
		basicError(ERR_INVALID_VAR,token);
		return NULL;
	}

	/* Only system variables can begin with a dollar */
	if (var_token->str[0] == '$')
	{
		basicError(ERR_INVALID_VAR,token);
		return NULL;
	}

	/* Make sure it doesn't already exist */
	if (getVariable(var_token->str))
	{
		basicError(ERR_DUPLICATE_DECL,token);
		return NULL;
	}

	/* Check for array size brackets */
	initValue(&arr_size);
	arr_size.val = 1;
	*is_array = 0;
	end = token->comma_loc ? token->comma_loc : token->line_end_loc;
	if (*pc < end)
	{
		br_token = &token_list[*pc+1];
		if (IS_OP_TYPE(br_token,OP_L_SQR_BRACKET))
		{
			*is_array = 1;
			if (!getIndex(
				*pc+2,
				br_token->u.close_bracket_loc-1,&arr_size,1))
			{
				return NULL;
			}
			*pc = br_token->u.close_bracket_loc + 1;
		}
		else ++*pc;
	}
	else ++*pc;

	var_token->var = allocateVariable(var_token->str,(int)arr_size.val);
	return var_token->var;
}




/*** Allocate the memory, set the fields and put in the list ***/
st_var *allocateVariable(char *name, int arr_size)
{
	st_var *var;
	int mem_size;
	int c;

	var = (st_var *)malloc(sizeof(st_var));
	assert(var);
	bzero(var,sizeof(st_var));

	var->name = name;
	mem_size = arr_size * sizeof(st_value);
	var->arr = (st_value *)malloc(mem_size);
	assert(var->arr);
	bzero(var->arr,mem_size);
	var->array_size = arr_size;

	c = name[0];
	if (first_var[c])
		last_var[c]->next = var;
	else
		first_var[c] = var;
	last_var[c] = var;

	return var;
}




/*** Get the value of the variable at the given position. Sets program counter
     and returns 1 or 0 ***/
int getVariableValue(int *pc, st_value *value)
{
	st_token *token;
	st_token *br_token;
	st_var *var;
	st_value index;
	st_dict *dict;
	int index_num;

	initValue(&index);

	token = &token_list[*pc];
	if (!token->var)
	{
		if (!(token->var = getVariableAndIndex(pc,&index)))
			return 0;
	}
	else
	{
		br_token = &token_list[*pc+1];
		if (IS_OP_TYPE(br_token,OP_L_SQR_BRACKET))
		{
			if (!getIndex(
				*pc+2,
				br_token->u.close_bracket_loc-1,&index,0))
			{
				return 0;
			}
			*pc = br_token->u.close_bracket_loc;
		}
	}
	var = token->var;

	/* Dictionary key index */
	if (index.strval)
	{
		dict = getDictionaryElementByKey(var,index.strval);
		clearValue(&index);
		if (!dict)
		{
			basicError(ERR_KEY_NOT_FOUND,token);
			return 0;
		}
		setValueByValue(value,&dict->val);
		
		++(*pc);
		return 1;
	}

	/* Numeric index */
	index_num = (int)index.val;
	if (index_num >= var->array_size)
	{
		basicError(ERR_ARR_INDEX_OOB,token);
		return 0;
	}

	/* Dynamic system variables - ie values change */
	if (var->sysvar_num)
	{
		assert(var->sysvar_num <= NUM_SYSTEM_VARS);

		/* If we have a function for the variable then call it 
		   otherwise just return variable value */
		switch(var->sysvar_num)
		{
		case SVAR_TIME:
			sysTime(value);
			break;

		case SVAR_UPTIME:
			sysUptime(value);
			break;

		case SVAR_KEY_START_NOTE:
			setValue(value,params.key_start_note,NULL);
			break;

		case SVAR_NOTE:
			setValue(value,shm->note,NULL);
			break;

		case SVAR_NOTE_NAME:
			setValue(value,0,getNoteName(shm->note,shm->note_scale));
			break;

		case SVAR_SCALE:
			setValue(value,shm->note_scale,NULL);
			break;

		case SVAR_SCALE_NAME:
			setValue(value,0,scale_name[shm->note_scale]);
			break;

		case SVAR_FREQ:
			setValue(value,shm->freq,NULL);
			break;

		case SVAR_SPAN:
			setValue(value,dft_freq_level[index_num],NULL);
			break;

		case SVAR_SPAN_MAX_INDEX:
			setValue(value,params.analyser_range-1,NULL);
			break;

		case SVAR_FILTER:
			setValue(value,shm->filter_val,NULL);
			break;

		case SVAR_EFFECTS_SEQ:
			setValue(value,0,effects_seq_str);
			break;

		case SVAR_WIN_MAPPED:
			setValue(value,win_mapped,NULL);
			break;

		case SVAR_WIN_WIDTH:
			setValue(value,win_width,NULL);
			break;

		case SVAR_WIN_HEIGHT:
			setValue(value,win_height,NULL);
			break;

		case SVAR_COM_EXEC_CNT:
			/* +1 to include the command we're in now */
			setValue(value,com_exec_cnt+1,NULL);
			break;


		case SVAR_SAMPLING_ENABLED:
			/* Sampling can be disabled after BASIC initialised
			   so can't hard code it in createSystemVariables() */
			setValue(value,do_sampling,NULL);
			break;

		default:
			setValueByValue(value,&var->arr[index_num]);
		}
		++(*pc);
		return 1;
	}

	setValueByValue(value,&var->arr[index_num]);
	++(*pc);
	return 1;
}




/*** Get the variable pointer and the index at the given positions ***/
st_var *getVariableAndIndex(int *pc, st_value *index)
{
	st_token *token;
	st_token *br_token;
	st_var *var;

	initValue(index);

	token = &token_list[*pc];
	if (token->var) var = token->var;
	else
	{
		if (!(var = getVariable(token->str)))
		{
			basicError(ERR_UNDEFINED_VAR,token);
			return NULL;
		}
		token->var = var;
	}

	br_token = &token_list[*pc+1];
	if (IS_OP_TYPE(br_token,OP_L_SQR_BRACKET))
	{
		if (!getIndex(*pc+2,br_token->u.close_bracket_loc-1,index,0))
			return NULL;

		*pc = br_token->u.close_bracket_loc;
	}

	return var;
}




/*** Use the linked list to find the variable since we only need to do it
     once ***/
st_var *getVariable(char *name)
{
	st_var *var;

	for(var=first_var[(int)name[0]];var;var=var->next)
		if (!strcmp(var->name,name)) return var;
	return NULL;
}




/*** Returns 1 or 0 ***/
int setVariable(
	st_var *var,
	st_value *index, int force, double num, char *str, st_token *token)
{
	int index_num;

	if (!force && var->sysvar_num)
	{
		basicError(ERR_READ_ONLY,token);
		return 0;
	}
	if (index)
	{
		if (index->strval) return setDictionaryValue(var,index,num,str);

		index_num = (int)index->val;
		if (index_num >= var->array_size)
		{
			basicError(ERR_ARR_INDEX_OOB,token);
			return 0;
		}
	}
	else index_num = 0;

	setValue(&var->arr[index_num],num,str);
	return 1;
}




/*** Returns 1 or 0 ***/
int setVariableByValue(
	st_var *var,
	st_value *index, int force, st_value *new_val, st_token *token)
{
	int index_num;

	if (!force && var->sysvar_num)
	{
		basicError(ERR_READ_ONLY,token);
		return 0;
	}
	if (index)
	{
		if (index->strval)
			return setDictionaryValue(var,index,new_val->val,new_val->strval);
		index_num = (int)index->val;
		if (index_num >= var->array_size)
		{
			basicError(ERR_ARR_INDEX_OOB,token);
			return 0;
		}
	}
	else index_num = 0;

	setValueByValue(&var->arr[index_num],new_val);
	return 1;
}




/*** Get the variable index value from the expression and do a sanity check.
     Returns 1 or 0 ***/
int getIndex(int pc, int end, st_value *index, int creating)
{
	st_value result;
	st_token *token = &token_list[pc];

	if (pc > end) goto ERROR;

	switch(evalExpression(pc,end,&result))
	{
	case NO_RESULT:
		clearValue(&result);
		return 0;

	case RESULT_STR:
		if (!creating)
		{
			setValueByValue(index,&result);
			clearValue(&result);
			return 1;
		}
		clearValue(&result);
		break;

	case RESULT_NUM:
		if (result.val < 0 || (creating && result.val < 1)) goto ERROR;
		setValue(index,result.val,result.strval);
		return 1;

	default:
		assert(0);
	}

	ERROR:
	basicError(creating ? ERR_INVALID_ARR_SIZE : ERR_INVALID_ARR_INDEX,token);
	return 0;
}




/*** Clear out a variable. Currently used for CLEAR command and during a 
     BASIC restart. Don't free "name" because its not allocated to the variable
     as its a token string or hard coded text for system variables ***/
void clearVariable(st_var *var)
{
	st_dict *dict;
	st_dict *next;
	int i;

	for(i=0;i < var->array_size;++i) clearValue(&(var->arr[i]));

	if (!var->dict_size) return;

	assert(var->dict_first);

	for(i=0;i < 256;++i)
	{
		for(dict=var->dict_first[i];dict;dict=next)
		{
			next = dict->next;
			free(dict->key);
			clearValue(&dict->val);
			free(dict);
		}
	}
	free(var->dict_first);
	free(var->dict_last);
	var->dict_first = NULL;
	var->dict_last = NULL;
	var->dict_size = 0;
}




/*** Delete all the variables ***/
void deleteVariables()
{
	st_var *var;
	st_var *var_next;
	int c;

	/* Go through all the variables - clear out the values then delete the
	   variable itself. */
	for(c=0;c < 256;++c)
	{
		for(var=first_var[c];var;var=var_next)
		{
			var_next = var->next;
			clearVariable(var);
			free(var);
		}
	}
	initVariableArrays();
}


/************************** Dictionary functions *****************************/

static int setDictionaryValue(
	st_var *var, st_value *key, double num, char *str)
{
	st_dict *dict;
	int bytes;
	int c;

	/* See if we already have it */
	if (!(dict = getDictionaryElementByKey(var,key->strval)))
	{
		/* Add it */
		dict = (st_dict *)malloc(sizeof(st_dict));
		assert(dict);
		dict->key = strdup(key->strval);
		dict->next = NULL;
		++var->dict_size;

		/* Allocate indexing level if not already done */
		if (!var->dict_first)
		{
			bytes = sizeof(st_dict *) * NUM_UCHARS;

			var->dict_first = (st_dict **)malloc(bytes);
			assert(var->dict_first);
			var->dict_last = (st_dict **)malloc(bytes);
			assert(var->dict_last);

			bzero(var->dict_first,bytes);
			bzero(var->dict_last,bytes);
		}

		/* Add to linked list */
		c = dict->key[0];
		if (var->dict_first[c])
		{
			dict->prev = var->dict_last[c];
			var->dict_last[c]->next = dict;
		}
		else
		{
			dict->prev = NULL;
			var->dict_first[c] = dict;
		}
		var->dict_last[c] = dict;
	
		initValue(&dict->val);
	}
	setValue(&dict->val,num,str);
	return 1;
}




st_dict *getDictionaryElementByKey(st_var *var, char *key)
{
	st_dict *dict;

	if (var->dict_first)
	{
		for(dict=var->dict_first[(int)key[0]];dict;dict=dict->next)
			if (!strcmp(dict->key,key)) return dict;
	}
	return NULL;
}




/*** Because of the index this won't often return the elements in the order
     they were added to the dictionary ***/
st_dict *getDictionaryElementByNumber(st_var *var, int pos)
{
	st_dict *dict;
	int c;
	int cnt;

	if (pos < 0 || !var->dict_first) return NULL;

	cnt = -1;
	for(c=0;c < NUM_UCHARS && cnt < pos;++c)
	{
		for(dict=var->dict_first[c];dict;dict=dict->next)
		{
			/* Do inside loop because don't want dict set to
			   dict->next before we exit */
			if (++cnt == pos) break;
		}
	}
	return dict;
}




int deleteDictionaryElement(st_var *var, char *key)
{
	st_dict *dict;
	int c;

	if (!(dict = getDictionaryElementByKey(var,key))) return 0;

	assert(var->dict_first);

	/* Remove from linked list */
	c = key[0];
	if (dict == var->dict_last[c])
		var->dict_last[c] = dict->prev;
	else
		dict->next->prev = dict->prev;

	if (dict == var->dict_first[c])
		var->dict_first[c] = dict->next;
	else
		dict->prev->next = dict->next;

	free(dict->key);
	clearValue(&dict->val);
	--var->dict_size;

	return 1;
}


/************************* System variable functions *************************/


void sysTime(st_value *value)
{
	setValue(value,getUsecTimeAsDouble(),NULL);
}




void sysUptime(st_value *value)
{
	setValue(value,getUsecTimeAsDouble() - boot_time,NULL);
}
