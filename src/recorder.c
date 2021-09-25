/*** Events recorder. Records and plays back X events to create a very simple
     sequencer. However editing is not currently supported.
 ***/

#include "globals.h"

#define EVR_ALLOC           100
#define USEC_MOD            1000000000

#define PRINTING_CHAR(C) (C < 32 ? '?' : C)

enum en_event_type
{
	EVR_TYPE_MOUSE_PRESS,
	EVR_TYPE_MOUSE_RELEASE,
	EVR_TYPE_MOTION,
	EVR_TYPE_KEY_PRESS,
	EVR_TYPE_KEY_RELEASE
};

static u_int evr_start_time;
static u_int pause_time;
static int evr_events_allocated;

static void initRecording();
static u_int getTimeOffset(u_int tm);
static void addPauseTimeToStartTime();
static void initPlayback();


/*************************** INTERFACE FUNCTIONS ****************************/

/*** Initialise everything for a fresh start ***/
void evrInit()
{
	bzero(&evr_flags,sizeof(evr_flags));
	evr_flags.patch_reset = 1;
	evrClear(0);
}




void evrClear(int press)
{
	evr_state = RECORDER_STOPPED;

	if (evr_events)
	{
		free(evr_events);
		evr_events = NULL;
		evr_events_allocated = 0;
	}
	evr_events_cnt = 0;
	evr_next_play_event = 0;
	bzero(&evr_params,sizeof(evr_params));
	bzero(&evr_sharmem,sizeof(evr_sharmem));

	/* Only do if key/gui button press */
	if (press)
	{
		message("Events recorder cleared");
		runEventSection(SECTION_BUTTON,BUT_EVR_CLEAR,1,NULL);
	}
}




void evrStop()
{
	button[BUT_EVR_RECORD].pressed = 0;
	button[BUT_EVR_PLAY_PAUSE].pressed = 0;
	evr_state = RECORDER_STOPPED;
	pause_time = getUsecTime();
	first_mouse_button = 0;
	playOff(0);
}




/*** Flip recording on or off. 'on' = 0 if command from button or keyboard else
     its -1 for BASIC off, 1 for BASIC on ***/
void evrRecord(int on)
{
	switch(evr_state)
	{
	case RECORDER_STOPPED:
		if (on > -1) initRecording();
		break;

	case RECORDER_RECORD:
		if (on < 1)
		{
			/* Ignore last event as it will be the click on the 
			   record button or key. */
			if (evr_events_cnt > 0) --evr_events_cnt;
			evrStop();
		}
		break;

	case RECORDER_PLAY:
	case RECORDER_PLAY_PAUSED:
		return;

	default:
		assert(0);
	}
	message("Events recorder = %s",evr_state_name[evr_state]);
	runEventSection(
		SECTION_BUTTON,
		BUT_EVR_RECORD,evr_state,evr_state_name[evr_state]);
}




/*** Flip play on or off. If switched off it will restart from the beginning.
     To pause the pause button must be used. ***/
void evrPlay()
{
	switch(evr_state)
	{
	case RECORDER_STOPPED:
		initPlayback();
		break;

	case RECORDER_PLAY:
		evrStop();
		break;

	case RECORDER_RECORD:
	case RECORDER_PLAY_PAUSED:
		break;

	default:
		assert(0);
	}
	message("Events recorder = %s",evr_state_name[evr_state]);
	runEventSection(
		SECTION_BUTTON,BUT_EVR_PLAY,1,evr_state_name[evr_state]);
}




/*** Flip pause mode on or off ***/
void evrPause()
{
	if (evr_state == RECORDER_PLAY)
	{
		evr_state = RECORDER_PLAY_PAUSED;
		button[BUT_EVR_PLAY_PAUSE].pressed = BUTTON_PRESS;
		pause_time = getUsecTime();
	}
	else if (evr_state == RECORDER_PLAY_PAUSED)
	{
		evr_state = RECORDER_PLAY;
		addPauseTimeToStartTime();
	}
	message("Events recorder = %s",evr_state_name[evr_state]);
	runEventSection(
		SECTION_BUTTON,BUT_EVR_PLAY_PAUSE,1,evr_state_name[evr_state]);
}




/*** Loop play on/off ***/
void evrLoop(u_char val)
{
	evr_flags.loop = val;
	button[BUT_EVR_LOOP].pressed = evr_flags.loop;

	message("Events playback looping = %s",onoff[evr_flags.loop]);
	runEventSection(
		SECTION_BUTTON,
		BUT_EVR_LOOP,evr_flags.loop,onoff[evr_flags.loop]);
}




/*** Sets flag that decides whether to include the events in a save or just
     save the patch ***/
void evrEventsInSave(u_char val)
{
	evr_flags.events_in_save = val;
	button[BUT_EVR_EVENTS_IN_SAVE].pressed = evr_flags.events_in_save;

	message("Events in save = %s",onoff[evr_flags.events_in_save]);
	runEventSection(
		SECTION_BUTTON,
		BUT_EVR_EVENTS_IN_SAVE,
		evr_flags.events_in_save,onoff[evr_flags.events_in_save]);
}




/*** Set flag that causes the patch to be reset to the events patch at the
     start of playing events ***/
void evrPatchReset(u_char val)
{
	evr_flags.patch_reset = val;
	button[BUT_EVR_PATCH_RESET].pressed = evr_flags.patch_reset;

	message("Events patch reset = %s",onoff[evr_flags.patch_reset]);
	runEventSection(
		SECTION_BUTTON,
		BUT_EVR_PATCH_RESET,
		evr_flags.patch_reset,onoff[evr_flags.patch_reset]);
}




/*** Sets flag that causes the current events patch (evr_params &
     evr_sharmem) to be replaced with the current params and shared
     memory data so it can be saved as events with a different patch ***/
void evrSetPatchToMain()
{
	/* Stop child process so we get a clean copy - not some fields halfway
	   through an update */
	pauseSoundProcess();
	memcpy(&evr_params,&params,sizeof(params));
	memcpy(&evr_sharmem,shm,sizeof(evr_sharmem));
	restartSoundProcess();
	message("Events patch set to main patch");
	runEventSection(SECTION_BUTTON,BUT_EVR_PATCH_EQ_MAIN,1,NULL);
}


/******************************** RECORDING *********************************/


/*** Set everything up to start recording but we still wait for first mouse
     or keypress event until we do it ***/
void initRecording()
{
	button[BUT_EVR_PLAY_PAUSE].pressed = 0;
	button[BUT_EVR_PLAY].pressed = 0;
	button[BUT_EVR_RECORD].pressed = BUTTON_PRESS;

	evr_state = RECORDER_RECORD;

	/* If we already have some events stored add time paused so the new 
	   time offsets don't produce a gap between the last event recorded 
	   and the new lot of events */
	if (evr_events_cnt) addPauseTimeToStartTime();
	else
	{
		/* New recording so save patch */
		pauseSoundProcess();
		memcpy(&evr_params,&params,sizeof(evr_params));
		memcpy(&evr_sharmem,shm,sizeof(evr_sharmem));
		restartSoundProcess();
	}
}




/*** Allocate/find then zero a new event structure ***/
struct st_recorder_event *evrCreateNewEvent()
{
	struct st_recorder_event *event;
	struct st_recorder_event *tmp;
	
	if (evr_events_cnt < evr_events_allocated)
		event = &evr_events[evr_events_cnt];
	else
	{
		evr_events_allocated += EVR_ALLOC;
		if (!(tmp = realloc(
			evr_events,
			evr_events_allocated * sizeof(struct st_recorder_event))))
		{
			message("WARNING: realloc(): %s",strerror(errno));
			evr_events_allocated -= EVR_ALLOC;
			return NULL;
		}
		evr_events = tmp;
		event = &evr_events[evr_events_cnt];
	}
	bzero(event,sizeof(struct st_recorder_event));
	++evr_events_cnt;
	return event;
}




/*** This gets the offset from the time to now. This needs to deal with the 
     value of getUsecTime() wrapping ***/
u_int getTimeOffset(u_int tm)
{
	u_int now = getUsecTime();
	return now < tm ? USEC_MOD - tm + now : now - tm;
}




void addPauseTimeToStartTime()
{
	evr_start_time = (evr_start_time + getTimeOffset(pause_time)) % USEC_MOD;
}




void evrAddMotionEvent(int mouse_button, short x, short y)
{
	struct st_recorder_event *event;

	/* Can only start record motion events if we've already had a mouse
	   or key press */
	if (evr_events_cnt && (event = evrCreateNewEvent()))
	{
		event->time_offset = getTimeOffset(evr_start_time);
		event->type = EVR_TYPE_MOTION;
		event->x = (short)((double)x / g_x_scale);
		event->y = (short)((double)y / g_y_scale);
		event->key_mb = (char)mouse_button;

		if (events_debug)
		{
			printf("RECORD EVENT %u: Mouse motion.  X = %d, Y = %d\n",
				evr_events_cnt,
				event->x,
				event->y);
		}
	}
}




void evrAddMousePressEvent(int mouse_button, short x, short y)
{
	struct st_recorder_event *event;

	/* If we're just starting record the time */
	if (!evr_events_cnt) evr_start_time = getUsecTime();

	if ((event = evrCreateNewEvent()))
	{
		event->time_offset = getTimeOffset(evr_start_time);
		event->type = EVR_TYPE_MOUSE_PRESS;
		event->x = (short)((double)x / g_x_scale);
		event->y = (short)((double)y / g_y_scale);
		event->key_mb = (char)mouse_button;

		if (events_debug)
		{
			printf("RECORD EVENT %u: Mouse press.   Button = %d, X = %d, Y = %d\n",
				evr_events_cnt,
				event->key_mb,
				event->x,event->y);
		}
	}
}




void evrAddMouseReleaseEvent(int mouse_button)
{
	struct st_recorder_event *event;
	if (evr_events_cnt && (event = evrCreateNewEvent()))
	{
		event->time_offset = getTimeOffset(evr_start_time);
		event->type = EVR_TYPE_MOUSE_RELEASE;
		event->key_mb = (char)mouse_button;

		if (events_debug)
			printf("RECORD EVENT %u: Mouse release. Button = %d\n",evr_events_cnt,event->key_mb);
	}
}




void evrAddKeyPressEvent(KeySym ksym, char key)
{
	struct st_recorder_event *event;

	if (!evr_events_cnt) evr_start_time = getUsecTime();

	if ((event = evrCreateNewEvent()))
	{
		event->time_offset = getTimeOffset(evr_start_time);
		event->type = EVR_TYPE_KEY_PRESS;

		/* KeySym is 64 bit long on 64 bit systems. Hopefully we don't
		   need all 64 bits. Need same word length for saving */
		event->ksym = (uint32_t)ksym;
		event->key_mb = key;

		if (events_debug)
		{
			printf("RECORD EVENT %u: Key press.   Keysym = %d, key = '%c' (%d)\n",
				evr_events_cnt,
				event->ksym,
				PRINTING_CHAR(event->key_mb),
				event->key_mb);
		}
	}
}




void evrAddKeyReleaseEvent(KeySym ksym, char key)
{
	struct st_recorder_event *event;
	if (evr_events_cnt && (event = evrCreateNewEvent()))
	{
		event->time_offset = getTimeOffset(evr_start_time);
		event->type = EVR_TYPE_KEY_RELEASE;
		event->ksym = (uint32_t)ksym;
		event->key_mb = key;

		if (events_debug)
		{
			printf("RECORD EVENT %u: Key release. Keysym = %d, key = '%c' (%d)\n",
				evr_events_cnt,
				event->ksym,
				PRINTING_CHAR(event->key_mb),
				event->key_mb);
		}
	}
}


/******************************** PLAYBACK *********************************/

/*** Initialise everything for the start of play ***/
void initPlayback()
{
	if (!evr_events_cnt) return;

	ignore_mouse_release = 1;
	evr_state = RECORDER_PLAY;
	evr_start_time = getUsecTime();
	evr_next_play_event = 0;

	if (evr_flags.patch_reset)
	{
		pauseSoundProcess();
		memcpy(&params,&evr_params,sizeof(evr_params));
		memcpy(shm,&evr_sharmem,sizeof(evr_sharmem));
		normalisePatch(0);
		restartSoundProcess();
	}

	button[BUT_EVR_PLAY_PAUSE].pressed = 0;
	button[BUT_EVR_RECORD].pressed = 0;
	button[BUT_EVR_PLAY].pressed = BUTTON_PRESS;
}




void evrPlayEvents(u_int now)
{
	struct st_recorder_event *event;
	short x;
	short y;

	while(evr_next_play_event < evr_events_cnt &&
	      evr_events[evr_next_play_event].time_offset < getTimeOffset(evr_start_time))
	{
		event = &evr_events[evr_next_play_event++];

		switch(event->type)
		{
		case EVR_TYPE_MOUSE_PRESS:
			x = (short)((double)event->x * g_x_scale);
			y = (short)((double)event->y * g_y_scale);
			if (events_debug)
			{
				printf("PLAY EVENT %u: Mouse press.   Button = %d, X = %d, Y = %d\n",
					evr_next_play_event,
					event->key_mb,
					x,y);
			}
			mousePressed(event->key_mb,x,y);
			break;

		case EVR_TYPE_MOUSE_RELEASE:
			if (events_debug)
				printf("PLAY EVENT %u: Mouse release. Button = %d\n",evr_next_play_event,event->key_mb);
			mouseReleased(event->key_mb);
			break;

		case EVR_TYPE_MOTION:
			x = (short)((double)event->x * g_x_scale);
			y = (short)((double)event->y * g_y_scale);
			if (events_debug)
				printf("PLAY EVENT %u: Mouse motion.  X = %d, Y = %d\n",evr_next_play_event,x,y);
			mouseData(event->key_mb,x,y,1);
			break;

		case EVR_TYPE_KEY_PRESS:
			if (events_debug)
			{
				printf("PLAY EVENT %u: Key press.   Keysym = %d, key = '%c' (%d)\n",
					evr_next_play_event,
					event->ksym,
					PRINTING_CHAR(event->key_mb),
					event->key_mb);
			}

			if (x_mode)
				processKeyPressed((KeySym)event->ksym,event->key_mb);
			else
				processKeyByAsciiValue(event->key_mb,NULL);
			break;

		case EVR_TYPE_KEY_RELEASE:
			if (events_debug)
			{
				printf("PLAY EVENT %u: Key release. Keysym = %d, key = '%c' (%d)\n",
					evr_next_play_event,
					event->ksym,
					PRINTING_CHAR(event->key_mb),
					event->key_mb);
			}

			if (x_mode) processKeyReleased(event->key_mb);
			break;

		default:
			assert(0);
		}
		if (!x_mode)
		{
			printf("Event %d/%d\r",evr_next_play_event,evr_events_cnt);
			fflush(stdout);
		}

	}
	if (evr_next_play_event == evr_events_cnt)
	{
		if (evr_flags.loop)
			initPlayback();
		else
			evrStop();
	}
}
