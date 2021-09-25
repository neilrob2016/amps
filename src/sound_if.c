/*** Interface between main process and child ***/

#include "globals.h"

#define LIMIT_TO_UCHAR(V) \
	if (V < 0) V = 0; \
	else \
	if (V > MAX_UCHAR) V = MAX_UCHAR;

static short prev_freq = 0;

static void setEffectsSeqString();

/******************************* FUNCTIONS *********************************/

/* Tell child to pause and wait. This is to stop race conditions if the 
   sound process tries to work from a partially loaded patch when its being
   loaded off disk or reset by the events recorder */
void pauseSoundProcess()
{
	if (shm->child_status != STAT_STOPPED)
	{
		shm->child_status = STAT_PAUSE;
		while(shm->child_status != STAT_PAUSED);
	}
}




void restartSoundProcess()
{
	if (shm->child_status != STAT_STOPPED) shm->child_status = STAT_RUN;
}




void playOn(int call_basic)
{
	int run_event = (call_basic && (!shm->play || shm->freq != prev_freq));

	/* Set before we call BASIC so it can undo it if it wants */
	shm->play = 1;

	if (run_event)
	{
		runEventSection(
			SECTION_PLAY,
			shm->freq_mode == FREQ_NOTES ? shm->note : -1,
			shm->freq,NULL);
	}
}




void playOff(int call_basic)
{
	shm->play = 0;

	if (call_basic)
	{
		runEventSection(
			SECTION_RELEASE,
			shm->freq_mode == FREQ_NOTES ? shm->note : -1,
			shm->freq,NULL);
	}
}




/*** Set the note for the child process ***/
void setNote(int note)
{
	if (note < 0)
		shm->note = 0;
	else if (note >= NUM_NOTES) 
		shm->note = NUM_NOTES - 1;
	else
		shm->note = (u_char)note;
}




/*** Sets the correct note scale array. Used by both parent and child ***/
void setNoteFreqArray()
{
	note_freq = note_freq_scale[shm->note_scale];
}




/*** Set the note based on the keyboard key pressed. '-' and '=' and the
     11th and 12th keys which should work for UK and US keyboards. ***/
void setNoteByKey(char key)
{
	int note;

	switch(key)
	{
	case '0':
		note = (int)params.key_start_note + 9;
		break;

	case '-':
		note = (int)params.key_start_note + 10;
		break;

	case '=':
		note = (int)params.key_start_note + 11;
		break;

	default:
		note = (int)params.key_start_note + (int)key - '1';
	}

	if (note < 0 || note >= NUM_NOTES) return;

	setNoteFreqArray();

	/* Need to set both because child process only looks at the note if 
	   its arpeggiating or chording */
	setNote(note);
	setFreq(note_freq[note]);
	playOn(1);
}




/*** Play the frequency (note or continous) based on mouse x position */
void setFreqByX(short x, int mouse_button)
{
	short freq;
	
	/* If we're playing a note then update array */
	if (mouse_button == LEFT_MBUTTON) setNoteFreqArray();

	if (x < 0) x = 0;

	/* Update freq based on freq mode */
	if (shm->freq_mode == FREQ_NOTES)
		setNoteAndFreq((double)x / key_spacing);
	else
	{
		/* Get frequency direct from x for both FREQ_CONT and
		   FREQ_STEPPED */
		freq = (short)(x * freq_range_mult);
		if (freq < 0) freq = 0;

		/* Set note to one nearest the frequency. This doesn't actually
		   change the freq, just useful for BASIC getting SYS:NOTE */
		setNote(findNote(freq));

		/* Modify frequency to that of the nearest note if stepped */
		if (shm->freq_mode == FREQ_STEPPED) freq = note_freq[shm->note];

		setFreq(freq);
	}
	if (mouse_button == LEFT_MBUTTON) playOn(1);
}




/*** Get the frequency based on the note */
void setNoteAndFreq(int note)
{
	setNote(note);
	setFreq(note_freq[shm->note]);
}




/*** Actually set it ***/
void setFreq(short freq)
{
	if (freq < 0) freq = 0;

	/* Only update if non zero so last played freq remains on screen */
	if (freq)
	{
		if (shm->sound == SND_SAMPLE)
			sprintf(sample_speed_text,"x%.2f",1 / SAMPLE_INC());
		else
			freq_text_len = sprintf(freq_text,"%d",freq);

		freq_col = RED * ((double)freq / 3000);
		if (freq_col > RED) freq_col = RED;
	}
	prev_freq = shm->freq;
	shm->freq = freq;
}




void setFilter(int val, int call_basic)
{
	int run_event;

	LIMIT_TO_UCHAR(val);

	/* Run event after setting value so BASIC can change it and so it
	   matches a get("SYS:FILTER") call */
	run_event = (call_basic && val != shm->filter_val);
	shm->filter_val = (u_char)val;
	if (run_event) runEventSection(SECTION_FILTER,val,0,NULL);
}




/*** Only change the filter value if below the filter line. This means the user
     can set the filter to a given value and play notes above the filter line
     with that exact same value. Also it means programs can dynamically change
     the value while the user plays notes ***/
void setFilterValueByY(int y)
{
	if (y >= FILTER_Y)
		setFilter((int)round((double)(y - FILTER_Y) * FILTER_MULT),1);
}




void setMainOsc(u_char snd)
{
	message("Main oscillator = %s",sound_name[snd]);
	shm->sound = (u_char)snd;

	/* Drag sub oscillator sound type with us */
	if (params.subs_follow_main) 
	{
		if (snd >= NUM_SUB_TYPES) snd = SND_OFF;

		/* Only update if not off */
		if (shm->sub1_sound) shm->sub1_sound = snd;
		if (shm->sub2_sound) shm->sub2_sound = snd;
	}
}




void incField(u_char *field, int by)
{
	int val = (int)*field + by;
	LIMIT_TO_UCHAR(val);
	*field = (u_char)val;
}




/*** Set the field limited by the max amount ***/
void setField(u_char *field, int val, int max)
{
	if (val < 0)
		*field = (u_char)(max + 1 - (-val % (max + 1)));
	else
		*field = (u_char)(val % (max + 1));
}




/*** Set the notes scale ***/
int setScale(char *strval, int scale)
{
	if (strval)
	{
		if ((scale = getScaleFromName(strval)) == -1) return 0;
	}
	else if (scale < 0) 
		scale = NUM_SCALES - 1;
	else if (scale >= NUM_SCALES)
		scale %= NUM_SCALES;

	shm->note_scale = (u_char)scale;
	setNoteFreqArray();

	runEventSection(
		SECTION_SCALE,shm->note_scale,0,scale_name[shm->note_scale]);
	return 1;
}




/*** Switch between note scale types to use (for now) C or C sharp. Change for 
     parent and child since they both use the note_freq array. Also set
     frequency since the given note value can have a different frequency
     depending on scale but only really useful if we have hold note set ***/
void cycleScale(int mbutton)
{
	setScale(NULL,shm->note_scale + (mbutton == WHEEL_UP ? 1 : -1));
	setFreq(note_freq[shm->note]);
}




/*** Cycle through preset values. Left mouse button cycles down, right cycles
     up, mid switches it back to OFF (which is always value 0). Wheel does
     up/down in the logical fashion ***/
void cycleField(u_char *field, u_char max, int mouse_button)
{
	switch(mouse_button)
	{
	case WHEEL_DOWN:
	case LEFT_MBUTTON:
		*field = (*field + 1) % max;
		break;

	case CENTRE_MBUTTON:
		*field = 0;
		break;

	case WHEEL_UP:
	case RIGHT_MBUTTON:
		--*field;
		if (*field >= max) *field = max - 1;
		break;

	default:
		printf("WARNING: Mouse button %d not used\n",mouse_button);
	}
}




int setOrCycleChord(int mouse_button, char *strval, int val)
{
	if (mouse_button == -1)
	{
		if (strval)
		{
			for(val=0;val < NUM_CHORDS;++val)
				if (!strcasecmp(strval,chord_name[val])) break;
		}
		if (val < 0 || val >= NUM_CHORDS) return 0;
		setField(&shm->chord,val,NUM_CHORDS);
	}
	else cycleField(&shm->chord,NUM_CHORDS,mouse_button);

	message("Chord = %s",chord_name[shm->chord]);
	runEventSection(
		SECTION_BUTTON,BUT_CHORD,shm->chord,chord_name[shm->chord]);
	return 1;
}




int setOrCycleARP(int mouse_button, char *strval, int val)
{
	/* Temporarily set to something so child process doesn't mod zero */
	shm->arp_mod = 1;

	if (mouse_button == -1)
	{
		if (strval)
		{
			for(val=0;val < NUM_ARP_SEQS;++val)
			{
				if (!strcasecmp(strval,arp_seq_name[val]))
					break;	
			}
		}
		if (val < 0 || val >= NUM_ARP_SEQS) return 0;
		setField(&shm->arp_seq,val,NUM_ARP_SEQS);
	}
	else cycleField(&shm->arp_seq,NUM_ARP_SEQS,mouse_button);

	if (shm->arp_seq == ARP_2) shm->arp_mod = 2;
	else if (shm->arp_seq <= ARP_3_DOWN_B) shm->arp_mod = 3;
	else if (shm->arp_seq <= ARP_4_DOWN_INTERLEAVED) shm->arp_mod = 4;
	else if (shm->arp_seq == ARP_4_UP_DOWN) shm->arp_mod = 6;
	else if (shm->arp_seq <= ARP_8_DOWN) shm->arp_mod = 8;
	else shm->arp_mod = 14;

	message("ARP seq = %s",arp_seq_name[shm->arp_seq]);
	runEventSection(
		SECTION_BUTTON,
		BUT_ARP_SEQ,shm->arp_seq,arp_seq_name[shm->arp_seq]);
	return 1;
}




int setOrCyclePhasingMode(int mouse_button, char *strval, int val)
{
	if (mouse_button == -1)
	{
		if (strval)
		{
			for(val=0;val < NUM_PHASING_MODES;++val)
				if (!strcasecmp(strval,phasing_mode[val])) break;
		}
		if (val < 0 || val >= NUM_PHASING_MODES) return 0;
		setField(&shm->phasing_mode,val,NUM_PHASING_MODES);
	}
	else cycleField(&shm->phasing_mode,NUM_PHASING_MODES,mouse_button);

	message("Phasing mode = %s",phasing_mode[shm->phasing_mode]);
	runEventSection(
		SECTION_BUTTON,
		BUT_PHASING_MODE,
		shm->phasing_mode,phasing_mode[shm->phasing_mode]);
	return 1;
}




int setOrCycleFreqMode(int mouse_button, char *strval, int val)
{
	if (mouse_button == -1)
	{
		if (strval)
		{
			for(val=0;val < NUM_FREQ_MODES;++val)
				if (!strcasecmp(strval,freq_mode[val])) break;
		}
		if (val < 0 || val >= NUM_FREQ_MODES) return 0;
		setField(&shm->freq_mode,val,NUM_FREQ_MODES);
	}
	else cycleField(&shm->freq_mode,NUM_FREQ_MODES,mouse_button);

	message("Frequency mode = %s",freq_mode[shm->freq_mode]);

	switch(shm->freq_mode)
	{
	case FREQ_NOTES:
	case FREQ_STEPPED:
		if (prev_note_scale != -1) setScale(NULL,prev_note_scale);
		prev_note_scale = -1;
		break;

	case FREQ_CONT:
		/* Set to scale C when not using notes */
		prev_note_scale = shm->note_scale;
		setScale(NULL,SCALE_C);
		break;

	default:
		assert(0);
	}

	runEventSection(
		SECTION_BUTTON,
		BUT_FREQ_MODE,shm->freq_mode,freq_mode[shm->freq_mode]);
	return 1;
}




int setOrCycleRingModRange(int mouse_button, char *strval, int val)
{
	if (mouse_button == -1)
	{
		if (strval)
		{
			for(val=0;val < NUM_RING_RANGES;++val)
				if (!strcasecmp(strval,ring_range[val])) break;
		}
		if (val < 0 || val >= NUM_RING_RANGES) return 0;
		setField(&shm->ring_range,val,NUM_RING_RANGES);
	}
	else cycleField(&shm->ring_range,NUM_RING_RANGES,mouse_button);

	if (shm->ring_range)
	{
		message("Ring range = %s (%d -> %d Hz)",
			ring_range[shm->ring_range],
			getRingModFreq(0),getRingModFreq(MAX_UCHAR));
	}
	else message("Ring range = OFF");

	runEventSection(
		SECTION_BUTTON,
		BUT_RING_RANGE,shm->ring_range,ring_range[shm->ring_range]);
	return 1;
}




int setOrCycleRingModMode(int mouse_button, char *strval, int val)
{
	if (mouse_button == -1)
	{
		if (strval)
		{
			for(val=0;val < NUM_RING_MODES;++val)
				if (!strcasecmp(strval,ring_mode[val])) break;
		}
		if (val < 0 || val >= NUM_RING_MODES) return 0;
		setField(&shm->ring_mode,val,NUM_RING_MODES);
	}
	else cycleField(&shm->ring_mode,NUM_RING_MODES,mouse_button);

	message("Ring mode = %s",ring_mode[shm->ring_mode]);
	runEventSection(
		SECTION_BUTTON,
		BUT_RING_MODE,shm->ring_mode,ring_mode[shm->ring_mode]);
	return 1;
}




/*** This will also update the ring range if the frequency is outside the
     freq range of the current one ***/
void setRingModFreq(int freq)
{
	int range;

	if (freq < 0) freq = 0;
	else if (freq > MAX_RING_FREQ) freq = MAX_RING_FREQ;

	if (freq)
	{
		range = (freq / NUM_UCHARS) + 1;
		shm->ring_range = (u_char)range;
	}
	else shm->ring_range = RING_OFF;

	shm->ring_freq = freq % NUM_UCHARS;
	message("Ring freq = %d",shm->ring_freq);
}




void cycleMainOsc()
{
	u_char snd;
	u_char prev_snd;

	prev_snd = shm->sound;

	snd = (shm->sound + 1) % NUM_SND_TYPES;
	if (!snd || (snd == SND_SAMPLE && !do_sampling)) snd = SND_SINE_FM;
	setMainOsc(snd);

	button[prev_snd-1].pressed = 0;
	button[shm->sound-1].pressed = BUTTON_PRESS;
}




int setOrCycleSubOsc(int osc, int mouse_button, char *strval, int val)
{
	u_char *subsnd;

	assert(osc == 1 || osc == 2);
	subsnd = (osc == 1 ? &shm->sub1_sound : &shm->sub2_sound);

	/* Only -1 if called by BASIC */
	if (mouse_button == -1)
	{
		if (strval) val = getSoundNumberFromName(strval);	
		if (val < SND_OFF || val >= SND_SAMPLE) return 0;
		setField(subsnd,val,NUM_SUB_TYPES);
	}
	else cycleField(subsnd,NUM_SUB_TYPES,mouse_button);

	message("Sub oscillator %d = %s",osc,sound_name[*subsnd]);
	runEventSection(
		SECTION_BUTTON,
		osc == 1 ? BUT_SUB1_SOUND : BUT_SUB2_SOUND,
		*subsnd,sound_name[*subsnd]);
	return 1;
}




int setOrCycleResMode(int mouse_button, char *strval, int val)
{
	if (mouse_button == -1)
	{
		if (strval)
		{
			for(val=0;val < NUM_RES_MODES;++val)
				if (!strcasecmp(strval,resonance_mode[val])) break;
		}
		if (val < 0 || val >= NUM_RES_MODES) return 0;
		setField(&shm->res_mode,val,NUM_RES_MODES);
	}
	else cycleField(&shm->res_mode,NUM_RES_MODES,mouse_button);

	message("Resonance mode = %s",resonance_mode[shm->res_mode]);
	runEventSection(
		SECTION_BUTTON,
		BUT_RES_MODE,shm->res_mode,resonance_mode[shm->res_mode]);
	return 1;
}




void setOrCycleEffectsSeq(int mouse_button, int val)
{
	char *str;
	char *e1;
	char *e2;
	int p1;
	int p2;

	if (mouse_button == -1)
		setField(&params.effects_swap,val,NUM_SWAP_EFFECTS);
	else
		cycleField(&params.effects_swap,NUM_SWAP_EFFECTS,mouse_button);

	p1 = effects_pos[params.effects_swap].pos1;
	p2 = effects_pos[params.effects_swap].pos2;
	e1 = effects_long_name[shm->effects_seq[p1]];
	e2 = effects_long_name[shm->effects_seq[p2]];

	++p1;
	message("Effects to swap = %d (%s) <-> %d (%s)",p1,e1,p2+1,e2);
	str = (char *)malloc(strlen(e1) + strlen(e2) + 6); 
	sprintf(str,"%s -> %s",e1,e2);
	runEventSection(SECTION_BUTTON,BUT_EFFECTS_TO_SWAP,p1,str);
	free(str);
}




/*** Parse a string in the form AA-BB-CC... containing the effects short
     names and set the correct sequence. Only called by funcSet() for now ***/
int setEffectsSeqByString(char *seq)
{
	u_char effects_seq[NUM_SWAP_EFFECTS];
	int effects_used[NUM_SWAP_EFFECTS];
	int len;
	int efnum;
	int pos;
	char *s;
	char *e;

	if (!seq) return ERR_INVALID_SET_VALUE;

	/* Check length. Must have all the effects */
	if ((len = strlen(seq)) < NUM_SWAP_EFFECTS * 2 - 1)
		return ERR_INVALID_EFFECTS_SEQ;

	/* Check basic syntax. Need AA-BB-CC..NN */
	for(s=seq+2,e=seq+len-3;s <= e;s+=3)
		if (*s != '-') return ERR_INVALID_EFFECTS_SEQ;

	/* Check effect names */
	bzero(effects_used,sizeof(effects_used));
	e = seq + len;
	for(s=seq,pos=0;s < e;s+=3,++pos)
	{
		/* Find effect name in list */
		for(efnum=0;efnum < NUM_SWAP_EFFECTS;++efnum)
			if (!strncmp(s,effects_short_name[efnum],2)) break;
		if (efnum == NUM_SWAP_EFFECTS) return ERR_INVALID_EFFECT_NAME;
		if (effects_used[efnum]) return ERR_DUPLICATE_EFFECT_NAME;

		effects_used[efnum] = 1;
		effects_seq[pos] = efnum;
	}
	memcpy(shm->effects_seq,effects_seq,sizeof(effects_seq));
	setEffectsSeqString();
	return OK;
}




void swapEffects()
{
	char *str;
	char *e1;
	char *e2;
	u_char tmp;
	int p1;
	int p2;

	p1 = effects_pos[params.effects_swap].pos1;
	p2 = effects_pos[params.effects_swap].pos2;

	/* Strictly speaking I should set STAT_PAUSE here for child in case
	   it tries to run the same effect twice but it'll be unnoticable so
	   I won't bother */
	tmp = shm->effects_seq[p2];
	shm->effects_seq[p2] = shm->effects_seq[p1];
	shm->effects_seq[p1] = tmp;
	setEffectsSeqString();

	e1 = effects_long_name[shm->effects_seq[p1]];
	e2 = effects_long_name[shm->effects_seq[p2]];

	++p1;
	str = (char *)malloc(strlen(e1) + strlen(e2) + 40);
	sprintf(str,"Effects swapped: %d = %s, %d = %s",p1,e1,p2+1,e2);
	message(str);
	runEventSection(SECTION_BUTTON,BUT_SWAP_EFFECTS,p1,str);
	free(str);
}




void resetEffectsSeq(int press)
{
	int i;
	for(i=0;i < NUM_SWAP_EFFECTS;++i) shm->effects_seq[i] = i;
	setEffectsSeqString();

	/* If GUI button or key pressed do below */
	if (press)
	{
		message("Effects sequence reset");
		runEventSection(SECTION_BUTTON,BUT_RESET_EFFECTS_SEQ,1,NULL);
	}
}




/*** Flip the field and button setting ***/
void invertField(u_char *field, enum en_button but)
{
	*field = !*field;
	button[but].pressed = !button[but].pressed;
}




/*** Set the field valued based on the angle of the dial ***/
void setFieldToDialAngle(u_char *field, enum en_button but)
{
	*field = (u_char)(button[but].angle - DIAL_ANGLE_MIN);

	/* Some fields can't be zero */
	if (!*field)
	{
		switch(but)
		{
			case BUT_ANALYSER_RANGE:
			case BUT_RES_LEVEL:
			case BUT_RES_FREQ:
			case BUT_PHASER_LOW_OFF_MULT:
			case BUT_GLIDE_VELOCITY:
				button[but].angle = DIAL_ANGLE_MIN + 1;
				*field = 1;
				break;

			case BUT_MAX_FREQ:
				button[but].angle = DIAL_ANGLE_MIN + 2;
				*field = 2;
				break;

			default:
				break;
		}
	}
}




/*** Set the keys count and frequency range ***/
void setByRange()
{
	key_cnt = params.freq_range;
	if (key_cnt < 2) key_cnt = 2;
	else if (key_cnt > NUM_NOTES) key_cnt = NUM_NOTES;

	key_spacing = (double)WIN_WIDTH / key_cnt;
	key_char_add = key_spacing / 2 - CHAR_SIZE / 2;

	freq_range_mult = (double)params.freq_range / FREQ_RANGE_DIAL_INIT;
}




/*** Do a binary chop search to find the note frequency nearest the one given
     as the argument ***/
int findNote(int freq)
{
	int pos;
	int next_pos;
	int add;

	pos = add = NUM_NOTES / 2;

	for(;;pos+=add)
	{
		if (note_freq[pos] < freq)
		{
			next_pos = pos+1;
			if (next_pos == NUM_NOTES) return pos;
			if (note_freq[next_pos] > freq) break;
			add = abs(add);
			if (add > 1) add /= 2;
		}
		else if (note_freq[pos] > freq)
		{
			next_pos = pos - 1;
			if (next_pos < 0) return pos;
			if (note_freq[next_pos] < freq) break;
			add = -abs(add);
			if (add < -1) add /= 2;
		}
		else return pos;
	}
	if (abs(freq - note_freq[pos]) < abs(freq - note_freq[next_pos]))
		return pos;
	else
		return next_pos;
}




/*** Make sure shared memory values are sane and reset buttons etc. Used
     after loading a patch, in the recorder and in randomiseSettings() ***/
void normalisePatch(int do_window)
{
	int i;
	int j;

	shm->freq = 0;

	/* Convert relevant to host byte order then call resize which will 
	   cause a ConfigureNotify event to be sent so don't need to update
	   other vars here */
	if (do_window)
	{
		shm->win_width = ntohs(shm->win_width);
		params.win_height = ntohs(params.win_height);

		/* Limit to display size. The X window manager will probably
		   do this anyway but justwin_width etc. to be sure... */
		if (!shm->win_width) shm->win_width = win_width;
		else if (shm->win_width > display_width)
		{
			printf("WARNING: Patch window width %d was greater than display width %d\n",
				shm->win_width,display_width);
			shm->win_width = display_width;
		}
		if (!params.win_height) params.win_height = win_height;
		else if (params.win_height > display_height)
		{
			printf("WARNING: Patch window height %d was greater than display height %d\n",
				params.win_height,display_height);
			params.win_height = display_height;
		}

		// This will cause us to receive a ConfigureNotify which will
		// then finish off setting everything.
		XResizeWindow(display,win,shm->win_width,params.win_height);
	}

	/* Make sure various fields are not out of range */
	shm->sound %= NUM_SND_TYPES;
	if (shm->sound == SND_OFF) shm->sound = SND_SINE_FM;

	/* Can't have sub oscillators set to sample */
	shm->sub1_sound %= SND_SAMPLE;
	shm->sub2_sound %= SND_SAMPLE;

	shm->chord %= NUM_CHORDS;
	shm->note_scale %= NUM_SCALES;
	shm->freq_mode %= NUM_FREQ_MODES;
	shm->phasing_mode %= NUM_PHASING_MODES;
	shm->res_mode %= NUM_RES_MODES;
	shm->ring_range %= NUM_RING_RANGES;
	shm->ring_mode %= NUM_RING_MODES;
	if (shm->arp_mod)
		shm->arp_seq %= NUM_ARP_SEQS;
	else
		shm->arp_seq = ARP_OFF;

	/* Can't allow zero or all pre 1.6.0 patches will be silent */
	if (!shm->compress_start) shm->compress_start = MAX_UCHAR;
	if (!shm->compress_exponent) shm->compress_exponent = MAX_UCHAR;

	if (!shm->echo_stretch) shm->echo_stretch = MAX_CHAR;

	/* Check effects seq not out of range or duplicated */
	for(i=0;i < NUM_SWAP_EFFECTS;++i)
	{
		if (shm->effects_seq[i] >= NUM_SWAP_EFFECTS)
		{
			resetEffectsSeq(0);
			break;
		}
		for(j=i+1;j < NUM_SWAP_EFFECTS;++j)
		{
			if (shm->effects_seq[i] == shm->effects_seq[j])
			{
				resetEffectsSeq(0);
				i = NUM_SWAP_EFFECTS;
				break;
			}
		}
	}

	playOff(0);
	setNote(0);
	setByRange();
	resetButtons(0,1);
	updateClickButtons();
	updateMainOscButtons();

	/* Set on/off buttons */
	setButtonByParam(BUT_SPECTRUM_ANALYSER,params.show_analyser);
	setButtonByParam(BUT_BUFFER_RESET,shm->buffer_reset);
	setButtonByParam(BUT_HIGHPASS_FILTER,shm->highpass_filter);
	setButtonByParam(BUT_ECHO_HIGHPASS_FILTER,shm->echo_highpass_filter);
	setButtonByParam(BUT_HOLD_NOTE,params.hold_note);
	setButtonByParam(BUT_ECHO_INVERT,shm->echo_invert);
	setButtonByParam(BUT_SUB1_NOTE_OFFSET,shm->sub1_note_offset);
	setButtonByParam(BUT_SUB2_NOTE_OFFSET,shm->sub2_note_offset);
	setButtonByParam(BUT_SUBS_FOLLOW,params.subs_follow_main);

	setNoteFreqArray();
}




/*** Create a random patch ***/
void randomiseSettings()
{
	u_char * ptr;
	u_char tmp;
	int i;
	int j;

	message("Randomising...");

	pauseSoundProcess();

	/* Have to do bit fields manually */
	shm->buffer_reset = random() % 2;
	shm->highpass_filter = random() % 2;
	shm->echo_clear = 0;
	shm->echo_invert = random() % 2;
	shm->echo_highpass_filter = random() % 2;
	shm->sub1_note_offset = random() % 2;
	shm->sub2_note_offset = random() % 2;

	/* Don't the main oscillator set to SAMPLE */
	shm->sound = SND_SINE_FM + random() % 6;

	/* Just go through the rest with a pointer. Easier that doing each 
	   field individually. Then normalise */
	for(ptr = RANDOMISE_START;ptr <= RANDOMISE_END;++ptr) 
		*ptr = random() % NUM_UCHARS;

	/* Set attack to a low number so result of randomise is immediately
	   obvious */
	shm->attack = random() % 20;

	/* If phaser is in FM FLANGER mode then offset (layers) and sweep need 
	   to be low to get a decent effect */
	if ((shm->phasing_mode % NUM_PHASING_MODES) == PHASING_FM_FLANGER)
	{
		shm->phasing_offset = random() % 5;
		shm->phasing_sweep = random() % 10;
	}

	/* Randomly swap about the effects seq */
	for(i=0;i < NUM_SWAP_EFFECTS;++i)
	{
		do
		{
			j = random() % NUM_SWAP_EFFECTS;
		}
		while(j == i);
		tmp = shm->effects_seq[i];
		shm->effects_seq[i] = shm->effects_seq[j];
		shm->effects_seq[j] = tmp;
	}
	setEffectsSeqString();
	normalisePatch(0);
	restartSoundProcess();
}


/**************************** Get complex values ***************************/

/*** Easier to put the following equations in functions ***/
double getFMOffset()
{
	return (double)(shm->fm_offset - MAX_CHAR) / FM_OFFSET_DIV;
}




double getPhaserLOM()
{
	return (double)shm->phaser_low_off_mult / MAX_UCHAR * PHASER_LOW_MULT_RANGE;
}




int getAnalyserRange()
{
	return (int)params.analyser_range * SAMPLES_PER_SEC;
}




int getMaxFrequency()
{
	if (shm->freq_mode == FREQ_NOTES) return key_cnt;
	return freq_range_mult * WIN_WIDTH;
}




double getResonanceDamping()
{
	return (double)shm->res_damping / MAX_UCHAR;
}




double getGainCompressionExponent()
{
	return (double)shm->compress_exponent / MAX_UCHAR;
}




/*** Returns 0.8 -> 1.2 ***/
double getEchoStretch()
{
	double d = 0.8 + 0.4 * (double)shm->echo_stretch / MAX_UCHAR;

	/* Hack to prevent it jumping from 0.999 to 1.001. User wants to see
	   1 as neutral value */
	return (d >= 0.999 && d < 1) ? 1 : d;
}




int getRingModFreq(int dial_freq)
{
	return (shm->ring_range == RING_OFF) ? 0 : 
	        NUM_UCHARS * (shm->ring_range - 1) + dial_freq;
}


/**************************** Support functions ***************************/

/*** Set the current effects sequence ***/
void setEffectsSeqString()
{
	int i;

	strcpy(effects_seq_str,effects_short_name[shm->effects_seq[0]]);
	for(i=1;i < NUM_SWAP_EFFECTS;++i)
	{
		strcat(effects_seq_str,"-");
		strcat(effects_seq_str,effects_short_name[shm->effects_seq[i]]);
	}	
}
