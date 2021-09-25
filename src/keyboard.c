/*** Controls keyboard input whether via X or the Linux console ***/

#include "globals.h"

static char curr_note_key = 0;

static void processMultiByteKey(int len, char *buff);
static void dumpBasicSectionStatus();
static void downOctave();
static void upOctave();
static void decVolume();
static void incVolume();
static void holdNote();


/****************************** CONSOLE MODE ********************************/

/*** We've read from data in from the terminal, process it ***/
int processConsoleData(int *is_note_key)
{
	int len;
	int i;
	char buff[10];

	if ((len = read(STDIN,buff,10)) == -1) return 0;

	for(i=0;i < len;++i)
	{
		/* If character is an escape and we have any bytes left then 
		   assume its a multibyte keycode. eg from arrow keys or 
		   function keys */
		if (buff[i] == ESCAPE_KEY && i < len-1)
		{
			processMultiByteKey(len-i-1,buff+i+1);
			return 1;
		}
		if (evr_state == RECORDER_RECORD)
			evrAddKeyPressEvent(0,buff[i]);
		processKeyByAsciiValue(buff[i],is_note_key);
	}
	return 1;
}




/*** This does the arrow and function keys. However function key codes for
     F1 to F5 seem to be different for different systems so these arn't
     guaranteed to work all the time ***/
void processMultiByteKey(int len, char *buff)
{
	/* Console F1 to F5 is linux console */
	enum
	{
		/* 0 */
		ESC_UP_ARROW,
		ESC_DOWN_ARROW,
		ESC_RIGHT_ARROW,
		ESC_LEFT_ARROW,
		ESC_XTERM_F1,

		/* 5 */
		ESC_XTERM_F2,
		ESC_XTERM_F3,
		ESC_XTERM_F4,
		ESC_F5,
		ESC_F6,

		/* 10 */
		ESC_F7,
		ESC_F8,
		ESC_F9,
		ESC_F10,
		ESC_F11,

		/* 15 */
		ESC_F12,
		ESC_CONSOLE_F1,
		ESC_CONSOLE_F2,
		ESC_CONSOLE_F3,
		ESC_CONSOLE_F4,

		/* 20 */
		ESC_CONSOLE_F5,
		ESC_VT100_F5,

		NUM_ESC_SEQS
	};
	static struct st_esc_seq
	{
		int len;
		char *str;
	} seq[NUM_ESC_SEQS] =
	{
		/* 0 */
		{ 2, "[A" },
		{ 2, "[B" },
		{ 2, "[C" },
		{ 2, "[D" },
		{ 2, "OP" },

		/* 5 */
		{ 2, "OQ" },
		{ 2, "OR" },
		{ 2, "OS" },
		{ 4, "[15~" },
                { 4, "[17~" },

		/* 10 */
                { 4, "[18~" },
                { 4, "[19~" },
                { 4, "[20~" },
                { 4, "[21~" },
                { 4, "[23~" },

		/* 15 */
                { 4, "[24~" },
		{ 3, "[[A" },
                { 3, "[[B" },
                { 3, "[[C" },
                { 3, "[[D" },

		/* 20 */
                { 3, "[[E" },
                { 2, "[M" }
	};
	int i;
	for(i=0;i < NUM_ESC_SEQS;++i)
	{
		if (len != seq[i].len || memcmp(buff,seq[i].str,seq[i].len))
			continue;

		switch(i)
		{
		case ESC_UP_ARROW:
			upOctave();
			break;

		case ESC_DOWN_ARROW:
			downOctave();
			break;

		case ESC_LEFT_ARROW:
			setKeyStartNote(params.key_start_note - 1);
			break;

		case ESC_RIGHT_ARROW:
			setKeyStartNote(params.key_start_note + 1);
			break;

		case ESC_XTERM_F1:
		case ESC_XTERM_F2:
		case ESC_XTERM_F3:
		case ESC_XTERM_F4:
		case ESC_F5:
		case ESC_F6:
		case ESC_F7:
		case ESC_F8:
		case ESC_F9:
		case ESC_F10:
		case ESC_F11:
		case ESC_F12:
			runEventSection(SECTION_FUNCKEY,i - ESC_XTERM_F1 + 1,0,NULL);
			break;

		case ESC_CONSOLE_F1:
		case ESC_CONSOLE_F2:
		case ESC_CONSOLE_F3:
		case ESC_CONSOLE_F4:
		case ESC_CONSOLE_F5:
			runEventSection(SECTION_FUNCKEY,i - ESC_CONSOLE_F1 + 1,0,NULL);
			break;

		/* Why this is an odd one out I have no idea */
		case ESC_VT100_F5:
			runEventSection(SECTION_FUNCKEY,5,0,NULL);
			break;

		default:
			assert(0);
		}
		return;
	}
}



/****************************** KEY PRESSED *********************************/

/*** Keyboard key pressed in X - do something. Or not ***/
void keyPressed(XEvent *event)
{
	KeySym ksym;
	char key;

	XLookupString(&event->xkey,&key,1,&ksym,NULL);

	if (evr_state == RECORDER_RECORD) evrAddKeyPressEvent(ksym,key);

	processKeyPressed(ksym,key);
}




/*** Seperate from above because its called direct by evrPlayEvents() ***/
void processKeyPressed(KeySym ksym, char key)
{
	if (ksym == XK_Escape)
	{
		if (escape_cnt) quit(QUIT_KEYBOARD);
		reset(1);
		escape_cnt = PRESS_TIMEOUT_CNT;
		return;
	}

	if (disk.op != DISK_NO_OP)
	{
		addFilenameChar(ksym,key);
		return;
	}

	/* Any key switches off credits age unless its shift left in which case
	   show the ascii table */
	if (show_credits)
	{
		show_credits = (ksym == XK_Shift_L ? 2 : 0);
		return;
	}

	/* See if we can process by the character first */
	if (processKeyByAsciiValue(key,NULL)) return;

	/* Use the ksym where the key returns more than a simple ascii char */
	switch(ksym)
	{
	case XK_Up:
		upOctave();
		break;

	case XK_Down:
		downOctave();
		break;

	case XK_Left:
		setKeyStartNote(params.key_start_note - 1);
		break;

	case XK_Right:
		setKeyStartNote(params.key_start_note + 1);
		break;

	case XK_F1:
	case XK_F2:
	case XK_F3:
	case XK_F4:
	case XK_F5:
	case XK_F6:
	case XK_F7:
	case XK_F8:
	case XK_F9:
	case XK_F10:
	case XK_F11:
	case XK_F12:
		runEventSection(SECTION_FUNCKEY,(int)ksym - XK_F1 + 1,0,NULL);
		break;
	}
}




/*** Process by ascii value. Used by X and console mode. Avoids duplicating
     most of the switch in processKeyPressed() ***/
int processKeyByAsciiValue(int key, int *is_note_key)
{
	if (is_note_key) *is_note_key = 0;

	// Uppercase and lowercase do the same except for 's'
	switch(key)
	{
	case 'a':
	case 'A':
		setOrCycleARP(LEFT_MBUTTON,NULL,0);
		break;

	case 'b':
	case 'B':
		evrPlay();
		break;

	case 'c':
	case 'C':
		setOrCycleChord(LEFT_MBUTTON,NULL,0);
		break;

	case 'd':
	case 'D':
		downOctave();
		break;

	case 'e':
	case 'E':
		swapEffects();
		button[BUT_SWAP_EFFECTS].pressed = BUTTON_PRESS;
		break;

	case 'f':
	case 'F':
		setOrCycleFreqMode(LEFT_MBUTTON,NULL,0);
		break;

	case 'g':
	case 'G':
		resetEffectsSeq(1);
		break;

	case 'h':
	case 'H':
		holdNote();
		break;

	case 'i':
	case 'I':
		setFillWaveform(!params.fill_waveform);
		break;

	case 'j':
	case 'J':
		setOrCycleSubOsc(1,LEFT_MBUTTON,NULL,0);
		break;

	case 'k':
	case 'K':
		setOrCycleSubOsc(2,LEFT_MBUTTON,NULL,0);
		break;

	case 'l':
		button[BUT_LOAD_PATCH].pressed = BUTTON_PRESS;
		loadPatchMode();
		break;

	case 'L':
		button[BUT_LOAD_PROG].pressed = BUTTON_PRESS;
		loadProgramMode();
		break;

	case 'm':
	case 'M':
		setOrCycleRingModRange(LEFT_MBUTTON,NULL,0);
		break;

	case 'n':
	case 'N':
		evrLoop(!evr_flags.loop);
		break;

	case 'o':
	case 'O':
		evrEventsInSave(!evr_flags.events_in_save);
		break;

	case 'p':
	case 'P':
		setOrCyclePhasingMode(LEFT_MBUTTON,NULL,0);
		break;

	case 'r':
		setOrCycleResMode(LEFT_MBUTTON,NULL,0);
		break;

	case 'R':
		button[BUT_RANDOMISE].pressed = BUTTON_PRESS;
		randomiseSettings();
		runEventSection(SECTION_BUTTON,BUT_RANDOMISE,1,NULL);
		break;

	case 's':
		button[BUT_SAVE_PATCH].pressed = BUTTON_PRESS;
		savePatchMode();
		break;

	case 'S':
		saveProgramMode();
		break;

	case 't':
	case 'T':
		setOrCycleEffectsSeq(LEFT_MBUTTON,0);
		button[BUT_EFFECTS_TO_SWAP].pressed = BUTTON_PRESS;
		break;

	case 'u':
	case 'U':
		upOctave();
		break;

	case 'v':
	case 'V':
		evrRecord(0);
		break;

	case 'w':
	case 'W':
		setFreezeWaveform(!freeze_waveform);
		break;

	case 'x':
	case 'X':
		incVolume();
		break;

	case 'z':
	case 'Z':
		decVolume();
		break;

	case ' ':
		evrPause();
		button[BUT_EVR_PLAY_PAUSE].pressed = BUTTON_PRESS;
		break;

	case ',':
		evrPatchReset(!evr_flags.patch_reset);
		break;

	case '.':
		evrSetPatchToMain();
		button[BUT_EVR_PATCH_EQ_MAIN].pressed = BUTTON_PRESS;
		break;
		
	case '/':
		evrClear(1);
		button[BUT_EVR_CLEAR].pressed = BUTTON_PRESS;
		break;

	case '\t':
		cycleMainOsc();
		break;

	case '#':
		if (x_mode)
			show_section_status = !show_section_status;
		else
			dumpBasicSectionStatus();
		break;

	case '[':
		restartProgram();
		button[BUT_RESTART_PROG].pressed = BUTTON_PRESS;
		break;

	case ']':
		pauseProgram(!pause_program);
		break;

	case '?':
		show_credits = 1;
		break;

	case '\r':
	case '\n':
		if (shm->freq_mode != FREQ_CONT)
		{
			cycleScale(WHEEL_UP);
			if (!x_mode)
				printf("Scale = %s\n",scale_name[shm->note_scale]);
		}
		break;

	case ESCAPE_KEY:
		if (escape_cnt) quit(QUIT_KEYBOARD);
		reset(1);
		escape_cnt = PRESS_TIMEOUT_CNT;
		break;

	default:
		if (key == '-' || 
		    key == '=' ||
		    (key >= '0' && key <= '9'))
		{
			curr_note_key = key;
			if (is_note_key) *is_note_key = 1;
			setNoteByKey((char)key);
		}
		else return 0;
	}
	return 1;
}




/*** Dump the BASIC section status' to standard out ***/
void dumpBasicSectionStatus()
{
	st_section *sec;
	char *status;
	int i;

	puts("\nSection   Status    Line");
	puts("-------   ------    ----");

	for(i=0;i < NUM_SECTIONS;++i)
	{
		sec = &section[i];
		if (sec->runnable)
		{
			if (blocking_section != SECTION_INIT && 
			    i != blocking_section)
				status = "BLOCKED";
			else if (sec->sleep_until) status = "SLEEP";
			else status = "RUN";

			printf("%-8s  %-7s   %-5d\n",
				sec->name,status,token_list[sec->pc].file_linenum);
		}
		else printf("%-8s  STOP\n",sec->name);
	}
	putchar('\n');
}




void downOctave()
{
	setKeyStartNote(params.key_start_note - OCTAVE);
}




void upOctave()
{
	setKeyStartNote(params.key_start_note + OCTAVE);
}




void decVolume()
{
	if (button[BUT_VOLUME].angle > DIAL_ANGLE_MIN)
	{
		--button[BUT_VOLUME].angle;
		setFieldToDialAngle(&shm->volume,BUT_VOLUME);
		message("Volume = %d",shm->volume);
		runEventSection(SECTION_DIAL,BUT_VOLUME,shm->volume,NULL);
	}
}




void incVolume()
{
	if (button[BUT_VOLUME].angle < DIAL_ANGLE_MAX)
	{
		++button[BUT_VOLUME].angle;
		setFieldToDialAngle(&shm->volume,BUT_VOLUME);
		message("Volume = %d",shm->volume);
		runEventSection(SECTION_DIAL,BUT_VOLUME,shm->volume,NULL);
	}
}




void holdNote()
{
	if (params.hold_note) 
	{
		params.hold_note = 0;
		playOff(1);
	}
	else params.hold_note = 1;

        button[BUT_HOLD_NOTE].pressed = params.hold_note;
        message("Hold note = %s",onoff[params.hold_note]);
	runEventSection(
		SECTION_BUTTON,
		BUT_HOLD_NOTE,params.hold_note,onoff[params.hold_note]);
}


/****************************** KEY RELEASED *********************************/

/*** Unset note if we were playing one ***/
void keyReleased(XEvent *event)
{
	KeySym ksym;
	char key;

	/* Don't play a note if we're entering a filename */
	if (disk.op != DISK_NO_OP) return;

	XLookupString(&event->xkey,&key,1,&ksym,NULL);

	if (evr_state == RECORDER_RECORD) evrAddKeyReleaseEvent(ksym,key);

	processKeyReleased(key);
}




/*** Switch note play off if we've just released the key we're currently
     pressing. This is done so if the user is pressing more than 1 key we 
     only send a release for the latest one pressed so play doesn't stop
     when releasing old notes ***/
void processKeyReleased(char key)
{
	if (!params.hold_note && key && key == curr_note_key) playOff(1);
}


/*********************************** MISC ************************************/

void resetKeyboard()
{
	if (x_mode)
	{
		/* Switch auto repeat back on */
		XAutoRepeatOn(display);
		XFlush(display);
	}
	/* Set keyboard back to what it was */
	else resetConsole();
}




void resetConsole()
{
	tcsetattr(0,TCSANOW,&saved_tio);
}
