/*** Sound process main loop and support functions plus writing the sound
     buffer to the sound device and also doing echo and glide ***/

#include "globals.h"

#define FM_OVERFLOW_CHAN (NUM_CHANS - 1)

#define LFO_DEGREES(X) ((double)(X) / MAX_UCHAR * 120)
#define LFO2_DEGREES(X) ((double)(X) / MAX_UCHAR * 60)

#define MAX_WRITE_ERRORS 3

static int arp_sequences[NUM_ARP_SEQS][14] = 
{
	{ 0 },
	{ 0,1 },
	{ 0,1,2 },
	{ 2,1,0 },
	{ 0,2,3 },
	{ 3,2,0 },

	{ 0,1,2,1 },
	{ 0,2,3,2 },
	{ 0,1,2,3 },
	{ 3,2,1,0 },
	{ 0,2,1,3 },
	{ 3,1,2,0 },

	{ 0,1,2,3,2,1 },
	{ 0,1,2,3,4,5,6,7 },
	{ 7,6,5,4,3,2,1,0 },
	{ 0,1,2,3,4,5,6,7,6,5,4,3,2,1 }
};


/*** Module specific vars ***/

static short g_freq;
static short prev_freq;
static double glide_freq_mult;
static int do_glide_reset;
static int do_echo_clear;
static int echobuff_write_pos;
static int echobuff_use_size;
static int check_cnt;
static int played_out;
static u_char prev_play;

static u_char do_decay;
static u_char attack_cnt;
static u_char decay_cnt;
static int    arp_delay;
static u_char arp_note_pos;

/*** Forward declarations ***/
void soundReset();
void resetSoundChannels();
void resetTransients();
void resetOSXbuffers();
void setGlide();
void silenceAfterFirstZero();

void createSound();
void arpeggiate(int *note);
void doAliasing(short volume);
void doReflection(short volume);
void doPhasing(short unused);
void doFilter(short unused);
void doResonance(short unused);
void doDistortion(short unused);
void doRingModulation(short unused);
void generateWaveform(int bank, short freq, int volume, int reset);
double getSubOscVolume(int main_volume, u_char shm_sub_vol);
double getSubOscFreq(
	u_char shm_sub_offset,
	u_char shm_sub_note_offset,
	short freq,
	short freq_add,
	int main_note);

void clearEchoBuffer();
void checkEcho();
void doEcho();
void stretchEchoBuffer();

void writeSoundToDevice();

/**************************** CHILD STARTS HERE ******************************/

void soundLoop()
{
	soundReset();

	/* Tell parent we've started */
	shm->child_status = STAT_RUN;

	/* Main child process loop */
	while(1)
	{
		checkEcho();

		/* Loop while we have a sound to play */
		while(1)
		{
			if (shm->do_reset) 
			{
				soundReset();
				break;
			}

			/* Pause and check loop */
			while(1)
			{
				/* If we've been reparented then die */
				check_cnt = (check_cnt + 1) % 20;
				if (!check_cnt && getppid() == 1)
				{
					puts("SOUND: Parent process dead - exiting");
					exit(1);
				}
				if (shm->child_status != STAT_PAUSED) break;
				usleep(USEC_LOOP_DELAY);
			} 

			/* See if parent has asked us to pause for load/save */
			if (shm->child_status == STAT_PAUSE)
			{
				shm->child_status = STAT_PAUSED;
				continue;
			}

			setNoteFreqArray();

			if (shm->echo_clear) 
			{
				clearEchoBuffer();
				shm->echo_clear = 0;
			}
			else echobuff_use_size = SNDBUFF_SIZE * shm->echo_len;

			g_freq = shm->freq;
			
			/* If we've started a new note or replayed the same
			   note then reset glide */
			if (shm->play && shm->play != prev_play)
				do_glide_reset = 1;

			prev_play = shm->play;

			/* Note being played */
			if (shm->play)
			{
				played_out = 0;

				/* See if user has changed frequency */
				if (g_freq != prev_freq)
				{
					resetTransients();
					prev_freq = g_freq;
					do_glide_reset = 1;

					/* Set to zero so decay < shm->decay 
					   true if shm->decay non zero and 
					   decay will occur */
					decay_cnt = 0;
				}
				/* Continuous frequency */
				else decay_cnt = 0;
			}
			/* No note being played. If we have a decay then
			   activate it. */
			else if (g_freq && decay_cnt < shm->decay)
			{
				do_decay = 1;
				/* Make sure createSound() doesn't continue 
				   with attack */
				attack_cnt = shm->attack;
			}
			else
			{
				/* A finished decay is at zero amplitude so 
				   just reset */
				if (do_decay)
				{
					resetTransients();
					resetSoundChannels();
					played_out = 1;
				}
				/* Create one last buffer of data but at first 
				   zero crossing point and zero the rest of 
				   it */
				else if (!played_out)
				{
					createSound();
					silenceAfterFirstZero();
					doEcho();
					writeSoundToDevice();
					resetTransients();
					resetSoundChannels();
					played_out = 1;
				}
				break;
			}

			/* Work out glide params */
			if (shm->glide_distance != MAX_CHAR)
				setGlide();
			else
				glide_freq_mult = 1;

			createSound();
			doEcho();
			writeSoundToDevice();
			checkEcho();
		} 

		if (shm->echo_len)
		{
			/* Let any echos play out. Reset sound buffer so the 
			   echos fade away instead of building on each other */
			resetSoundBuffer();
			doEcho();
			writeSoundToDevice();
		}
		/* Don't kill the CPU if nothing to do */
		else
		{
#if SOUND==OSX
			/* If we don't do this we get a blip at the end of play
			   where OS/X continues to play old data */
			resetOSXbuffers();
#endif
			usleep(USEC_LOOP_DELAY);
		}
	}
}




void soundReset()
{
	prev_freq = 0;
	do_echo_clear = 1;
	glide_freq_mult = 1;
	check_cnt = 0;
	played_out = 0;
	prev_play = shm->play;
	noise_prev_res = 0;
	vibrato_sweep_ang = 0;
	ring_sin_ang = 0;
	ring_sin_inc = 0;
	ring_sq_do_mult = 0;
	ring_sq_period = 0;
	ring_sq_next_edge = 0;
	ring_sq_do_mult = 0;
	ring_tri_mult = 0;
	ring_tri_inc = 0;
	ring_tri_min = 0;
	ring_saw_mult = 0;
	ring_saw_inc = 0;
	ring_saw_min = 0;
	ring_sine_prev_freq = 0;
	ring_sqr_prev_freq = 0;
	ring_saw_prev_freq = 0;
	ring_tri_prev_freq = 0;
	ring_tri_prev_dist = 0;
	ring_saw_prev_dist = 0;
	bzero(phasingbuff,sizeof(phasingbuff));

	resetSoundChannels();
	clearEchoBuffer();
	resetSoundBuffer();
	resetTransients();
	resonate(0,0,0,1);
#if SOUND==OSX
	resetOSXbuffers();
#endif
	shm->do_reset = 0;
}




void resetSoundChannels()
{
	bzero(sin_ang,sizeof(sin_ang));
	bzero(sin_ang,sizeof(sin_ang));
	bzero(tri_val,sizeof(tri_val));
	bzero(tri_inc,sizeof(tri_inc));
	bzero(sq_vol,sizeof(sq_vol));
	bzero(sq_cnt,sizeof(sq_cnt));
	bzero(sq_next_edge,sizeof(sq_next_edge));
	bzero(saw_val,sizeof(saw_val));
	bzero(saw_flatten_cnt,sizeof(saw_flatten_cnt));
	bzero(saw_flatten_step_cnt,sizeof(saw_flatten_step_cnt));
	bzero(sin_fm_ang1,sizeof(sin_fm_ang1));
	bzero(sin_fm_ang2,sizeof(sin_fm_ang2));
	bzero(wav_pos,sizeof(wav_pos));
}




void resetSoundBuffer()
{
	bzero(sndbuff,SNDBUFF_BYTES);
}




/*** Some vars need to be reset whenever the frequency changes or a note is
     no longer playing ***/
void resetTransients()
{
	arp_note_pos = 0;
	arp_delay = 0;
	attack_cnt = 0;
	do_decay = 0;
	decay_cnt = shm->decay;
}




#if SOUND==OSX
void resetOSXbuffers()
{
	for(int i=0;i < num_osx_buffers;++i)
		bzero(osx_buffer[i]->mAudioData,SNDBUFF_BYTES);
}
#endif




/*** Sets glide_freq_mult for later functions to use. This goes from -1 -> 1
     hence glide can make the frequency vary from 0 -> 2 * freq with a varying
     speed ***/
void setGlide()
{
	static double mult_add = 0;
	static int cnt = 0;
	static int max_cnt = 0;
	double mult;

	if (do_glide_reset)
	{
		mult = (double)(shm->glide_distance - MAX_CHAR) / MAX_CHAR;

		max_cnt = (MAX_UCHAR - shm->glide_velocity) * 2;
		if (max_cnt < 1) max_cnt = 1;

		mult_add = mult / max_cnt;
		cnt = 0;
		glide_freq_mult = 1;
		do_glide_reset = 0;
	}
	if (++cnt <= max_cnt) glide_freq_mult += mult_add; 
}



/*** When user has finished playing a note we can't just stop at end of
     current buffer as data probably won't be zero so we'll get a clicking
     sound. Set to zero all buffer data from first zero crossing point ***/
void silenceAfterFirstZero()
{
	int p1;

	for(p1=0;p1 < SNDBUFF_SIZE;++p1)
	{
		/* If we've crossed or are at zero then zero reset of buff */
		if (p1 && 
		   (!sndbuff[p1] || SGN(sndbuff[p1]) != SGN(sndbuff[p1-1]))) 
		{
			bzero(sndbuff+p1,(SNDBUFF_SIZE - p1) * sizeof(short));
			return;
		}
	}
}



/****************************** SOUND CREATION ******************************/

/*** Get note/freq required, do volume envelopes, generate waveform then
     add in effects ***/
void createSound()
{
	static void (*effects_func[NUM_SWAP_EFFECTS])(short) = 
	{
		doAliasing,
		doReflection,
		doPhasing,
		doFilter,
		doResonance,
		doDistortion,
		doRingModulation
	};
	/* Update this if chord enum changed */
	static struct 
	{
		int note2_add;
		int note3_add;
	} chord_data[NUM_CHORDS] =
	{
		{ 0,0 },

		/* 2 note chords */
		{ 2,0 },
		{ 3,0 },
		{ 4,0 },
		{ 5,0 },
		{ 7,0 },

		/* 3 note chords */
		{ 1,4 },
		{ 1,5 },
		{ 2,4 },
		{ 2,5 },
		{ 2,7 },
		{ 3,5 },
		{ 3,6 },
		{ 3,7 },
		{ 4,7 },
		{ 5,7 }
	};
	double main_volume;
	double volume;
	short peak_volume;
	short freq_add;
	int note1;
	int note2;
	int note3;
	int i;

	/* Do arpeggiation else get note which is set in
	   setFreq() if notes, arpeggiation or chord are on */
	if (shm->arp_seq != ARP_OFF) 
		arpeggiate(&note1); /* sets g_freq */
	else 
		note1 = shm->note;

	/* Add vibrato. Args are freq, 0 -> 1 mult, 0 -> 120 degs. Can't go
	   up to 180 degs because sin(0) = sin(180) = 0 so you'd not get any
	   vibrato at all */
	g_freq = vibrato(
		g_freq,
		(double)shm->vib_sweep / MAX_UCHAR,
		LFO_DEGREES(shm->vib_lfo));

	/* << 6 only so there's less chance of distortion. Only can have 
	   MAX_SHORT anyway which would be << 7, not 0xFFFF */
	main_volume = ((int)shm->volume << VOLUME_SHIFT);

	/* Do volume envelopes */
	if (attack_cnt < shm->attack)
	{
		++attack_cnt;
		volume = main_volume * ((double)attack_cnt /  shm->attack);
	}
	else if (do_decay && decay_cnt < shm->decay)
	{
		++decay_cnt;
		volume = main_volume * (1 - (double)decay_cnt /  shm->decay);
	}
	else volume = main_volume;

	if (shm->chord == CHORD_OFF) 
		generateWaveform(0,g_freq,volume,shm->buffer_reset);
	else
	{
		/* Generate chord notes */
		if (g_freq != note_freq[note1])
			freq_add = g_freq - note_freq[note1];
		else
			freq_add = 0;

		note2 = note1 + chord_data[shm->chord].note2_add;

		/* Multiply by 0.66 for 2 note chords, 0.5 for 3 */
		volume = round((double)volume * (chord_data[shm->chord].note3_add ? 0.5 : 0.66));

		/* Generate main waveform */
		generateWaveform(0,g_freq,volume,shm->buffer_reset);

		/* Generate 2nd chord note */
		if (note2 >= NUM_NOTES) note2 = NUM_NOTES - 1;
		generateWaveform(1,note_freq[note2]+freq_add,volume,0);

		/* Generate optional 3rd note */
		if (chord_data[shm->chord].note3_add)
		{
			note3 = note1 + chord_data[shm->chord].note3_add;
			if (note3 >= NUM_NOTES) note3 = NUM_NOTES - 1;
			generateWaveform(2,note_freq[note3]+freq_add,volume,0);
		}
	}

	/* After combining waveforms the peak volume will have changed so find
	   out what it is for the effects. */
	peak_volume = 0;
	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		/* Haven't used abs() here for efficiency reasons. Don't want
		   the overhead of 2 unnecessary function calls */
		if (sndbuff[i] > peak_volume) 
			peak_volume = sndbuff[i];
		else if (sndbuff[i] < -peak_volume) 
			peak_volume = -sndbuff[i];
	}

	/* Call the order swappable effects functions */
	for(i=0;i < NUM_SWAP_EFFECTS;++i) 
		(*effects_func[shm->effects_seq[i]])(peak_volume);
}




/*** Play an automated sequence of notes based on the current frequency ***/
void arpeggiate(int *note)
{
	int arp_note;

	arp_note = shm->note + 
		   arp_sequences[shm->arp_seq][arp_note_pos] * shm->arp_spacing;
	if (arp_note < 0) arp_note = 0;
	else
	if (arp_note >= NUM_NOTES) arp_note = NUM_NOTES - 1;

	/* If we're playing notes then use note freq else use related freq */
	if (shm->freq_mode == FREQ_CONT)
		g_freq = note_freq[arp_note] + (g_freq - note_freq[shm->note]);
	else
		g_freq = note_freq[arp_note];

	if (++arp_delay >= shm->arp_delay)
	{
		arp_note_pos = (arp_note_pos + 1) % shm->arp_mod;
		arp_delay = 0;
	}
	*note = arp_note;
}




void doAliasing(short volume)
{
	if (shm->aliasing) alias(shm->aliasing,volume);
}




void doReflection(short volume)
{
	if (shm->reflect_level) 
		reflect(shm->reflect_level,shm->reflect_smoothing,volume);
}




void doPhasing(short unused)
{
	static double sweep_ang = 0;
	int sweep;
	int smooth;

	if (shm->phasing_mode == PHASING_OFF) return;

	/* 1 + sin / 2 because we want to go from 0 -> 1, not -1 to 1 */
	sweep = (int)round((double)shm->phasing_sweep * (1 + SIN(sweep_ang)) / 2);
	smooth = shm->phasing_sweep && (
			shm->sound == SND_SINE || 
			shm->sound == SND_SINE_FM ||
			shm->sound == SND_NOISE ||
			shm->sound == SND_TRIANGLE);

	switch(shm->phasing_mode)
	{
	case PHASING_FLANGER_ADD:
	case PHASING_FLANGER_SUB:
		flanger(
			shm->phasing_mode == PHASING_FLANGER_ADD ? 1 : -1,
			sweep + shm->phasing_offset,0,smooth);
		break;

	case PHASING_DIRTY_ADD:
	case PHASING_DIRTY_SUB:
		flanger(
			shm->phasing_mode == PHASING_DIRTY_ADD ? 1 : -1,
			sweep + shm->phasing_offset,1,smooth);
		break;

	case PHASING_PHASER_ADD:
	case PHASING_PHASER_SUB:
		phaser(
			shm->phasing_mode == PHASING_PHASER_ADD ? 1 : -1,
			shm->phaser_freq_sep,
			sweep + shm->phasing_offset,
			(double)shm->phaser_low_off_mult / MAX_UCHAR * PHASER_LOW_MULT_RANGE,
			smooth);
		break;

	case PHASING_FM_FLANGER:
		fmFlanger(
			shm->phasing_offset,
			(int)round(shm->phasing_sweep * fabs(COS(sweep_ang))));
		break;

	default:
		assert(0);
	}

	if (shm->phasing_lfo)
	{
		/* 120 for same reason as vibrato */
		incAngle(&sweep_ang,LFO_DEGREES(shm->phasing_lfo));
	}
	else sweep_ang = 0;
}




void doFilter(short unused)
{
	static double filter_ang = 0;
	static short prev_buff_end[2] = { 0,0 };
	double level;
	double sub;

	if (!shm->filter_val) return;

	if (shm->filter_lfo)
		incAngle(&filter_ang,LFO_DEGREES(shm->filter_lfo));
	else 
		filter_ang = 0;

	if (shm->highpass_filter)
	{
		/* Linear response to mouse */
		level = (double)shm->filter_val / MAX_UCHAR;
	}
	else
	{
		/* Power of 2 response which works better with low pass */
		sub = MAX_UCHAR - shm->filter_val;
		level = 1 - (double)(sub * sub) / MAX_USHORT;
	}

	/* Add LFO effect which involves sweeping from current filter level down
	   to extent of filter sweep which will be between filter level and
	   zero. If filter sweep is 255 (max) go all the way from val down to
	   zero filtering. If sweep = half (127) then go halfway down etc. */
	if (filter_ang)
	{
		sub = level * (double)shm->filter_sweep / MAX_UCHAR;
 		/* Want 1 -> 0, not 1 -> -1 */
		level -= sub * (1 + COS(filter_ang)) / 2;
	}

	filter(shm->highpass_filter,level,sndbuff,SNDBUFF_SIZE,prev_buff_end);
}




void doResonance(short unused)
{
	switch(shm->res_mode)
	{
	case RES_OFF:
		break;

	case RES_INDEPENDENT_FREQ:
		resonate(
			0.01*shm->res_freq,
			0.01*shm->res_level,
			(double)shm->res_damping / MAX_UCHAR,
			0);
		break;

	case RES_FOLLOW_MAIN_FREQ:
		resonate(
			(double)shm->res_freq / 500000*g_freq,
			0.01*shm->res_level,
			(double)shm->res_damping / MAX_UCHAR,
			0);
		break;

	default:
		assert(0);
	}
}




void doDistortion(short unused)
{
	if (shm->distortion) distort(shm->distortion);
}




void doRingModulation(short unused)
{
	if (shm->ring_range)
	{
		ringModulate(
			shm->ring_mode,
			getRingModFreq(shm->ring_freq),shm->ring_level);
	}
}




/*** Call the appropriate sound generation routines in sound_gen.c to build
     up the waveform in sndbuff[] ***/
void generateWaveform(int bank, short freq, int volume, int reset)
{
	static double sine_cutoff_lfo_ang = 0;
	static double square_width_lfo_ang = 0;
	static double saw_flatten_lfo_ang = 0;
	double lfo_mult;
	double fm_mult1;
	double fm_mult2;
	double fm_offset;
	double square_width;
	double sub_vol;
	double sub_freq;
	double main_freq;
	double saw_flatten_ratio;
	double fm_vol_exp;
	double wav_volume;
	short fm_harm;
	short fm_vol1;
	short fm_vol2;
	short freq_add;
	int note;
	int sine_cutoff_ang;

	bank *= 3;
	note = 0;
	freq_add = 0;
	fm_harm = shm->fm_harm_offset - MAX_CHAR;
	fm_mult1 = (double)shm->fm_mult1 / 10;
	fm_mult2 = (double)shm->fm_mult2 / 10;
	fm_offset = (double)(shm->fm_offset - MAX_CHAR) / FM_OFFSET_DIV;
	fm_vol1 = shm->fm_volume1;
	fm_vol2 = shm->fm_volume2;
	/* 0 -> 255 becomes 1 -> 3.55 */
	fm_vol_exp = (double)shm->fm_wierd / 100 + 1;

	/* If the square width LFO is activated calc the width */
	if (shm->square_width_lfo)
	{
		lfo_mult = fabs(COS(square_width_lfo_ang));
		square_width = ((double)MAX_UCHAR - 
		                (MAX_UCHAR - shm->square_width) * lfo_mult) / 
		                 MAX_UCHAR;
		incAngle(&square_width_lfo_ang,LFO2_DEGREES(shm->square_width_lfo));
	}
	else square_width = (double)shm->square_width / MAX_UCHAR;

	/* Ditto sine cutoff */
	if (shm->sine_cutoff_lfo)
	{
		lfo_mult = fabs(COS(sine_cutoff_lfo_ang));
		sine_cutoff_ang = ((double)MAX_UCHAR - 
		                   (shm->sine_cutoff * lfo_mult)) /
		                   MAX_UCHAR * 180;
		incAngle(&sine_cutoff_lfo_ang,LFO2_DEGREES(shm->sine_cutoff_lfo));
	}

	else sine_cutoff_ang = (double)(MAX_UCHAR - shm->sine_cutoff) / MAX_UCHAR * 180;

	/* Ditto saw flatten ratio */
	if (shm->saw_flatten_lfo)
	{
		lfo_mult = fabs(COS(saw_flatten_lfo_ang));
		saw_flatten_ratio = (double)shm->saw_flatten * lfo_mult / MAX_UCHAR;
		incAngle(&saw_flatten_lfo_ang,LFO2_DEGREES(shm->saw_flatten_lfo));
	}
	else saw_flatten_ratio = (double)shm->saw_flatten / MAX_UCHAR;
	
	/* Add glide effect */
	main_freq = round(glide_freq_mult * freq);

	/* Normalised volume for WAVs so initial volume = 1 */
	wav_volume = (double)(volume >> VOLUME_SHIFT) / VOLUME_INIT;

	switch(shm->sound)
	{
	case SND_SINE_FM:
		addSineFM(
			0+bank,
			volume,
			main_freq,
			fm_vol1,
			main_freq*fm_mult1,fm_vol_exp,sine_cutoff_ang,reset);
		addSineFM(
			1+bank,
			volume,
			main_freq+fm_harm,
			fm_vol2,
			main_freq*fm_mult2+fm_offset,
			fm_vol_exp,sine_cutoff_ang,0);
		break;

	case SND_SINE:
		addSine(0+bank,volume,main_freq,sine_cutoff_ang,reset);
		break;

	case SND_SQUARE:
		addSquare(0+bank,volume,main_freq,square_width,reset);
		break;

	case SND_TRIANGLE:
		addTriangle(0+bank,volume,main_freq,reset);
		break;

	case SND_SAWTOOTH:
		addSawtooth(0+bank,volume,main_freq,saw_flatten_ratio,reset);
		break;

	case SND_AAH:
		addWav(0+bank,WAV_AAH,wav_volume,main_freq,reset);	
		break;

	case SND_OOH:
		addWav(0+bank,WAV_OOH,wav_volume,main_freq,reset);	
		break;

	case SND_NOISE:
		addNoise(volume,main_freq,reset);
		break;

	case SND_SAMPLE:
		addSample(
			volume,
			shm->win_width,main_freq,shm->sample_mod_cnt,reset);
		break;

	default:
		break;
	}

	if (shm->sub1_note_offset || shm->sub2_note_offset)
	{
		note = findNote(freq);
		freq_add = freq - note_freq[note];
	}

	/* Do sub oscillators */
	if (shm->sub1_sound != SND_OFF)
	{
		sub_vol = getSubOscVolume(volume,shm->sub1_vol);
		sub_freq = getSubOscFreq(
			shm->sub1_offset,
			shm->sub1_note_offset,freq,freq_add,note);

		switch(shm->sub1_sound)
		{
		case SND_SINE_FM:
			addSineFM(
				2+bank,
				sub_vol,
				sub_freq,
				fm_vol1,
				(double)sub_freq*fm_mult1,
				fm_vol_exp,sine_cutoff_ang,0);
			break;

		case SND_SINE:
			addSine(1+bank,sub_vol,sub_freq,sine_cutoff_ang,0);
			break;

		case SND_SQUARE:
			addSquare(1+bank,sub_vol,sub_freq,square_width,0);
			break;

		case SND_TRIANGLE:
			addTriangle(1+bank,sub_vol,sub_freq,0);
			break;

		case SND_SAWTOOTH:
			addSawtooth(1+bank,sub_vol,sub_freq,saw_flatten_ratio,0);
			break;

		case SND_AAH:
			sub_vol = wav_volume * (double)shm->sub1_vol / MAX_UCHAR;
			addWav(1+bank,WAV_AAH,sub_vol,sub_freq,0);	
			break;

		case SND_OOH:
			sub_vol = wav_volume * (double)shm->sub1_vol / MAX_UCHAR;
			addWav(1+bank,WAV_OOH,sub_vol,sub_freq,0);	
			break;

		case SND_NOISE:
			addNoise(sub_vol,sub_freq,0);
			break;

		default:
			assert(0);
		}
	}
	if (shm->sub2_sound != SND_OFF)
	{
		sub_vol = getSubOscVolume(volume,shm->sub2_vol);
		sub_freq = getSubOscFreq(
			shm->sub2_offset,
			shm->sub2_note_offset,freq,freq_add,note);

		switch(shm->sub2_sound)
		{
		case SND_SINE_FM:
			addSineFM(
				FM_OVERFLOW_CHAN,
				sub_vol,
				sub_freq+fm_harm,
				fm_vol2,
				(double)sub_freq*fm_mult2+fm_offset,
				fm_vol_exp,sine_cutoff_ang,0);
			break;

		case SND_SINE:
			addSine(2+bank,sub_vol,sub_freq,sine_cutoff_ang,0);
			break;

		case SND_SQUARE:
			addSquare(2+bank,sub_vol,sub_freq,square_width,0);
			break;

		case SND_TRIANGLE:
			addTriangle(2+bank,sub_vol,sub_freq,0);
			break;

		case SND_SAWTOOTH:
			addSawtooth(2+bank,sub_vol,sub_freq,saw_flatten_ratio,0);
			break;

		case SND_AAH:
			sub_vol = wav_volume * (double)shm->sub2_vol / MAX_UCHAR;
			addWav(2+bank,WAV_AAH,sub_vol,sub_freq,0);	
			break;

		case SND_OOH:
			sub_vol = wav_volume * (double)shm->sub2_vol / MAX_UCHAR;
			addWav(2+bank,WAV_OOH,sub_vol,sub_freq,0);	
			break;

		case SND_NOISE:
			addNoise(sub_vol,sub_freq,0);
			break;

		default:
			assert(0);
		}
	}
}




/*** Get sub osc volume based on main ***/
double getSubOscVolume(int main_volume, u_char shm_sub_vol)
{
	double sub_vol;

	sub_vol = ((double)shm_sub_vol / MAX_UCHAR) * main_volume;
	if (sub_vol < 0) sub_vol = 0;
	else if (sub_vol > MAX_SHORT) sub_vol = MAX_SHORT;

	return sub_vol;
}




/*** Return the sub oscillator frequency based on main freq/note and sub
     offsets ***/
double getSubOscFreq(
	u_char shm_sub_offset,
	u_char shm_sub_note_offset,
	short freq,
	short freq_add,
	int main_note)
{
	double sub_freq;
	int note;
	short offset = (short)shm_sub_offset - MAX_CHAR;

	if (shm_sub_note_offset)
	{
		note = main_note + offset;
		if (note < 0) note = 0;
		else if (note >= NUM_NOTES) note = NUM_NOTES - 1;
		sub_freq = note_freq[note] + freq_add;
	}
	else sub_freq = freq + offset;

	if (sub_freq < 0) sub_freq = 0;
	else if (sub_freq > MAX_SHORT) sub_freq = MAX_SHORT;

	return round(sub_freq * glide_freq_mult);
}


/****************************** ECHO FUNCTIONS *******************************/

void clearEchoBuffer()
{
	bzero(echobuff,sizeof(echobuff));
	echobuff_write_pos = 0;
	echobuff_use_size = 0;
}




/*** Reset echo buffer if echo has just been switched off otherwise when its
     switched on again we'll still have the previous echo in it ***/
void checkEcho()
{
	if (shm->echo_len && do_echo_clear)
	{
		clearEchoBuffer();
		do_echo_clear = 0;
	}
	else if (!shm->echo_len) do_echo_clear = 1;
}




/*** Add echo/reverb effect to sound output and update echo buffer ***/
void doEcho()
{
	static short prev_buff_end[2] = { 0,0 };
	double decay_mult;
	int echo_inv;
	int res;
	int i;
	int j;

	if (!shm->echo_len) return;

	/* This can happen if BASIC has set echo length after main loop should 
	   have set this. Return, next time should be ok */
	if (!echobuff_use_size) return;

	stretchEchoBuffer();

	/* Optionally filter the echo buffer on each pass to give 
	   interesting effects. It makes no difference if this is
	   done before or after the update below */
	if (shm->echo_filter)
	{
		filter(
			shm->echo_highpass_filter,
			(double)shm->echo_filter / MAX_UCHAR,
			echobuff,ECHOBUFF_SIZE,prev_buff_end);
	}

	decay_mult = (double)shm->echo_decay / MAX_UCHAR;

	/* Set sndbuff and update echo buffer */
	echo_inv = (shm->echo_invert ? -1 : 1);
	for(i=0,j=echobuff_write_pos;i < SNDBUFF_SIZE;++i)
	{
		res = gainCompression(round(decay_mult * echobuff[j] + sndbuff[i]));
		CLIP_AND_SET_BUFFER(i);

		echobuff[j] = sndbuff[i] * echo_inv;
		j = (j + 1) % echobuff_use_size;
	}
	echobuff_write_pos = j;
}




/*** Stretch or shrink the sound in the echo buffer to make the frequency go
     up or down on each echo ***/
void stretchEchoBuffer()
{
	static short echobuff2[ECHOBUFF_SIZE];
	int i;
	int wp;
	int start_wp;
	int prev_wp;
	int read_pos;
	double write_pos;
	double pos_add;
	double res;

	if (!echobuff_use_size || shm->echo_stretch == MAX_CHAR) return;

	/* 0.8 to 1.2 */
	pos_add = 0.8 + (double)shm->echo_stretch / MAX_CHAR / 5;

	write_pos = pos_add + echobuff_write_pos;
	start_wp = wp = (int)write_pos % echobuff_use_size;
	prev_wp = echobuff_write_pos;

	for(i=0;i < echobuff_use_size;++i)
	{
		read_pos = (echobuff_write_pos + i) % echobuff_use_size;
		echobuff2[wp] = echobuff[read_pos];

		if (wp - prev_wp > 1)
		{
			res = gainCompression((double)(echobuff2[prev_wp] + echobuff2[wp]) / 2);
			CLIP(res);
			echobuff2[wp-1] = res;
		}
		prev_wp = wp;

		write_pos = write_pos + pos_add;
		wp = (int)write_pos % echobuff_use_size;

		if (pos_add > 1 && wp == start_wp) break;
	}

	/* If we're speeding up the echo then set any leftover buffer to zero */
	if (pos_add < 1)
	{
		for(i=wp;i != start_wp;i=(i + 1) % echobuff_use_size)
			echobuff2[i] = 0;
	}

	/* Copy new data back into real echo buffer */
	for(i=0;i < echobuff_use_size;++i) echobuff[i] = echobuff2[i];
}


/******************************** WRITE OUT *********************************/

/*** Write sound in sndbuff to device ***/
void writeSoundToDevice()
{
#if SOUND==ALSA
	int len;
	int write_errs;
	int frames;
	int total_frames;
#elif SOUND==OPENSOUND
	int len;
	int write_errs;
	int bytes;
	int total_bytes;
	u_int before = 0; 
	u_int wtime;
#endif

	/* If set don't write out data as sound device not opened */
	if (!do_sound)
	{
		usleep(WRITE_DELAY);
		return;
	}

#if SOUND==ALSA
	write_errs = 0;

	/* Write sound data to device. ALSA uses frames (frame = size of format
	   number of channels so mono 16 bit = 2 bytes, stereo = 4 bytes).
	   Add/sub works here because a frame is 2 bytes and sndbuff is of
	   type short */
	total_frames = 0;
	for(len = SNDBUFF_SIZE;len > 0 && write_errs < MAX_WRITE_ERRORS ;)
	{
		/* Occasionally get underrun and need to recover because stream
		   won't work again until you do. I'm not sure if snd_...
		   function is all or nothing. Assuming its not. */
		if ((frames = snd_pcm_writei(handle,sndbuff+total_frames,len)) < 0)
		{
			snd_pcm_recover(handle,frames,1);
			++write_errs;
		}
		else
		{
			total_frames += frames;
			len -= frames;
		}
	}
	if (write_errs == MAX_WRITE_ERRORS)
	{
		printf("SOUND: WARNING: Error writing to sound buffer: snd_pcm_writei(): %s\n",
			snd_strerror(frames));
	}
#elif SOUND==OPENSOUND
	write_errs = 0;
	total_bytes = 0;
	if (use_write_delay) before = getUsecTime();

	/* OpenSound just uses bytes */
	for(len = SNDBUFF_BYTES,bytes=0;write_errs < MAX_WRITE_ERRORS && len > 0;)
	{
		if ((bytes = write(sndfd,(char *)sndbuff+total_bytes,len)) == -1)
		{
			++write_errs;
		}
		else
		{
			total_bytes += bytes;
			len -= bytes;
		}
	}
	if (write_errs == MAX_WRITE_ERRORS)
	{
		printf("SOUND: WARNING: Error writing to sound buffer: write(): %s\n",
			strerror(errno));
	}

	/* Delay because can't rely on how long write() takes */
	if (use_write_delay && (wtime = getUsecTime() - before) < write_delay) 
		usleep(write_delay - wtime);

#elif SOUND==OSX
	/* Copy sound buffer into osx audio buffer. Not very efficient but
	   too complex to fix rest of code to use it direct. Use multiple
	   buffers to smooth out glitches. Not 100% successful. */
	memcpy(osx_buffer[osx_buff_num]->mAudioData,sndbuff,SNDBUFF_BYTES);

	/* 2nd argument is how long to run the function for, ie the delay. 
	   Finessing the value makes little difference */
	CFRunLoopRunInMode(kCFRunLoopDefaultMode,1.0 / SAMPLES_PER_SEC,false);
	osx_buff_num = (osx_buff_num + 1) % num_osx_buffers;

#elif SOUND==NO_SOUND
	/* If no sound compiled in delay anyway so drawn waveform looks correct
	   due to correct sound buffer update speed */
	usleep(WRITE_DELAY);
#endif 
}



#if SOUND==OSX
/*** Function simply buffers the audio. This works better with 2 buffers than
     doing it direct in the above function ***/
void osxAudioCallback(void *ptr, AudioQueueRef q, AudioQueueBufferRef buf)
{
	AudioQueueEnqueueBuffer(q,buf,0,NULL);
}
#endif
