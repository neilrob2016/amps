/*** Anything that doesn't fit anywhere else goes here ***/

#include "globals.h"


/*** Get the current time down to the microsecond. Value wraps once every
     1000 seconds. Goes from 0 -> 999999999 (1 billion - 1) ***/
u_int getUsecTime()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (u_int)(tv.tv_sec % 1000) * 1000000 + (u_int)tv.tv_usec;
}




/*** Gets time as a double. This wraps ... sometime in the future ***/
double getUsecTimeAsDouble()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
}




/*** Increment double angles. 'add' should only ever be positive but just in
     case it accounts for negatives. ***/
void incAngle(double *ang, double add)
{
	double res;
	long dec;

	res = *ang + add;

	if (res >= 360)
	{
		dec = (long)res;
		res = (double)(res - dec) + (dec % 360);
	}
	else if (res < 0) 
	{
		dec = (long)fabs(res);
		res = 360.0 - ((-res - dec) + (dec % 360));
	}
	*ang = res;
}




/*** Return the length in pixels taking account of scales ***/
double getTextPixelLen(int len, double scale)
{
	return (double)len * scale * (CHAR_SIZE + CHAR_GAP) - CHAR_GAP;
}




void setTitleBar(char *str)
{
	XTextProperty title;
	char *sound_system;

	if (!str)
	{
#if SOUND==ALSA
		sound_system = "ALSA";
#elif SOUND==OPENSOUND
		sound_system = "OpenSound";
#elif SOUND==OSX
		sound_system = "OSX";
#elif SOUND==NO_SOUND
		sound_system = "No sound";
#else
		assert(0);
#endif
		snprintf(title_str,TITLE_MAX_LEN+1,"AMPS v%s (%s) : %s, %s",
			VERSION,sound_system,
			patch_file ? patch_file : "<no patch>",
			basic_file ? basic_file : "<no program>");
	}
	else
	{
		strncpy(title_str,str,TITLE_MAX_LEN);
		title_str[TITLE_MAX_LEN] = 0;
	}

	if (x_mode)
	{
		/* Have to assign to pointer else next func crashes */
		str = title_str;
		XStringListToTextProperty(&str,1,&title);
		XSetWMProperties(display,win,&title,NULL,NULL,0,NULL,NULL,NULL);
	}
}




/*** Freeze the current waveform on screen ***/
void setFreezeWaveform(int frz)
{
	freeze_waveform = frz;
	if (freeze_waveform)
	{
		memcpy(freeze_sndbuff,sndbuff,sizeof(freeze_sndbuff));
		message("Waveform FROZEN");
	}
	else message("Waveform THAWED");
}




void setFillWaveform(int fill)
{
	params.fill_waveform = fill;
	message("Waveform %s",fill ? "FILLED" : "WIREFRAME");
}




void reset(int reset_evr)
{
	message("Resetting...");
	resetSharedMemory();
	paramsReset(1,reset_evr);
	if (x_mode) windowResized();

	/* Because all the buttons have been reset and we want to see click */
	button[BUT_RESET].pressed = BUTTON_PRESS;
	first_mouse_button = 0;

	patch_file = NULL;
	setTitleBar(NULL);

	runEventSection(SECTION_BUTTON,BUT_RESET,1,"");
}




void quit(int reason)
{
	char *rstr[NUM_QUIT_REASONS] =
	{
		"GUI",
		"keyboard",
		"BASIC",
		"signal",
		"XKILL"
	};

	/* We don't call Basic if it killed itself or if a signal killed the
	   process. The latter because we could be running Basic at the time
	   the signal was received and we'd end up with re-entrant issues */
	switch(reason)
	{
	case QUIT_GUI:
	case QUIT_KEYBOARD:
		runEventSection(SECTION_BUTTON,BUT_QUIT,1,"");
		break;

	case QUIT_XKILL:
		runEventSection(SECTION_EVENT,0,0,NULL);
		break;

	case QUIT_BASIC:
	case QUIT_SIGNAL:
		break;

	default:
		assert(0);
	}

	printf("*** AMPS terminated by %s ****\n",rstr[reason]);
	resetKeyboard();
	hdestroy();
	exit(0);
}
