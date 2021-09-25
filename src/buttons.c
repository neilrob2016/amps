/*** Button creation and control code. This code is a mess and is on its
     way to becoming sphagetti code. Needs a tidy up but I don't have the 
     time. ***/

#include "globals.h"

static void doButtonAction(
	int mouse_button, int but, double angle, int bas_on);


/* Has to be a macro because you can't pass the address of a bitfield to a
   function */
#define FLIP(BIT,BUT) \
	{ \
		BIT = !BIT; \
		button[BUT].pressed = BIT; \
	}

#define ON   1
#define OFF -1
#define ON_OFF() (bas_on == ON ? 1 : 0)

/******************************* FUNCTIONS *********************************/

void setupButtons()
{
	ENTRY e;
	double x;
	int i;
	int col;

	bzero(button,sizeof(button));

	/* Add name to map and set colours etc */
	for(i=0,x=0;i < NUM_BUTTONS;++i)
	{
		assert((e.key = strdup(button_name[i])));
		e.data = (void *)(long)i;
		assert(hsearch(e,ENTER));

		/* There's no particular scheme to these colours, just grouped
		   by functionality and looks clear */
		switch(i)
		{
		case BUT_SINE_FM:
		case BUT_SINE:
		case BUT_SQUARE:
		case BUT_TRIANGLE:
		case BUT_SAWTOOTH:
		case BUT_AAH:
		case BUT_OOH:
		case BUT_NOISE:
		case BUT_VIB_SWEEP:
		case BUT_VIB_LFO:
		case BUT_ALIASING:
			col = LIGHT_BLUE;
			break;

		case BUT_SAMPLE:
		case BUT_EVR_PLAY_PAUSE:
		case BUT_HIGHPASS_FILTER:
		case BUT_PAUSE_PROG:
		case BUT_GLIDE_DISTANCE:
		case BUT_GLIDE_VELOCITY:
		case BUT_SAMPLE_MOD_DEC:
		case BUT_SAMPLE_MOD_INC:
		case BUT_FILTER_SWEEP:
		case BUT_FILTER_LFO:
			col = ORANGE;
			break;

		case BUT_EVR_RECORD:
		case BUT_BUFFER_RESET:
		case BUT_DEL_PROG:
		case BUT_SINE_CUTOFF:
		case BUT_SINE_CUTOFF_LFO:
		case BUT_DISTORTION:
		case BUT_VOLUME:
		case BUT_QUIT:
			col = RED;
			break;

		case BUT_EVR_PLAY:
		case BUT_RES_MODE:
		case BUT_RES_LEVEL:
		case BUT_RES_FREQ:
		case BUT_RES_DAMPING:
		case BUT_SAVE_PATCH:
		case BUT_SAVE_PROG:
			col = GREEN;
			break;

		case BUT_RANDOMISE:
		case BUT_ARP_SEQ:
		case BUT_ARP_SPACING_DEC:
		case BUT_ARP_SPACING_INC:
		case BUT_ARP_DELAY_DEC:
		case BUT_ARP_DELAY_INC:
			col = LIGHT_GREEN;
			break;

		case BUT_SUB1_SOUND:
		case BUT_SUB1_OFFSET:
		case BUT_SUB1_NOTE_OFFSET:
		case BUT_SUB1_VOL:
		case BUT_SQUARE_WIDTH:
		case BUT_SQUARE_WIDTH_LFO:
		case BUT_ATTACK:
		case BUT_DECAY:
			col = PURPLE;
			break;

		case BUT_EVR_LOOP:
		case BUT_EVR_EVENTS_IN_SAVE:
		case BUT_EVR_PATCH_RESET:
		case BUT_SPECTRUM_ANALYSER:
		case BUT_PHASING_MODE:
		case BUT_PHASING_OFFSET:
		case BUT_PHASING_SWEEP:
		case BUT_PHASING_LFO:
		case BUT_PHASER_FREQ_SEP:
		case BUT_PHASER_LOW_OFF_MULT:
			col = BLUE;
			break;

		case BUT_EVR_PATCH_EQ_MAIN:
		case BUT_SUB2_SOUND:
		case BUT_SUB2_OFFSET:
		case BUT_SUB2_NOTE_OFFSET:
		case BUT_SUB2_VOL:
		case BUT_ANALYSER_RANGE:
		case BUT_COMPRESS_START:
		case BUT_COMPRESS_EXP:
			col = MAUVE;
			break;

		case BUT_HOLD_NOTE:
		case BUT_EFFECTS_TO_SWAP:
		case BUT_SWAP_EFFECTS:
		case BUT_RESET_EFFECTS_SEQ:
			col = PINKY_RED;
			break;

		case BUT_EVR_CLEAR:
		case BUT_CHORD:
		case BUT_FM_HARM_OFFSET:
		case BUT_FM_MULT1:
		case BUT_FM_MULT2:
		case BUT_FM_OFFSET:
		case BUT_FM_VOL1:
		case BUT_FM_VOL2:
		case BUT_FM_WIERD:
			col = MEDIUM_YELLOW;
			break;
 
		case BUT_RESTART_PROG:
		case BUT_REFLECT_LEVEL:
		case BUT_REFLECT_SMOOTHING:
		case BUT_SAW_FLATTEN:
		case BUT_SAW_FLATTEN_LFO:
		case BUT_RESET:
			col = YELLOW;
			break;

		case BUT_ECHO_CLEAR:
		case BUT_ECHO_INVERT:
		case BUT_ECHO_LEN:
		case BUT_ECHO_DECAY:
		case BUT_ECHO_FILTER:
		case BUT_ECHO_STRETCH:
		case BUT_ECHO_HIGHPASS_FILTER:
		case BUT_LOAD_PATCH:
		case BUT_LOAD_PROG:
			col = TURQUOISE;
			break;

		case BUT_SUBS_FOLLOW:
		case BUT_FREQ_MODE:
		case BUT_MAX_FREQ:
		case BUT_RING_RANGE:
		case BUT_RING_MODE:
		case BUT_RING_LEVEL:
		case BUT_RING_FREQ:
			col = KHAKI;
			break;

		default:
			assert(0);
		}
		button[i].colour = col;
		button[i].x = (short)x;

		// Note to self - tidy this ugly code up
		if (i <= TOP_BUTTONS_END)
		{
			if (i == TOP_BUTTONS_END)
				x = 0;
			else
				x += TOP_BUTTON_WIDTH;

			button[i].y = 0;
			button[i].width = (short)TOP_BUTTON_WIDTH;
		}
		else if (i <= BOT_BUTTONS1_END)
		{
			if (i == BOT_BUTTONS1_END)
				x = 0;
			else
				x += BOT_BUTTON_WIDTH1;
			button[i].y = BOT_Y1;
			button[i].width = (short)BOT_BUTTON_WIDTH1;
		}
		else if (i <= BOT_BUTTONS2_END)
		{
			if (i == BOT_BUTTONS2_END)
				x = 0;
			else
				x += BOT_BUTTON_WIDTH2;
			button[i].y = BOT_Y2;
			button[i].width = (short)BOT_BUTTON_WIDTH2;
		}
		else if (i <= BOT_BUTTONS3_END)
		{
			if (i == BOT_BUTTONS3_END)
				x = 0;
			else
				x += BOT_BUTTON_WIDTH3;
			button[i].y = BOT_Y3;
			button[i].width = (short)BOT_BUTTON_WIDTH3;
		}
		else if (i <= BOT_BUTTONS4_END)
		{
			if (i == BOT_BUTTONS4_END)
				x = 0;
			else
				x += BOT_BUTTON_WIDTH4;
			button[i].y = BOT_Y4;
			button[i].width = (short)BOT_BUTTON_WIDTH4;
		}
		else
		{
			if (i == BOT_BUTTONS5_END)
				x = 0;
			else
				x += BOT_BUTTON_WIDTH5;
			button[i].y = BOT_Y5;
			button[i].width = (short)BOT_BUTTON_WIDTH5;
		}
		button[i].mid_x = button[i].x + button[i].width / 2;
		button[i].mid_y = button[i].y + BUTTON_HEIGHT / 2;
	}
}




void resetButtons(int init, int reset_evr)
{
	double text_len;
	int i;

	/* Reset dial angles and a few other things */
	for(i=0;i < NUM_BUTTONS;++i)
	{
		if (init)
		{
			/* Set default button states */
			switch(i)
			{
			case BUT_SINE_FM:
			case BUT_BUFFER_RESET:
				button[i].pressed = BUTTON_PRESS;
				break;

			case BUT_EVR_PATCH_RESET:
				if (reset_evr) button[i].pressed = BUTTON_PRESS;
				break;

			case BUT_PAUSE_PROG:
				button[i].pressed = (pause_program ? BUTTON_PRESS : 0);
				break;

			case BUT_EVR_RECORD:
			case BUT_EVR_PLAY:
			case BUT_EVR_PLAY_PAUSE:
			case BUT_EVR_LOOP:
			case BUT_EVR_EVENTS_IN_SAVE:
			case BUT_EVR_PATCH_EQ_MAIN:
			case BUT_EVR_CLEAR:
				if (reset_evr) button[i].pressed = 0;
				break;

			default:
				button[i].pressed = 0;
			}
		}
		button[i].type = BTYPE_DIAL;

		/* Set up dial values */
		switch(i)
		{
		case BUT_SINE_FM:
		case BUT_SINE:
		case BUT_SQUARE:
		case BUT_TRIANGLE:
		case BUT_SAWTOOTH:
		case BUT_AAH:
		case BUT_OOH:
		case BUT_NOISE:
		case BUT_SAMPLE:
		case BUT_EVR_LOOP:
		case BUT_EVR_PATCH_RESET:
		case BUT_EVR_EVENTS_IN_SAVE:
		case BUT_SPECTRUM_ANALYSER:
		case BUT_BUFFER_RESET:
		case BUT_HIGHPASS_FILTER:
		case BUT_ECHO_INVERT:
		case BUT_ECHO_HIGHPASS_FILTER:
		case BUT_HOLD_NOTE:
		case BUT_SUB1_NOTE_OFFSET:
		case BUT_SUB2_NOTE_OFFSET:
		case BUT_SUBS_FOLLOW:
		case BUT_PAUSE_PROG:
			button[i].type = BTYPE_STATIC;
			break;

		case BUT_EVR_PATCH_EQ_MAIN:
		case BUT_EVR_CLEAR:
		case BUT_RESET:
		case BUT_ECHO_CLEAR:
		case BUT_ARP_SPACING_DEC:
		case BUT_ARP_SPACING_INC:
		case BUT_ARP_DELAY_DEC:
		case BUT_ARP_DELAY_INC:
		case BUT_EFFECTS_TO_SWAP:
		case BUT_SWAP_EFFECTS:
		case BUT_RESET_EFFECTS_SEQ:
		case BUT_SAVE_PATCH:
		case BUT_SAVE_PROG:
		case BUT_LOAD_PATCH:
		case BUT_SAMPLE_MOD_DEC:
		case BUT_SAMPLE_MOD_INC:
		case BUT_RANDOMISE:
		case BUT_RESTART_PROG:
		case BUT_DEL_PROG:
		case BUT_RING_MODE:
			button[i].type = BTYPE_CLICK;
			break;

		case BUT_ANALYSER_RANGE:
			button[i].angle = params.analyser_range;
			break;

		case BUT_ECHO_LEN:
			button[i].angle = shm->echo_len;
			break;

		case BUT_ECHO_DECAY:
			button[i].angle = shm->echo_decay;
			break;

		case BUT_ECHO_FILTER:
			button[i].angle = shm->echo_filter;
			break;

		case BUT_ECHO_STRETCH:
			button[i].angle = shm->echo_stretch;
			break;

		case BUT_SUB1_OFFSET:
			button[i].angle = shm->sub1_offset;
			break;

		case BUT_SUB2_OFFSET:
			button[i].angle = shm->sub2_offset;
			break;

		case BUT_SUB1_VOL:
			button[i].angle = shm->sub1_vol;
			break;

		case BUT_SUB2_VOL:
			button[i].angle = shm->sub2_vol;
			break;

		case BUT_VIB_SWEEP:
			button[i].angle = shm->vib_sweep;
			break;

		case BUT_VIB_LFO:
			button[i].angle = shm->vib_lfo;
			break;

		case BUT_SQUARE_WIDTH:
			button[i].angle = shm->square_width;
			break;

		case BUT_SQUARE_WIDTH_LFO:
			button[i].angle = shm->square_width_lfo;
			break;

		case BUT_SINE_CUTOFF:
			button[i].angle = shm->sine_cutoff;
			break;

		case BUT_SINE_CUTOFF_LFO:
			button[i].angle = shm->sine_cutoff_lfo;
			break;

		case BUT_GLIDE_DISTANCE:
			button[i].angle = shm->glide_distance;
			break;

		case BUT_GLIDE_VELOCITY:
			button[i].angle = shm->glide_velocity;
			break;

		case BUT_MAX_FREQ:
			button[i].angle = params.freq_range;
			break;

		case BUT_COMPRESS_START:
			button[i].angle = shm->compress_start;
			break;

		case BUT_COMPRESS_EXP:
			button[i].angle = shm->compress_exponent;
			break;

		case BUT_FM_HARM_OFFSET:
			button[i].angle = shm->fm_harm_offset;
			break;

		case BUT_FM_MULT1:
			button[i].angle = shm->fm_mult1;
			break;

		case BUT_FM_MULT2:
			button[i].angle = shm->fm_mult2;
			break;

		case BUT_FM_OFFSET:
			button[i].angle = shm->fm_offset;
			break;

		case BUT_FM_VOL1:
			button[i].angle = shm->fm_volume1;
			break;

		case BUT_FM_VOL2:
			button[i].angle = shm->fm_volume2;
			break;

		case BUT_FM_WIERD:
			button[i].angle = shm->fm_wierd;
			break;

		case BUT_PHASING_OFFSET:
			button[i].angle = shm->phasing_offset;
			break;

		case BUT_PHASING_SWEEP:
			button[i].angle = shm->phasing_sweep;
			break;

		case BUT_PHASING_LFO:
			button[i].angle = shm->phasing_lfo;
			break;

		case BUT_PHASER_FREQ_SEP:
			button[i].angle = shm->phaser_freq_sep;
			break;

		case BUT_PHASER_LOW_OFF_MULT:
			button[i].angle = shm->phaser_low_off_mult;
			break;

		case BUT_FILTER_SWEEP:
			button[i].angle = shm->filter_sweep;
			break;

		case BUT_FILTER_LFO:
			button[i].angle = shm->filter_lfo;
			break;

		case BUT_RES_LEVEL:
			button[i].angle = shm->res_level;
			break;

		case BUT_RES_FREQ:
			button[i].angle = shm->res_freq;
			break;

		case BUT_RES_DAMPING:
			button[i].angle = shm->res_damping;
			break;

		case BUT_REFLECT_LEVEL:
			button[i].angle = shm->reflect_level;
			break;

		case BUT_REFLECT_SMOOTHING:
			button[i].angle = shm->reflect_smoothing;
			break;

		case BUT_SAW_FLATTEN:
			button[i].angle = shm->saw_flatten;
			break;

		case BUT_SAW_FLATTEN_LFO:
			button[i].angle = shm->saw_flatten_lfo;
			break;

		case BUT_DISTORTION:
			button[i].angle = shm->distortion;
			break;

		case BUT_ALIASING:
			button[i].angle = shm->aliasing;
			break;

		case BUT_RING_FREQ:
			button[i].angle = shm->ring_freq;
			break;

		case BUT_RING_LEVEL:
			button[i].angle = shm->ring_level;
			break;

		case BUT_ATTACK:
			button[i].angle = shm->attack;
			break;

		case BUT_DECAY:
			button[i].angle = shm->decay;
			break;

		case BUT_VOLUME:
			button[i].angle = shm->volume;
			break;

		case BUT_EVR_RECORD:
		case BUT_EVR_PLAY:
		case BUT_EVR_PLAY_PAUSE:
		case BUT_QUIT:
		case BUT_LOAD_PROG:
		case BUT_FREQ_MODE:
		case BUT_CHORD:
		case BUT_ARP_SEQ:
		case BUT_SUB1_SOUND:
		case BUT_SUB2_SOUND:
		case BUT_PHASING_MODE:
		case BUT_RES_MODE:
		case BUT_RING_RANGE:
			button[i].type = BTYPE_BLINK;
			break;

		default:
			assert(0);
		}
		button[i].angle += DIAL_ANGLE_MIN;
	
		if (!init) continue;

		button[i].text_y = button[i].y + BUTTON_HEIGHT / 3;
		button[i].text_y_scale = (double)BUTTON_HEIGHT / CHAR_SIZE / 3;

		/* Buttons with longer names need a calculated scale */
		text_len = getTextPixelLen(strlen(button_name[i]),1);
		button[i].text_x_scale = ((double)(button[i].width - 
		                          BUTTON_GAP * 2) / text_len) * 0.75;
		button[i].text_x = button[i].mid_x - (
		                   text_len * button[i].text_x_scale) / 2 + 1;

		/* Set any underline text required */
		switch(i)
		{
		/* No shift - underline 1st letter with a minus */
		case BUT_HOLD_NOTE:
		case BUT_CHORD:
		case BUT_ARP_SEQ:
		case BUT_FREQ_MODE:
		case BUT_SWAP_EFFECTS:
		case BUT_SAVE_PATCH:
		case BUT_LOAD_PATCH:
		case BUT_PHASING_MODE:
		case BUT_RES_MODE:
			button[i].underline_text = "-";
			button[i].underline_text_y = button[i].text_y + 8 * button[i].text_y_scale;
			break;

		/* No shift - underline 2nd letter */
		case BUT_EFFECTS_TO_SWAP:
			button[i].underline_text = " -";
			button[i].underline_text_y = button[i].text_y + 8 * button[i].text_y_scale;
			break;

		/* Shift - underline 1st letter with a tilda */
		case BUT_SAVE_PROG:
		case BUT_LOAD_PROG:
		case BUT_RANDOMISE:
			button[i].underline_text = "~";
			button[i].underline_text_y = button[i].text_y + 8 * button[i].text_y_scale;
			break;
			
		/* Don't underline */
		default:
			button[i].underline_text = NULL;
			button[i].underline_text_y = 0;
		}
	}
}




/*** Do countdown for click animation ***/
void updateClickButtons()
{
	int but;

	for(but=0;but < NUM_BUTTONS;++but)
	{
		if (button[but].type == BTYPE_STATIC || 
		    button[but].type == BTYPE_DIAL) continue;

		if (button[but].pressed) --button[but].pressed;

		/* Flash certain buttons if their functions are on or in the
		   case of quit when the escape key can be pressed to quit or
		   BASIC program load when you can reload last prog */
		else if ((but == BUT_QUIT && escape_cnt) ||
		         (but == BUT_LOAD_PROG && load_prog_cnt) ||
		         (but == BUT_FREQ_MODE && shm->freq_mode != FREQ_NOTES) ||
		         (but == BUT_CHORD && shm->chord) ||
		         (but == BUT_ARP_SEQ && shm->arp_seq) ||
		         (but == BUT_SUB1_SOUND && shm->sub1_sound) ||
		         (but == BUT_SUB2_SOUND && shm->sub2_sound) ||
		         (but == BUT_PHASING_MODE && shm->phasing_mode) ||
		         (but == BUT_RES_MODE && shm->res_mode) ||
		         (but == BUT_RING_RANGE && shm->ring_range) ||
		         (but == BUT_EVR_RECORD && evr_state == RECORDER_RECORD) ||
		         (but == BUT_EVR_PLAY && evr_state == RECORDER_PLAY) ||
		         (but == BUT_EVR_PLAY_PAUSE && evr_state == RECORDER_PLAY_PAUSED))
		
		{
			assert(button[but].type == BTYPE_BLINK);
			button[but].pressed = BUTTON_PRESS * 2;
		}
	}
}




/*** Update the main oscillator buttons (at the top) based on the main
     sound. This is used after a patch is loaded and by BASIC setting
     SYS:MAIN_OSC ***/
void updateMainOscButtons()
{
	int i;
	for(i=0;i < NUM_TOP_BUTTONS;++i) button[i].pressed = 0;
	button[shm->sound - 1].pressed = BUTTON_PRESS;
}




/*** Get the relative angle of x2,y2 from x1,y1. Because Y increases down
     the window, zero degrees is returned if x2,y2 is directly below and 180
     degrees if directly above ***/
double getAngle(double x1, double y1, double x2, double y2)
{
	double xd;
	double yd;
	double ang;

	xd = x1 - x2;
	yd = y2 - y1;

	if (!yd) ang = (xd > 0 ? 90 : 270);
	else
	if (!xd) ang = (yd > 0 ? 0 : 180);
	else
	{
		ang = (double)(atan(xd / yd) * DEGS_PER_RADIAN);
		if (yd < 0) ang += 180;
		else
		if (xd < 0) ang += 360;
	}
	return ang;
}




/*** Returns 1 if x,y position is over the button ***/
int overButton(int but, double x, double y)
{
	return (x >= button[but].x &&
	        x <= button[but].x + button[but].width &&
	        y >= button[but].y &&
	        y <= button[but].y + BUTTON_HEIGHT);
}




/*** GUI button pressed. Do something ***/
int buttonPressed(int mouse_button, char motion, double wx, double wy)
{
	double angle = 0;
	int but;

	/* Find button thats been pressed */
	for(but=0;but < NUM_BUTTONS;++but)
	{
		if (!overButton(but,wx,wy)) continue;

		/* Button , not dial pressed */
		if (button[but].type != BTYPE_DIAL)
		{
			if (motion) return 1;
			break;
		}

		/* Dial - Left mouse button gets angle from mouse position, 
		   wheel increments/decrements */
		switch(mouse_button)
		{
		case LEFT_MBUTTON:
			angle = getAngle(
				button[but].mid_x,button[but].mid_y,wx,wy);
			break;

		case CENTRE_MBUTTON:
		case RIGHT_MBUTTON:
			/* Not currently used */
			return 1;

		case WHEEL_DOWN:
			angle = button[but].angle - 1;
			break;

		case WHEEL_UP:
			angle = button[but].angle + 1;
			break;

		default:
			printf("WARNING: Mouse button %d not used\n",mouse_button);
			return 1;
		}
		break;
	}

	/* Return if no button pressed or we've pressed an on/off button with
	   anything other than the left mouse button */
	if (but == NUM_BUTTONS ||
	    (mouse_button != LEFT_MBUTTON && button[but].type == BTYPE_STATIC))
		return 0;

	doButtonAction(mouse_button,but,angle,0);
	return 1;
}




void setDialAngle(int but, double angle)
{
	/* We map angle directly to 8 bit value so 
	   there need to be limits:
	   (360 - 255) / 2 = 52.5
	*/
	if (angle < DIAL_ANGLE_MIN) angle = DIAL_ANGLE_MIN; 
	else if (angle > DIAL_ANGLE_MAX) angle = DIAL_ANGLE_MAX;

	button[but].angle = angle;
}




/*** This is seperated from buttonPress() so it can be called directly by
     setButtonField(). "bas_on" set to -1 for OFF, 1 for ON when called
     by Basic for FLIP() buttons only ***/
void doButtonAction(int mouse_button, int but, double angle, int bas_on)
{
	double dval;
	int ival;
	int i;

	if (button[but].type == BTYPE_DIAL)
		setDialAngle(but,angle);
	else
		button[but].pressed = BUTTON_PRESS;

	switch(but)
	{
	/*** BOTTOM LAYER 1 ***/
	case BUT_EVR_RECORD:
		evrRecord(bas_on);
		break;

	case BUT_EVR_PLAY:
		evrPlay();
		break;

	case BUT_EVR_PLAY_PAUSE:
		evrPause();
		break;

	case BUT_EVR_LOOP:
		if (bas_on)
			evrLoop(ON_OFF());
		else
			evrLoop(!evr_flags.loop);
		break;

	case BUT_EVR_EVENTS_IN_SAVE:
		if (bas_on)
			evrEventsInSave(ON_OFF());
		else
			evrEventsInSave(!evr_flags.events_in_save);
		break;

	case BUT_EVR_PATCH_RESET:
		if (bas_on)
			evrPatchReset(ON_OFF());
		else
			evrPatchReset(!evr_flags.patch_reset);
		break;

	case BUT_EVR_PATCH_EQ_MAIN:
		if (bas_on != OFF) evrSetPatchToMain();
		break;

	case BUT_EVR_CLEAR:
		if (bas_on != OFF) evrClear(1);
		break;

	/*** BOTTOM LAYER 2 ***/
	case BUT_SPECTRUM_ANALYSER:
		if (bas_on)
			params.show_analyser = ON_OFF();
		else
			FLIP(params.show_analyser,BUT_SPECTRUM_ANALYSER);

		message("Spectrum analyser = %s",onoff[params.show_analyser]);
		runEventSection(
			SECTION_BUTTON,
			but,params.show_analyser,onoff[params.show_analyser]);
		break;

	case BUT_BUFFER_RESET:
		if (bas_on)
			shm->buffer_reset = ON_OFF();
		else
			FLIP(shm->buffer_reset,BUT_BUFFER_RESET);

		message("Buffer reset = %s",onoff[shm->buffer_reset]);
		runEventSection(SECTION_BUTTON,but,shm->buffer_reset,onoff[shm->buffer_reset]);
		break;

	case BUT_HIGHPASS_FILTER:
		if (bas_on)
			shm->highpass_filter = ON_OFF();
		else
			FLIP(shm->highpass_filter,BUT_HIGHPASS_FILTER);

		message("Highpass filter = %s",onoff[shm->highpass_filter]);
		runEventSection(
			SECTION_BUTTON,
			but,shm->highpass_filter,onoff[shm->highpass_filter]);
		break;

	case BUT_ECHO_HIGHPASS_FILTER:
		if (bas_on)
			shm->echo_highpass_filter = ON_OFF();
		else
			FLIP(shm->echo_highpass_filter,BUT_ECHO_HIGHPASS_FILTER);
		message("Echo highpass filter = %s",onoff[shm->echo_highpass_filter]);
		runEventSection(
			SECTION_BUTTON,
			but,
			shm->echo_highpass_filter,
			onoff[shm->echo_highpass_filter]);
		break;

	case BUT_HOLD_NOTE:
		if (bas_on)
			params.hold_note = ON_OFF();
		else
			FLIP(params.hold_note,BUT_HOLD_NOTE);

		message("Hold note = %s",onoff[params.hold_note]);
		runEventSection(SECTION_BUTTON,but,params.hold_note,onoff[params.hold_note]);
		break;

	case BUT_SUB1_NOTE_OFFSET:
		if (bas_on)
			shm->sub1_note_offset = ON_OFF();
		else
			FLIP(shm->sub1_note_offset,BUT_SUB1_NOTE_OFFSET);

		message("Sub1 note offset = %s",onoff[shm->sub1_note_offset]);
		runEventSection(
			SECTION_BUTTON,
			but,shm->sub1_note_offset,onoff[shm->sub1_note_offset]);
		break;

	case BUT_SUB2_NOTE_OFFSET:
		if (bas_on)
			shm->sub2_note_offset = ON_OFF();
		else
			FLIP(shm->sub2_note_offset,BUT_SUB2_NOTE_OFFSET);

		message("Sub2 note offset = %s",onoff[shm->sub2_note_offset]);
		runEventSection(
			SECTION_BUTTON,
			but,shm->sub2_note_offset,onoff[shm->sub2_note_offset]);
		break;

	case BUT_SUBS_FOLLOW:
		if (bas_on)
			params.subs_follow_main = ON_OFF();
		else
			FLIP(params.subs_follow_main,BUT_SUBS_FOLLOW);

		message("Sub oscillators follow main = %s",
			onoff[params.subs_follow_main]);
		runEventSection(
			SECTION_BUTTON,
			but,
			params.subs_follow_main,onoff[params.subs_follow_main]);
		break;

	case BUT_RANDOMISE:
		if (bas_on != OFF) randomiseSettings();
		runEventSection(SECTION_BUTTON,but,1,NULL);
		break;

	case BUT_RESTART_PROG:
		if (bas_on != OFF) restartProgram();
		break;

	case BUT_DEL_PROG:
		if (bas_on != OFF)
		{
			deleteProgram(0);
			FREE(basic_file);
			setTitleBar(NULL);
		}
		break;

	case BUT_PAUSE_PROG:
		/* Won't call BASIC here for obvious reasons */
		if (bas_on)
			pauseProgram(ON_OFF());
		else
			pauseProgram(!pause_program);
		break;

	/*** BOTTOM LAYER 3 ***/
	case BUT_RESET:
		if (bas_on != OFF) reset(1);
		break;

	case BUT_ANALYSER_RANGE:
		setFieldToDialAngle(&params.analyser_range,BUT_ANALYSER_RANGE);
		ival = getAnalyserRange();
		message("Analyser range = %dHz",ival);
		runEventSection(SECTION_DIAL,but,ival,NULL);
		break;
		
	case BUT_ECHO_CLEAR:
		shm->echo_clear = 1;
		message("Echo buffer cleared");
		runEventSection(SECTION_BUTTON,but,1,"");
		break;

	case BUT_ECHO_INVERT:
		if (bas_on)
			shm->echo_invert = ON_OFF();
		else
			FLIP(shm->echo_invert,BUT_ECHO_INVERT);
		message("Echo invert = %s",onoff[shm->echo_invert]);
		runEventSection(
			SECTION_BUTTON,
			but,shm->echo_invert,onoff[shm->echo_invert]);
		break;

	case BUT_ECHO_LEN:
		setFieldToDialAngle(&shm->echo_len,BUT_ECHO_LEN);
		message("Echo length = %d",shm->echo_len);
		runEventSection(SECTION_DIAL,but,shm->echo_len,NULL);
		break;

	case BUT_ECHO_DECAY:
		setFieldToDialAngle(&shm->echo_decay,BUT_ECHO_DECAY);
		message("Echo decay = %d",shm->echo_decay);
		runEventSection(SECTION_DIAL,but,shm->echo_decay,NULL);
		break;

	case BUT_ECHO_FILTER:
		setFieldToDialAngle(&shm->echo_filter,BUT_ECHO_FILTER);
		message("Echo filter = %d",shm->echo_filter);
		runEventSection(SECTION_DIAL,but,shm->echo_filter,NULL);
		break;

	case BUT_ECHO_STRETCH:
		setFieldToDialAngle(&shm->echo_stretch,BUT_ECHO_STRETCH);
		dval = getEchoStretch();
		message("Echo stretch = %.3f",dval);
		runEventSection(SECTION_DIAL,but,dval,NULL);
		break;
		
	case BUT_CHORD:
		setOrCycleChord(mouse_button,NULL,0);
		break;

	case BUT_ARP_SEQ:
		setOrCycleARP(mouse_button,NULL,0);
		break;

	case BUT_ARP_SPACING_DEC:
		if (shm->arp_spacing > 1) incField(&shm->arp_spacing,-1);
		message("ARP spacing = %d",shm->arp_spacing);
		runEventSection(SECTION_BUTTON,but,shm->arp_spacing,"");
		break;

	case BUT_ARP_SPACING_INC:
		incField(&shm->arp_spacing,1);
		message("ARP spacing = %d",shm->arp_spacing);
		runEventSection(SECTION_BUTTON,but,shm->arp_spacing,"");
		break;

	case BUT_ARP_DELAY_DEC:
		if (shm->arp_delay > 1) incField(&shm->arp_delay,-1);
		message("ARP delay = %d",shm->arp_delay);
		runEventSection(SECTION_BUTTON,but,shm->arp_delay,"");
		break;

	case BUT_ARP_DELAY_INC:
		incField(&shm->arp_delay,1);
		message("ARP delay = %d",shm->arp_delay);
		runEventSection(SECTION_BUTTON,but,shm->arp_delay,"");
		break;

	case BUT_SUB1_SOUND:
		setOrCycleSubOsc(1,mouse_button,NULL,0);
		break;

	case BUT_SUB2_SOUND:
		setOrCycleSubOsc(2,mouse_button,NULL,0);
		break;

	case BUT_SUB1_OFFSET:
		setFieldToDialAngle(&shm->sub1_offset,BUT_SUB1_OFFSET);
		ival = shm->sub1_offset - MAX_CHAR;
		message("Sub osc 1 offset = %d%s",
			ival,shm->sub1_note_offset ? " notes" : "Hz");
		runEventSection(SECTION_DIAL,but,ival,NULL);
		break;

	case BUT_SUB2_OFFSET:
		setFieldToDialAngle(&shm->sub2_offset,BUT_SUB2_OFFSET);
		ival = shm->sub2_offset - MAX_CHAR;
		message("Sub osc 2 offset = %d%s",
			ival,shm->sub2_note_offset ? " notes" : "Hz");
		runEventSection(SECTION_DIAL,but,ival,NULL);
		break;

	case BUT_SUB1_VOL:
		setFieldToDialAngle(&shm->sub1_vol,BUT_SUB1_VOL);
		message("Sub osc 1 volume = %d",shm->sub1_vol);
		runEventSection(SECTION_DIAL,but,shm->sub1_vol,NULL);
		break;

	case BUT_SUB2_VOL:
		setFieldToDialAngle(&shm->sub2_vol,BUT_SUB2_VOL);
		message("Sub osc 2 volume = %d",shm->sub2_vol);
		runEventSection(SECTION_DIAL,but,shm->sub2_vol,NULL);
		break;

	case BUT_VIB_SWEEP:
		setFieldToDialAngle(&shm->vib_sweep,BUT_VIB_SWEEP);
		message("Vibrato sweep = %d",shm->vib_sweep);
		runEventSection(SECTION_DIAL,but,shm->vib_sweep,NULL);
		break;

	case BUT_VIB_LFO:
		setFieldToDialAngle(&shm->vib_lfo,BUT_VIB_LFO);
		message("Vibrato LFO = %d",shm->vib_lfo);
		runEventSection(SECTION_DIAL,but,shm->vib_lfo,NULL);
		break;

	case BUT_SQUARE_WIDTH:
		setFieldToDialAngle(&shm->square_width,BUT_SQUARE_WIDTH);
		message("Square width = %d",shm->square_width);
		runEventSection(SECTION_DIAL,but,shm->square_width,NULL);
		break;

	case BUT_SQUARE_WIDTH_LFO:
		setFieldToDialAngle(&shm->square_width_lfo,BUT_SQUARE_WIDTH_LFO);
		message("Square width LFO = %d",shm->square_width_lfo);
		runEventSection(SECTION_DIAL,but,shm->square_width_lfo,NULL);
		break;

	case BUT_SINE_CUTOFF:
		setFieldToDialAngle(&shm->sine_cutoff,BUT_SINE_CUTOFF);
		message("Sine cutoff = %d",shm->sine_cutoff);
		runEventSection(SECTION_DIAL,but,shm->sine_cutoff,NULL);
		break;

	case BUT_SINE_CUTOFF_LFO:
		setFieldToDialAngle(&shm->sine_cutoff_lfo,BUT_SINE_CUTOFF_LFO);
		message("Sine cutoff LFO = %d",shm->sine_cutoff_lfo);
		runEventSection(SECTION_DIAL,but,shm->sine_cutoff_lfo,NULL);
		break;

	case BUT_GLIDE_DISTANCE:
		setFieldToDialAngle(&shm->glide_distance,BUT_GLIDE_DISTANCE);
		ival = shm->glide_distance - MAX_CHAR;
		message("Glide distance = %d",ival);
		runEventSection(SECTION_DIAL,but,ival,NULL);
		break;

	case BUT_GLIDE_VELOCITY:
		setFieldToDialAngle(&shm->glide_velocity,BUT_GLIDE_VELOCITY);
		message("Glide velocity = %d",shm->glide_velocity);
		runEventSection(SECTION_DIAL,but,shm->glide_velocity,NULL);
		break;

	case BUT_FREQ_MODE:
		setOrCycleFreqMode(mouse_button,NULL,0);
		break;

	case BUT_MAX_FREQ:
		setFieldToDialAngle(&params.freq_range,BUT_MAX_FREQ);
		setByRange();
		if (shm->freq_mode == FREQ_NOTES)
		{
			message("Keys count = %d",key_cnt);
			runEventSection(SECTION_DIAL,but,key_cnt,NULL);
		}
		else
		{
			dval = freq_range_mult * WIN_WIDTH;
			message("Max frequency = %dHz",(int)dval);
			runEventSection(SECTION_DIAL,but,dval,NULL);
		}
		break;

	case BUT_COMPRESS_START:
		setFieldToDialAngle(&shm->compress_start,BUT_COMPRESS_START);
		message("Gain compression start = %d",shm->compress_start);
		runEventSection(SECTION_DIAL,but,shm->compress_start,NULL);
		break;

	case BUT_COMPRESS_EXP:
		setFieldToDialAngle(&shm->compress_exponent,BUT_COMPRESS_EXP);
		dval = getGainCompressionExponent();
		message("Gain compression exp = %.3f",dval);
		runEventSection(SECTION_DIAL,but,dval,NULL);
		break;

	case BUT_EFFECTS_TO_SWAP:
		setOrCycleEffectsSeq(mouse_button,0);
		break;

	case BUT_SWAP_EFFECTS:
		/* bas_on will be OFF if set() called for this button with a
		   zero which is pointless since its a click button but
		   just in case someone does check for OFF */
		if (bas_on != OFF) swapEffects();
		break;

	case BUT_RESET_EFFECTS_SEQ:
		if (bas_on != OFF) resetEffectsSeq(1);
		break;

	/*** BOTTOM LAYER 4 ***/
	case BUT_SAVE_PATCH:
		if (bas_on != OFF) savePatchMode();
		break;

	case BUT_SAVE_PROG:
		if (bas_on != OFF) saveProgramMode();
		break;

	case BUT_LOAD_PATCH:
		if (bas_on != OFF) loadPatchMode();
		break;

	case BUT_LOAD_PROG:
		if (bas_on == OFF) break;
		if (load_prog_cnt)
		{
			loadProgram(0);
			clearDiskStruct();
		}
		else
		{
			/* If we have a program name set timer */
			if (basic_file) load_prog_cnt = PRESS_TIMEOUT_CNT;
			loadProgramMode();
		}
		break;

	case BUT_SAMPLE_MOD_DEC:
		incField(&shm->sample_mod_cnt,-1);
		message("Sample mod count = %d",shm->sample_mod_cnt);
		runEventSection(SECTION_BUTTON,but,shm->sample_mod_cnt,"");
		break;

	case BUT_SAMPLE_MOD_INC:
		incField(&shm->sample_mod_cnt,1);
		message("Sample mod count = %d",shm->sample_mod_cnt);
		runEventSection(SECTION_BUTTON,but,shm->sample_mod_cnt,"");
		break;

	case BUT_FM_HARM_OFFSET:
		setFieldToDialAngle(&shm->fm_harm_offset,BUT_FM_HARM_OFFSET);
		ival = shm->fm_harm_offset - MAX_CHAR;
		message("FM harmonic offset = %dHz",ival);
		runEventSection(SECTION_DIAL,but,ival,NULL);
		break;

	case BUT_FM_MULT1:
		setFieldToDialAngle(&shm->fm_mult1,BUT_FM_MULT1);
		message("FM multiply 1 = %d",shm->fm_mult1);
		runEventSection(SECTION_DIAL,but,shm->fm_mult1,NULL);
		break;

	case BUT_FM_MULT2:
		setFieldToDialAngle(&shm->fm_mult2,BUT_FM_MULT2);
		message("FM multiply 2 = %d",shm->fm_mult2);
		runEventSection(SECTION_DIAL,but,shm->fm_mult2,NULL);
		break;

	case BUT_FM_OFFSET:
		setFieldToDialAngle(&shm->fm_offset,BUT_FM_OFFSET);
		dval = getFMOffset();
		message("FM offset = %.1fHz",dval);
		runEventSection(SECTION_DIAL,but,dval,NULL);
		break;

	case BUT_FM_VOL1:
		setFieldToDialAngle(&shm->fm_volume1,BUT_FM_VOL1);
		message("FM volume 1 = %d",shm->fm_volume1);
		runEventSection(SECTION_DIAL,but,shm->fm_volume1,NULL);
		break;

	case BUT_FM_VOL2:
		setFieldToDialAngle(&shm->fm_volume2,BUT_FM_VOL2);
		message("FM volume 2 = %d",shm->fm_volume2);
		runEventSection(SECTION_DIAL,but,shm->fm_volume2,NULL);
		break;

	case BUT_FM_WIERD:
		setFieldToDialAngle(&shm->fm_wierd,BUT_FM_WIERD);
		message("FM wierd = %d",shm->fm_wierd);
		runEventSection(SECTION_DIAL,but,shm->fm_wierd,NULL);
		break;

	case BUT_PHASING_MODE:
		setOrCyclePhasingMode(mouse_button,NULL,0);
		break;
		
	case BUT_PHASING_OFFSET:
		setFieldToDialAngle(&shm->phasing_offset,BUT_PHASING_OFFSET);
		if (shm->phasing_mode == PHASING_FM_FLANGER)
			message("FM flanger layers = %d",shm->phasing_offset);
		else
			message("Phasing offset = %d",shm->phasing_offset);
		runEventSection(SECTION_DIAL,but,shm->phasing_offset,NULL);
		break;

	case BUT_PHASING_SWEEP:
		setFieldToDialAngle(&shm->phasing_sweep,BUT_PHASING_SWEEP);
		message("Phasing sweep = %d",shm->phasing_sweep);
		runEventSection(SECTION_DIAL,but,shm->phasing_sweep,NULL);
		break;

	case BUT_PHASING_LFO:
		setFieldToDialAngle(&shm->phasing_lfo,BUT_PHASING_LFO);
		message("Phasing LFO = %d",shm->phasing_lfo);
		runEventSection(SECTION_DIAL,but,shm->phasing_lfo,NULL);
		break;

	case BUT_PHASER_FREQ_SEP:
		setFieldToDialAngle(&shm->phaser_freq_sep,BUT_PHASER_FREQ_SEP);
		message("Phaser freq sep = %d",shm->phaser_freq_sep);
		runEventSection(SECTION_DIAL,but,shm->phaser_freq_sep,NULL);
		break;

	case BUT_PHASER_LOW_OFF_MULT:
		setFieldToDialAngle(&shm->phaser_low_off_mult,BUT_PHASER_LOW_OFF_MULT);
		dval = getPhaserLOM();
		message("Phaser low freq offset mult = %.2f",dval);
		runEventSection(SECTION_DIAL,but,dval,NULL);
		break;

	case BUT_FILTER_SWEEP:
		setFieldToDialAngle(&shm->filter_sweep,BUT_FILTER_SWEEP);
		message("Filter sweep = %d",shm->filter_sweep);
		runEventSection(SECTION_DIAL,but,shm->filter_sweep,NULL);
		break;

	case BUT_FILTER_LFO:
		setFieldToDialAngle(&shm->filter_lfo,BUT_FILTER_LFO);
		message("Filter LFO = %d",shm->filter_lfo);
		runEventSection(SECTION_DIAL,but,shm->filter_lfo,NULL);
		break;

	case BUT_RES_MODE:
		setOrCycleResMode(mouse_button,NULL,0);
		break;
		
	case BUT_RES_LEVEL:
		setFieldToDialAngle(&shm->res_level,BUT_RES_LEVEL);
		message("Resonance level = %d",shm->res_level);
		runEventSection(SECTION_DIAL,but,shm->res_level,NULL);
		break;

	case BUT_RES_FREQ:
		setFieldToDialAngle(&shm->res_freq,BUT_RES_FREQ);
		message("Resonance freq = %d",shm->res_freq);
		runEventSection(SECTION_DIAL,but,shm->res_freq,NULL);
		break;

	case BUT_RES_DAMPING:
		setFieldToDialAngle(&shm->res_damping,BUT_RES_DAMPING);
		dval = getResonanceDamping();
		message("Resonance damping = %.3f",dval);
		runEventSection(SECTION_DIAL,but,dval,NULL);
		break;

	case BUT_REFLECT_LEVEL:
		setFieldToDialAngle(&shm->reflect_level,BUT_REFLECT_LEVEL);
		message("Reflect level = %d",shm->reflect_level);
		runEventSection(SECTION_DIAL,but,shm->reflect_level,NULL);
		break;

	case BUT_REFLECT_SMOOTHING:
		setFieldToDialAngle(&shm->reflect_smoothing,BUT_REFLECT_SMOOTHING);
		message("Reflect smoothing = %d",shm->reflect_smoothing);
		runEventSection(SECTION_DIAL,but,shm->reflect_smoothing,NULL);
		break;

	case BUT_SAW_FLATTEN:
		setFieldToDialAngle(&shm->saw_flatten,BUT_SAW_FLATTEN);
		message("Saw flatten = %d",shm->saw_flatten);
		runEventSection(SECTION_DIAL,but,shm->saw_flatten,NULL);
		break;

	case BUT_SAW_FLATTEN_LFO:
		setFieldToDialAngle(&shm->saw_flatten_lfo,BUT_SAW_FLATTEN_LFO);
		message("Saw flatten LFO = %d",shm->saw_flatten_lfo);
		runEventSection(SECTION_DIAL,but,shm->saw_flatten_lfo,NULL);
		break;

	case BUT_DISTORTION:
		setFieldToDialAngle(&shm->distortion,BUT_DISTORTION);
		message("Distortion = %d",shm->distortion);
		runEventSection(SECTION_DIAL,but,shm->distortion,NULL);
		break;

	case BUT_ALIASING:
		setFieldToDialAngle(&shm->aliasing,BUT_ALIASING);
		message("Aliasing = %d",shm->aliasing);
		runEventSection(SECTION_DIAL,but,shm->aliasing,NULL);
		break;

	case BUT_RING_RANGE:
		setOrCycleRingModRange(mouse_button,NULL,0);
		break;

	case BUT_RING_MODE:
		setOrCycleRingModMode(mouse_button,NULL,0);
		break;

	case BUT_RING_FREQ:
		/* Not called by BASIC. That calls setRingFreq() via
		   setButtonField() */
		setFieldToDialAngle(&shm->ring_freq,BUT_RING_FREQ);
		ival = getRingModFreq(shm->ring_freq);
		message("Ring freq = %d Hz",ival);
		runEventSection(SECTION_DIAL,but,ival,NULL);
		break;

	case BUT_RING_LEVEL:
		setFieldToDialAngle(&shm->ring_level,BUT_RING_LEVEL);
		message("Ring level = %d",shm->ring_level);
		runEventSection(SECTION_DIAL,but,shm->ring_level,NULL);
		break;

	case BUT_ATTACK:
		setFieldToDialAngle(&shm->attack,BUT_ATTACK);
		message("Attack = %d",shm->attack);
		runEventSection(SECTION_DIAL,but,shm->attack,NULL);
		break;

	case BUT_DECAY:
		setFieldToDialAngle(&shm->decay,BUT_DECAY);
		message("Decay = %d",shm->decay);
		runEventSection(SECTION_DIAL,but,shm->decay,NULL);
		break;

	case BUT_VOLUME:
		setFieldToDialAngle(&shm->volume,BUT_VOLUME);
		message("Volume = %d",shm->volume);
		runEventSection(SECTION_DIAL,but,shm->volume,NULL);
		break;

	case BUT_QUIT:
		if (bas_on != OFF) quit(QUIT_GUI);
		break;

	default:
		/*** TOP LAYER ***/
		if (but == BUT_SAMPLE && !do_sampling)
			message("WARNING: Sampling unavailable");
		for(i=0;i < NUM_TOP_BUTTONS;++i)
			if (i != but) button[i].pressed = 0;
		setMainOsc(but+1);
		runEventSection(SECTION_BUTTON,but,button[i].pressed > 0,"");
		break;
	}
}




/*** Set the value of the button under the co-ords ***/
void setHoverString(short x, short y)
{
	static char hover_num[10];
	double wx;
	double wy;
	double ang;
	int but;
	int plen;

	wx = (double)x / g_x_scale;
	wy = (double)y / g_y_scale;

	for(but=0;but < NUM_BUTTONS;++but)
	{
		if (!overButton(but,wx,wy)) continue;

		ang = button[but].angle - DIAL_ANGLE_MIN;

		switch(but)
		{
		/* Dial buttons */
		case BUT_ANALYSER_RANGE:
			sprintf(hover_num,"%dHz",(int)params.analyser_range * SAMPLES_PER_SEC);
			hover_str = hover_num;
			break;

		case BUT_ECHO_STRETCH:
			sprintf(hover_num,"%.3f",getEchoStretch());
			hover_str = hover_num;
			break;
			
		case BUT_CHORD:
			/* shm->chord only updated when note played */
			hover_str = chord_name[shm->chord];
			break;

		case BUT_SUB1_SOUND:
			hover_str = sound_name[shm->sub1_sound];
			break;

		case BUT_SUB2_SOUND:
			hover_str = sound_name[shm->sub2_sound];
			break;

		case BUT_SUB1_OFFSET:
		case BUT_SUB2_OFFSET:
		case BUT_FM_HARM_OFFSET:
		case BUT_GLIDE_DISTANCE:
			sprintf(hover_num,"%d",(int)ang - MAX_CHAR);
			hover_str = hover_num;
			break;

		case BUT_COMPRESS_EXP:
			sprintf(hover_num,"%.3f",getGainCompressionExponent());
			hover_str = hover_num;
			break;

		case BUT_FM_OFFSET:
			sprintf(hover_num,"%.1fHz",getFMOffset());
			hover_str = hover_num;
			break;

		case BUT_PHASER_LOW_OFF_MULT:
			sprintf(hover_num,"%.2f",getPhaserLOM());
			hover_str = hover_num;
			break;

		case BUT_MAX_FREQ:
			if (shm->freq_mode == FREQ_NOTES)
				sprintf(hover_num,"%d",getMaxFrequency());
			else
				sprintf(hover_num,"%dHz",getMaxFrequency());
			hover_str = hover_num;
			break;

		case BUT_RES_DAMPING:
			sprintf(hover_num,"%.3f",getResonanceDamping());
			hover_str = hover_num;
			break;
				
		/* Other buttons */
		case BUT_ARP_SEQ:
			hover_str = arp_seq_name[shm->arp_seq];
			break;

		case BUT_FREQ_MODE:
			hover_str = freq_mode[shm->freq_mode];
			break;

		case BUT_PHASING_MODE:
			hover_str = phasing_mode[shm->phasing_mode];
			break;
		
		case BUT_RES_MODE:
			hover_str = resonance_mode[shm->res_mode];
			hover_x_scale = 0.8;
			break;

		case BUT_RING_RANGE:
			hover_str = ring_range[shm->ring_range];
			break;

		case BUT_RING_MODE:
			hover_str = ring_mode[shm->ring_mode];
			break;

		case BUT_RING_FREQ:
			sprintf(hover_num,"%d",getRingModFreq(shm->ring_freq));
			hover_str = hover_num;
			break;

		default:
			if (button[but].type == BTYPE_DIAL)
			{
				sprintf(hover_num,"%d",(int)ang);
				hover_str = hover_num;
				break;
			}	
			hover_str = NULL;
			return;
		}

		/* Set hover params */
		hover_x_scale = (button[but].type == BTYPE_BLINK ? 1 : 2);
		plen = getTextPixelLen(strlen(hover_str),hover_x_scale);
		if (plen > WIN_WIDTH - button[but].x)
			hover_x = WIN_WIDTH - plen;
		else
			hover_x = button[but].x;
		hover_y = button[but].y;
		return;
	}
}




/*** Used by funcGet() & funcSet() in bas_functions ***/
double getButtonValue(int but)
{
	switch(but)
	{
	case BUT_SPECTRUM_ANALYSER:
		return params.show_analyser;

	case BUT_BUFFER_RESET:
		return shm->buffer_reset;

	case BUT_HIGHPASS_FILTER:
		return shm->highpass_filter;

	case BUT_ECHO_HIGHPASS_FILTER:
		return shm->echo_highpass_filter;

	case BUT_HOLD_NOTE:
		return params.hold_note;

	case BUT_SUB1_NOTE_OFFSET:
		return shm->sub1_note_offset;

	case BUT_SUB2_NOTE_OFFSET:
		return shm->sub2_note_offset;

	case BUT_SUBS_FOLLOW:
		return params.subs_follow_main;

	case BUT_ANALYSER_RANGE:
		return (int)params.analyser_range * SAMPLES_PER_SEC;
		
	case BUT_ECHO_CLEAR:
		return shm->echo_clear;

	case BUT_ECHO_LEN:
		return shm->echo_len;

	case BUT_ECHO_DECAY:
		return shm->echo_decay;

	case BUT_ECHO_FILTER:
		return shm->echo_filter;

	case BUT_ECHO_STRETCH:
		return getEchoStretch();

	case BUT_CHORD:
		return shm->chord;

	case BUT_ARP_SEQ:
		return shm->arp_seq;

	case BUT_ARP_SPACING_DEC:
	case BUT_ARP_SPACING_INC:
		return shm->arp_spacing;

	case BUT_ARP_DELAY_DEC:
	case BUT_ARP_DELAY_INC:
		return shm->arp_delay;

	case BUT_SUB1_SOUND:
		return shm->sub1_sound;

	case BUT_SUB2_SOUND:
		return shm->sub2_sound;

	case BUT_SUB1_OFFSET:
		return shm->sub1_offset - MAX_CHAR;

	case BUT_SUB2_OFFSET:
		return shm->sub2_offset - MAX_CHAR;

	case BUT_SUB1_VOL:
		return shm->sub1_vol;

	case BUT_SUB2_VOL:
		return shm->sub2_vol;

	case BUT_VIB_SWEEP:
		return shm->vib_sweep;

	case BUT_VIB_LFO:
		return shm->vib_lfo;

	case BUT_SQUARE_WIDTH:
		return shm->square_width;

	case BUT_SQUARE_WIDTH_LFO:
		return shm->square_width_lfo;

	case BUT_SINE_CUTOFF:
		return shm->sine_cutoff;

	case BUT_SINE_CUTOFF_LFO:
		return shm->sine_cutoff_lfo;

	case BUT_GLIDE_DISTANCE:
		return shm->glide_distance - MAX_CHAR;

	case BUT_GLIDE_VELOCITY:
		return shm->glide_velocity;

	case BUT_FREQ_MODE:
		return shm->freq_mode;

	case BUT_MAX_FREQ:
		return (double)getMaxFrequency();

	case BUT_COMPRESS_START:
		return shm->compress_start;

	case BUT_COMPRESS_EXP:
		return getGainCompressionExponent();

	case BUT_EFFECTS_TO_SWAP:
		return effects_pos[params.effects_swap].pos1;

	case BUT_SAMPLE_MOD_DEC:
	case BUT_SAMPLE_MOD_INC:
		return shm->sample_mod_cnt;

	case BUT_FM_HARM_OFFSET:
		return shm->fm_harm_offset - MAX_CHAR;

	case BUT_FM_MULT1:
		return shm->fm_mult1;

	case BUT_FM_MULT2:
		return shm->fm_mult2;

	case BUT_FM_OFFSET:
		return getFMOffset();

	case BUT_FM_VOL1:
		return shm->fm_volume1;

	case BUT_FM_VOL2:
		return shm->fm_volume2;

	case BUT_DISTORTION:
		return shm->distortion;

	case BUT_PHASING_MODE:
		return shm->phasing_mode;
		
	case BUT_PHASING_OFFSET:
		return shm->phasing_offset;

	case BUT_PHASING_SWEEP:
		return shm->phasing_sweep;

	case BUT_PHASING_LFO:
		return shm->phasing_lfo;

	case BUT_PHASER_FREQ_SEP:
		return shm->phaser_freq_sep;

	case BUT_PHASER_LOW_OFF_MULT:
		return getPhaserLOM();

	case BUT_FILTER_SWEEP:
		return shm->filter_sweep;

	case BUT_FILTER_LFO:
		return shm->filter_lfo;

	case BUT_RES_MODE:
		return shm->res_mode;
		
	case BUT_RES_LEVEL:
		return shm->res_level;

	case BUT_RES_FREQ:
		return shm->res_freq;

	case BUT_RES_DAMPING:
		return getResonanceDamping();

	case BUT_REFLECT_LEVEL:
		return shm->reflect_level;

	case BUT_REFLECT_SMOOTHING:
		return shm->reflect_smoothing;

	case BUT_SAW_FLATTEN:
		return shm->saw_flatten;

	case BUT_ALIASING:
		return shm->aliasing;

	case BUT_ATTACK:
		return shm->attack;

	case BUT_DECAY:
		return shm->decay;

	case BUT_VOLUME:
		return shm->volume;

	case BUT_RING_RANGE:
		return shm->ring_range;

	case BUT_RING_MODE:
		return shm->ring_mode;

	case BUT_RING_FREQ:
		return getRingModFreq(shm->ring_freq);

	case BUT_RING_LEVEL:
		return shm->ring_level;

	default:
		/* Just return 1 or 0 depending on whether button is pressed */
		if (but < NUM_BUTTONS) return (button[but].pressed > 0);
	}
	return 0;
}




/*** Directly set the field that corresponds to the button. Only used by 
     Basic. Returns button number or negative error ***/
int setButtonField(int but, char *strval, double val)
{
	int diff;

	/* Only some button values can be set by a string value */
	if (strval)
	{
		switch(but)
		{
		case BUT_CHORD:
		case BUT_FREQ_MODE:
		case BUT_SUB1_SOUND:
		case BUT_SUB2_SOUND:
		case BUT_PHASING_MODE:
		case BUT_RES_MODE:
		case BUT_ARP_SEQ:
		case BUT_RING_RANGE:
		case BUT_RING_MODE:
			break;

		default:
			return -ERR_INVALID_SET_VALUE;
		}
	}

	/* Click buttons */
	if (button[but].type != BTYPE_DIAL)
	{
		/* Set because doButtonAction() not called for every button */
		if (button[but].type == BTYPE_CLICK)
			button[but].pressed = BUTTON_PRESS;

		switch(but)
		{
		case BUT_CHORD:
			if (!setOrCycleChord(-1,strval,(int)val))
				return -ERR_INVALID_CHORD;
			break;

		case BUT_ARP_SEQ:
			if (!setOrCycleARP(-1,strval,(int)val))
				return -ERR_INVALID_SET_VALUE;
			break;

		case BUT_ARP_SPACING_DEC:
		case BUT_ARP_SPACING_INC:
			setField(&shm->arp_spacing,(int)val,MAX_UCHAR);
			break;

		case BUT_ARP_DELAY_DEC:
		case BUT_ARP_DELAY_INC:
			setField(&shm->arp_delay,(int)val,MAX_UCHAR);
			break;

		case BUT_SAMPLE_MOD_DEC:
		case BUT_SAMPLE_MOD_INC:
			setField(&shm->sample_mod_cnt,(int)val,MAX_UCHAR);
			break;

		case BUT_SUB1_SOUND:
			if (!setOrCycleSubOsc(1,-1,strval,(int)val))
				return -ERR_INVALID_WAVEFORM;
			break;

		case BUT_SUB2_SOUND:
			if (!setOrCycleSubOsc(2,-1,strval,(int)val))
				return -ERR_INVALID_WAVEFORM;
			break;
		
		case BUT_FREQ_MODE:
			if (!setOrCycleFreqMode(-1,strval,(int)val))
				return -ERR_INVALID_SET_VALUE;
			break;

		case BUT_RES_MODE:
			if (!setOrCycleResMode(-1,strval,(int)val))
				return -ERR_INVALID_SET_VALUE;
			break;
	
		case BUT_PHASING_MODE:
			if (!setOrCyclePhasingMode(-1,strval,(int)val))
				return -ERR_INVALID_SET_VALUE;
			break;

		case BUT_EFFECTS_TO_SWAP:
			setOrCycleEffectsSeq(-1,(int)val);
			break;

		case BUT_RING_RANGE:
			if (!setOrCycleRingModRange(-1,strval,(int)val))
				return -ERR_INVALID_SET_VALUE;
			break;
			
		case BUT_RING_MODE:
			if (!setOrCycleRingModMode(-1,strval,(int)val))
				return -ERR_INVALID_SET_VALUE;
			break;

		default:
			doButtonAction(LEFT_MBUTTON,but,0,val > 0 ? 1 : -1);
		}
		return but;
	}

	diff = DIAL_ANGLE_MIN;

	/* Most dial values are simply the dial angle minus a fixed
	   value but there are exceptions */
	switch(but)
	{
	case BUT_ANALYSER_RANGE:
		val /= SAMPLES_PER_SEC;
		doButtonAction(LEFT_MBUTTON,but,val + diff,0);
		break;

	case BUT_PHASER_LOW_OFF_MULT:
		val = val / PHASER_LOW_MULT_RANGE * MAX_UCHAR;
		doButtonAction(LEFT_MBUTTON,but,val + diff,0);
		break;

	case BUT_ECHO_STRETCH:
		val = DIAL_ANGLE_MIN + (val - 0.8) / 0.4 * DIAL_ANGLE_RANGE;
		doButtonAction(LEFT_MBUTTON,but,val,0);
		break;

	case BUT_SUB1_OFFSET:
	case BUT_SUB2_OFFSET:
	case BUT_FM_HARM_OFFSET:
	case BUT_GLIDE_DISTANCE:
		diff = DIAL_ANGLE_MIN + MAX_CHAR;
		doButtonAction(LEFT_MBUTTON,but,val + diff,0);
		break;

	case BUT_COMPRESS_EXP:
		doButtonAction(
			LEFT_MBUTTON,
			but,DIAL_ANGLE_MIN + val * DIAL_ANGLE_RANGE,0);
		break;

	case BUT_MAX_FREQ:
		if (shm->freq_mode == FREQ_NOTES)
		{
			doButtonAction(LEFT_MBUTTON,but,val + diff,0);
			break;
		}
		val = val / WIN_WIDTH * FREQ_RANGE_DIAL_INIT + diff;
		doButtonAction(LEFT_MBUTTON,but,val,0);
		break;

	case BUT_FM_OFFSET:
		diff = DIAL_ANGLE_MIN + MAX_CHAR;
		val = val * FM_OFFSET_DIV + diff;
		doButtonAction(LEFT_MBUTTON,but,val,0);
		break;

	case BUT_RES_DAMPING:
		val = val * MAX_UCHAR + diff;
		doButtonAction(LEFT_MBUTTON,but,val,0);
		break;
		
	case BUT_RING_FREQ:
		/* Setting the ring freq is too complex to be left to 
		   doButtonAction() */
		setRingModFreq(val);
		setDialAngle(BUT_RING_FREQ,DIAL_ANGLE_MIN + shm->ring_freq);	
		break;

	default:
		doButtonAction(LEFT_MBUTTON,but,val + diff,0);
	}
	return but;
}




void setButtonByParam(enum en_button but, u_char param)
{
	button[but].pressed = param ? BUTTON_PRESS : 0;
}




int getButton(char *name)
{
	ENTRY se;
	ENTRY *fe;

	se.key = name;
	return (fe = hsearch(se,FIND)) ? (int)fe->data : -1;
}
