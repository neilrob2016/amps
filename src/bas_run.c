/*** Top level interface to run BASIC interpreter ***/

#include "globals.h"

void doRunSection(int sect);


/*** Called when a dial or button is updated or a note is played. The EVENT
     section is always called regardless of whether there is a dial/button/
     note section anyway ***/
void runEventSection(int sect, int ival, double dval, char *strval)
{
	if (!num_tokens || pause_program) return;

	setVariable(system_var[SVAR_EVENT_TYPE],NULL,1,0,section[sect].name,0);
	
	/* Set system variables that must be set with the ephemeral data 
	   passed to this function and can't be set in getVariableValue() */
	switch(sect)
	{
	case SECTION_DIAL:
		setVariable(system_var[SVAR_DIAL],NULL,1,0,button_name[ival],0);
		setVariable(system_var[SVAR_DIAL_VALUE],NULL,1,dval,strval,0);
		break;

	case SECTION_BUTTON:
		setVariable(system_var[SVAR_BUTTON],NULL,1,0,button_name[ival],0);
		setVariable(system_var[SVAR_BUTTON_VALUE],NULL,1,dval,NULL,0);
		setVariable(system_var[SVAR_BUTTON_VALUE_NAME],NULL,1,0,strval,0);
		break;

	case SECTION_FUNCKEY:
		setVariable(system_var[SVAR_FUNCTION_KEY],NULL,1,ival,NULL,0);
		break;

	case SECTION_EVENT:
		/* This section is ONLY called direct when the window is 
		   closed by X so set the $event_type variable accordingly */
		setVariable(system_var[SVAR_EVENT_TYPE],NULL,1,0,"XKILL",0);
		break;
		
	case SECTION_KEYSTART:
	case SECTION_PLAY:
	case SECTION_RELEASE:
	case SECTION_SCALE:
	case SECTION_FILTER:
	case SECTION_WINDOW:
		break;

	default:
		assert(0);
	}
	/* Prevents recursion. Could use parameters but this saves a lot 
	   of "if" statements inside doButtonAction(). However we do set
	   any appropriate system variables */
	if (call_basic)
	{
		if (sect != SECTION_EVENT) runSection(sect);

		/* Always run EVENT section */
		runSection(SECTION_EVENT);
	}
}




/*** Run the TIMER & MAIN sections ***/
void runNonEventSections()
{
	int s;
	for(s=SECTION_TIMER0;s < NUM_SECTIONS;++s) runSection(s);
}




/*** Run section directly ***/
void runSection(int sect)
{
	if (pause_program)
	{
		if (sect == SECTION_INIT) deferred_init = 1;
		return;
	}
	if (section[sect].runnable &&
	         (blocking_section == SECTION_INIT || sect == blocking_section))
	{
		do_messages = auto_messages;
		doRunSection(sect);
		do_messages = 1;
	}
}




/*** Run the actual code in the section ***/
void doRunSection(int sect)
{
	st_section *sec;
	st_token *token;
	st_value result;
	double diff;
	int pc;

	sec = &section[sect];
	setVariable(system_var[SVAR_SECTION],NULL,1,0,sec->name,0);

	if (IS_MAIN_SECTION(sect))
	{
		/* The main sections start from their previous program counter 
		   location. All other sections always start from their 
		   beginning */
		pc = sec->pc;

		CHECK_SLEEP:
		/* See if we're supposed to be sleeping */
		if (sec->sleep_until)
		{
			if (sec->sleep_until - getUsecTimeAsDouble() > 0)
				return;
			sec->sleep_until = 0;
		}
	}
	else 
	{
		/* See if timer section due to run */
		if (IS_TIMER_SECTION(sect))
		{
			if (sec->sleep_until)
			{
				diff = sec->sleep_until - getUsecTimeAsDouble();
				if (diff > 0) return;

				if (!do_timer_catchup && 
				    diff < -sec->timer_interval) diff = 0;
			}
			else diff = 0;
			sec->sleep_until = getUsecTimeAsDouble() + 
			                   sec->timer_interval + diff;
		}
		pc = sec->start_loc;
	}

	com_exec_cnt = 0;

	/* Execute commands until we hit the end of the section, the yield
	   count (unless its zero or not a MAIN section) or an error */
	while(pc < num_tokens && 
	      (!IS_MAIN_SECTION(sect) || com_exec_cnt < yield_after || !yield_after))
	{
		token = &token_list[pc];

		switch(token->token_type)
		{
		case TOK_SECTION:
		case TOK_PADDING:
			/* Only stop INIT and MAIN sections when they reach the
			   end of their code */
			if (sect == SECTION_INIT || IS_MAIN_SECTION(sect))
				stopSection(sect,0);
			return;

		case TOK_VAR:
			assert(token->next_line_loc);
			if ((pc = comLet(sect,pc,token)) == -1)
			{
				stopSection(sect,1);
				return;
			}
			++com_exec_cnt;
			break;

		case TOK_COM:
			/* If no argments with yield then yield control */
			if (token->token_num == COM_YIELD &&
			    token->line_end_loc == pc)
			{
				sec->pc = token->next_line_loc;
				return;
			}

			/* Call the command function */
			pc = (*command[token->token_num].func)(sect,pc,token);
			if (pc == -1)
			{
				stopSection(sect,1);
				return;
			}

			/* If we've restarted we're done here */
			if (token->token_num == COM_RESTART) return;

			++com_exec_cnt;

			/* If a MAIN section is trying to sleep then we can't
			   just do sleep() as it'll block. Need to exit this
			   loop and keep checking time */
			if (IS_MAIN_SECTION(sect) &&
			    token->token_num == COM_SLEEP)
			{
				sec->pc = pc;
				goto CHECK_SLEEP;
			}
			break;

		case TOK_LABEL:
			++pc;
			break;

		case TOK_OP:
			if (token->token_num == OP_COLON) 
			{
				++pc;
				continue;
			}
			/* Fall through */

		default:
			/* Try and evaluate since it could be a function such
			   as set or delkey that doesn't need to always assign
			   to something */
			if (evalExpression(pc,token->line_end_loc,&result) == NO_RESULT)
			{
				stopSection(sect,1);
				return;
			}
			clearValue(&result);
			pc = token->next_line_loc;
		}
	}
	sec->pc = pc;
}




void stopSection(int sect, int err)
{
	section[sect].runnable = 0;
	--runnable_sections;
	assert(runnable_sections >= 0);

	/* The INIT section exiting is a given, we don't need confirmation */
	if (sect != SECTION_INIT)
		dualMessage("BASIC: Section '%s' exited",section[sect].name);
	blocking_section = SECTION_INIT;

	/* If INIT failed with an error stop all other sections from running */
	if (sect == SECTION_INIT && err)
	{
		stopProgram();
		dualMessage("BASIC: *** Program has been terminated due to 'INIT' failure ***");
		return;
	}

	/* Print a message if all sections have stopped */
	if (!runnable_sections) dualMessage("BASIC: *** Program complete ***");
}




void pauseProgram(int on)
{
	pause_program = on;
	button[BUT_PAUSE_PROG].pressed = on;

	if (!num_tokens)
	{
		message(NO_PROG_LOADED);
		return;
	}
	message("Program %s",on ? "PAUSED" : "RUNNING");

	/* This will occur if we loaded a program while pause was on and INIT
	   didn't get to run. Run it now */
	if (!on && deferred_init)
	{
		runSection(SECTION_INIT);
		deferred_init = 0;
	}
}




/*** Set all sections to non runnable ***/
void stopProgram()
{
	int i;
	for(i=0;i < NUM_SECTIONS;++i) section[i].runnable = 0;
	runnable_sections = 0;
}
