/*** Startup BASIC interpreter and reset it ***/

#include "globals.h"

static void commonInit();


/*** Load basic program specified then call INIT section. Delete any current
     program in memory first ***/
void loadProgram(int startup)
{
	if (!basic_file) return;

	dualMessage("BASIC: Loading program '%s'...",basic_file);

	/* Clear down any previous program */
	deleteProgram(1);

	if (!tokenise(basic_file))
	{
		/* Cleardown if program load failed */
		deleteProgram(1);

		if (startup)
		{
			if (!x_mode) resetConsole();
			exit(1);
		}
		return;
	}

	commonInit();
	setTitleBar(NULL);
}




/*** Delete any program currently in memory ***/
void deleteProgram(int loading)
{
	st_token *token;
	st_label *label;
	st_label *next_label;
	int i;

	if (!num_tokens)
	{
		if (!loading) message(NO_PROG_LOADED);
		return;
	}
	if (!loading) dualMessage("BASIC: *** Program deleted ***");

	/* We've got a program loaded so clear it out */
	for(i=0;i < num_tokens;++i)
	{
		token = &token_list[i];
		if (token->token_type == TOK_PADDING) break;
		free(token->str);

		/* if_block and loop_block are in a union so can use
		   either to free the memory */
		if (token->u.if_block && token->token_type == TOK_COM)
		{
			switch(token->token_num)
			{
			case COM_IF:
			case COM_WHILE:
			case COM_DO:
			case COM_FOR:
				free(token->u.if_block);
				break;
			}
		}
	}
	free(token_list);

	for(i=0;i < NUM_SECTIONS;++i)
	{
		/* Zero everything except the name at the start of the 
		   structure */
		bzero(
			((char *)&section[i])+sizeof(char *),
			sizeof(st_section) - sizeof(char *));
	}

	for(label=first_label;label;label=next_label)
	{
		next_label = label->next;
		free(label);
	}
	deleteVariables();
	initBasic();
}




void initBasic()
{
	num_tokens = 0;
	num_tokens_alloc = 0;
	runnable_sections = 0;
	restart_cnt = 0;
	deferred_init = 0;
	token_list = NULL;
	first_label = last_label = NULL;
}




/*** Called when restart button pressed. Reset program counters, delete
     variables, remove variable pointers from tokens then call INIT section
     again  ***/
void restartProgram()
{
	st_section *sec;
	int i;

	if (!num_tokens)
	{
		message(NO_PROG_LOADED);
		return;
	}
	message("Program restarting...");
	runnable_sections = 0;
	++restart_cnt;

	pause_program = 0;
	button[BUT_PAUSE_PROG].pressed = 0;

	/* Reset section parameters */
	for(i=0;i < NUM_SECTIONS;++i)
	{
		sec = &section[i];
		if (!sec->start_loc) continue;

		sec->pc = sec->start_loc;
		sec->runnable = 1;
		sec->read_loc = 0;
		sec->restore_loc = 0;
		sec->returns_alloc = 0;
		sec->num_returns = 0;
		sec->sleep_until = 0;
		FREE(sec->return_loc);

		++runnable_sections;
	}

	for(i=0;i < num_tokens;++i)
	{
		token_list[i].var = NULL;
		token_list[i].checked = 0;
	}

	deleteVariables();
	commonInit();

	/* All buttons have been reset but we want to see the click */
	button[BUT_RESTART_PROG].pressed = BUTTON_PRESS;
}




/*** Common initialisation actions ***/
void commonInit()
{
	auto_messages = 0;
	io_messages = 0;
	call_basic = 1;
	yield_after = YIELD_AFTER;
	com_exec_cnt = 0;
	blocking_section = SECTION_INIT; 
	do_timer_catchup = 0;
	
	initVariableArrays();
	createSystemVariables();

	if (section[SECTION_INIT].start_loc) runSection(SECTION_INIT);
}
