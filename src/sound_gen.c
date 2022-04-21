/*** This contains all the low level sound generation and modification 
     functions (plus sampling) that create the data in sndbuff ***/

#include "globals.h"

#define ONE_WAVELENGTH 2
#define FMFL_MULT      0.5
#define WAV_FREQ       99   /* Frequency of actual recordings */


/* This value is a compromise between efficiency (fewer reads) and losing data
   if we dump whats left in the buffer */
#define SAMPLE_READ_LEN 20

#define SMOOTHING_LEN 150
#define SMOOTHING_END (SNDBUFF_SIZE - SMOOTHING_LEN)

static short tmp_phasingbuff[PHASINGBUFF_SIZE];

int gainCompression(int val);
void ringModSine(int freq, double dist);
void ringModTriangle(int freq, double dist);
void ringModSawtooth(int freq, double dist);
void ringModSquare(int freq, double dist);


/**************************** SOUND GENERATION *****************************/

/*** This has channels because starting from the previous angle prevents
     a clicking sound. If we didn't have this then playing 2 sin notes at
     the same time would be a mess. ***/
void addSine(int ch, double vol, double freq, int cutoff_ang1, int reset)
{
	double ang_inc;
	double dither_val;
	int cutoff_ang2;
	int res;
	int i;
	
	assert(ch < NUM_CHANS);

	if (freq < 1) freq = 1;
	if (reset) resetSoundBuffer();
	if (ch == MAIN_CHANNEL)
	{
		sin_ang_morph[MCHAN_MAIN] = sin_ang[ch];
		sin_ang_morph[MCHAN_GLOBAL] = sin_ang[ch];
	}

	ang_inc = 360 / ((double)pcm_freq / freq);
	cutoff_ang2 = cutoff_ang1 + 180;

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		if (sin_ang[ch] < cutoff_ang1 || 
		    (sin_ang[ch] >= 180 && sin_ang[ch] < cutoff_ang2))
		{
			dither_val = vol * SIN(sin_ang[ch]);
			if (random() % 2)
				dither_val = ceil(dither_val);
			else
				dither_val = floor(dither_val);
			res = gainCompression(sndbuff[i] + (short)dither_val);
			CLIP_AND_SET_BUFFER(i);
		}
		else sndbuff[i] = 0;

		incAngle(&sin_ang[ch],ang_inc);
	}
}




/*** As above except with frequency modulation of the sin wave ***/
void addSineFM(
	int ch, 
	double vol,
	double freq,
	double fm_vol,
	double fm_freq, double fm_vol_exp, int cutoff_ang1, int reset)
{
	double ang_inc;
	double fm_ang_inc;
	double fm_add;
	double dither_val;
	int cutoff_ang2;
	int res;
	int i;
	
	assert(ch < NUM_CHANS);

	if (freq < 1) freq = 1;
	if (fm_freq < 1) fm_freq = 1;
	if (reset) resetSoundBuffer();

	ang_inc = 360 / ((double)pcm_freq / freq);
	fm_ang_inc = (double)360 / ((double)pcm_freq / fm_freq);
	cutoff_ang2 = cutoff_ang1 + 180;

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		if (sin_fm_ang1[ch] < cutoff_ang1 || 
		    (sin_fm_ang1[ch] >= 180 && sin_fm_ang1[ch] < cutoff_ang2))
		{
			fm_add = pow(fm_vol,fm_vol_exp) * SIN(sin_fm_ang2[ch]);
			dither_val = vol * SIN(sin_fm_ang1[ch] + fm_add);
			if (random() % 2)
				dither_val = ceil(dither_val);
			else
				dither_val = floor(dither_val);
			res = gainCompression(sndbuff[i] + (short)dither_val);
			CLIP_AND_SET_BUFFER(i);
		}
		else sndbuff[i] = 0;

		incAngle(&sin_fm_ang1[ch],ang_inc);
		incAngle(&sin_fm_ang2[ch],fm_ang_inc);
	}
}




/*** Triangle waveform ***/
void addTriangle(int ch, double vol, double freq, int reset)
{
	double inc;
	double period;
	double dither_val;
	int res;
	int i;

	assert(ch < NUM_CHANS);

	if (freq < 1) freq = 1;
	if (reset) resetSoundBuffer();

	period = ((double)pcm_freq / freq) / 4;
	inc = vol / (period < 1 ? 1 : period);

	if (!tri_inc[ch] || fabs(tri_inc[ch]) != inc) 
		tri_inc[ch] = tri_inc[ch] < 0 ? -inc : inc;

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		if (random() % 2)
			dither_val = ceil(tri_val[ch]);
		else
			dither_val = floor(tri_val[ch]);
		res = gainCompression(sndbuff[i] + (short)dither_val);
		CLIP_AND_SET_BUFFER(i);

		tri_val[ch] += tri_inc[ch];
		if (tri_val[ch] >= vol)
		{
			tri_val[ch] = vol - (tri_val[ch] - vol);
			tri_inc[ch] = -tri_inc[ch];
		}
		else if (tri_val[ch] <= -vol)
		{
			tri_val[ch] = -vol + (-vol - tri_val[ch]);
			tri_inc[ch] = -tri_inc[ch];
		}
	}
}




/*** Square wave. Get nasty aliasing at higher frequencies but there's nothing
     that can be done other to increase the sample rate. Width is 0 -> 1 and
     defines how much of a pulse we see else zero value  ***/
void addSquare(int ch, double vol, double freq, double width, int reset)
{
	double period;
	double dither_val;
	int res;
	int i;

	assert(ch < NUM_CHANS);

	if (freq < 1) freq = 1;
	if (reset) resetSoundBuffer();

	period = (double)pcm_freq / freq / 2;

	/* Set to new volume but keep the +/- the same or we'll get a click */
	if (fabs(sq_vol[ch]) != fabs(vol)) 
	{
		if (sq_vol[ch] < 0)
			sq_vol[ch] = -(int)vol;
		else
			sq_vol[ch] = (int)vol;
	}

	for(i=0;i < SNDBUFF_SIZE;++i,++sq_cnt[ch])
	{
		if (i >= sq_next_edge[ch])
		{
			/* At edge , invert */
			sq_vol[ch] = -sq_vol[ch];
			sq_cnt[ch] = 0;
			sq_next_edge[ch] += period;
		}
		if (sq_cnt[ch] / period < width)
		{
			if (random() % 2)
				dither_val = ceil(sq_vol[ch]);
			else
				dither_val = floor(sq_vol[ch]);
			res = gainCompression(sndbuff[i] + (short)dither_val);
			CLIP_AND_SET_BUFFER(i);
		}
	}

	/* Wrap value for next call */
	sq_next_edge[ch] -= SNDBUFF_SIZE;
	if (sq_next_edge[ch] < 0) sq_next_edge[ch] = 0;
}




/*** Sawtooth waveform - sharp drop followed by gradual climb, then a sharp
     drop again etc etc. Flatten ratio is 0 -> 1. If zero then pure sawtooth, 
     if 1 then max duty cycle square wave ***/
void addSawtooth(
	int ch, double vol, double freq, double flatten_ratio, int reset)
{
	double val_inc;
	double peak_dist;
	double flatten_dist;
	double flatten_inc;
	double dither_val;
	int res;
	int step_cnt;
	int i;

	assert(ch < NUM_CHANS);

	if (freq < 1) freq = 1;
	if (reset) resetSoundBuffer();

	/* Distance between peaks. -1 for the final drop back down at the end */
	peak_dist = (double)pcm_freq / freq - 1;

	/* Get val_inc for non flattened wave */
	val_inc = vol * 2 / (peak_dist < 1 ? 1 : peak_dist);
	step_cnt = vol * 2 / val_inc + 1;

	if (flatten_ratio)
	{
		flatten_dist = peak_dist * flatten_ratio;
		/* Don't want to just add 1, need fractional part */
		flatten_inc = flatten_dist / (int)flatten_dist;
		peak_dist -= flatten_dist;
		val_inc = vol * 2 / (peak_dist < 1 ? 1 : peak_dist);
	}
	else
	{
		/* Reset */
		saw_flatten_cnt[ch] = 0; 
		saw_flatten_step_cnt[ch] = 0; 
	}

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		if (random() % 2)
			dither_val = ceil(saw_val[ch]);
		else
			dither_val = floor(saw_val[ch]);
		res = gainCompression(sndbuff[i] + (short)dither_val);
		CLIP_AND_SET_BUFFER(i);
		++saw_flatten_step_cnt[ch];

		if (saw_val[ch] >= vol)
		{
			/* Checking against the count is a belt and braces
			   approach since just checking against flatten_dist
			   is sometimes inaccurate due to floating point errors
			   and leads to an audible change in frequency */
			if (flatten_ratio && 
			    saw_flatten_cnt[ch] < flatten_dist && 
			    saw_flatten_step_cnt[ch] < step_cnt)
			{
				saw_val[ch] = vol;
				saw_flatten_cnt[ch] += flatten_inc;
			}
			else
			{
				saw_val[ch] = -vol + (saw_val[ch] - vol);
				saw_flatten_cnt[ch] = 0;
				saw_flatten_step_cnt[ch] = 0;
			}
		}
		else saw_val[ch] += val_inc;
	}
}




/*** Play one of the wave sounds. They don't require dithering since they're
     a recording, not a generated waveform. ***/
void addWav(int ch, int snd, double vol, double freq, int reset)
{
	double wav_inc;
	int res;
	int i;

	assert(ch < NUM_CHANS);
	assert(snd < NUM_WAV_SOUNDS);

	if (freq < 1) freq = 1;
	if (reset) resetSoundBuffer();

	wav_inc = freq / WAV_FREQ;

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		res = gainCompression(
			sndbuff[i] + 
			(short)(vol * waveform[snd]->data[(int)wav_pos[ch]]));
		CLIP_AND_SET_BUFFER(i);

		wav_pos[ch] += wav_inc;
		if (wav_pos[ch] >= waveform[snd]->size) wav_pos[ch] = 0;
	}
}




/*** Create noise. The code picks random Y values every "period" X value.
     Between these points it interpolates. ***/
void addNoise(double vol, double freq, int reset)
{
	/* Static otherwise we get a clicking sound on each call */
	double res;
	double inc;
	double target;
	double period;
	double next;
	short svol;
	int i;
	int j;

	if (reset) resetSoundBuffer();

	period = (double)pcm_freq / (freq < 1 ? 1 : freq) / 4;
	svol = (short)vol;
	inc = 0;
	target = 0;
	next = period;

	for(i=0,j=0;i < SNDBUFF_SIZE;++i,++j)
	{
		res = noise_prev_res + inc;
		noise_prev_res = res;

		if (j >= next)
		{
			target = (random() % (svol * 2 + 1)) - svol;
			inc = (target - res) / period;
			next += period;
		}
		res = gainCompression(sndbuff[i] + res);
		CLIP_AND_SET_BUFFER(i);
	}
}




/*** Used by addSample() ***/
int getSampleShort(int reset, short *sample)
{
#if SOUND!=NO_SOUND
	static short buff[SAMPLE_READ_LEN];
	static int sample_pos = 0;
#if SOUND==ALSA
	int j;
	int err;
#elif SOUND==OPENSOUND
	int read_pos;
	int bytes_left;
	int len;
#endif

	if (reset || sample_pos == SAMPLE_READ_LEN)
	{
		sample_pos = 0;
#if SOUND==ALSA
		/* Sometimes the stream needs to be recovered because ALSA is 
		   shit. Try 10 times then give up. Don't need to check length
		   returned because mic is opened in blocking mode */
		for(j=0;j < 10;++j)
		{
			if ((err = snd_pcm_readi(mic_handle,buff,SAMPLE_READ_LEN)) < 0)
				snd_pcm_recover(mic_handle,err,1);
			else break;
		}
		if (j == 10)
		{
			printf("SOUND: snd_pcm_readi(): %s\n",snd_strerror(err));
			return 0;
		}
#elif SOUND==OPENSOUND
		bytes_left = SAMPLE_READ_LEN * sizeof(short);
		read_pos = 0;
		do
		{
			if ((len = read(sndfd,(char *)buff+read_pos,bytes_left)) < 0)
			{
				printf("SOUND: read(): %s\n",strerror(len));
				return 0;
			}
			read_pos += len;
			bytes_left -= len;
		} while(bytes_left > 0);
#endif
	}
	*sample = buff[sample_pos++];
#endif
	return 1;
}




/*** Read data from the mic into the sound buffer. 

     For slower than normal frequencies (inc > 1) less than SNDBUFF_SIZE 
     samples are read and the following happens:

     |----| -> |-.-.-.-.| where . is interpolated

     For faster than normal (inc < 1) more than SNDBUFF_SIZE samples are read 
     but some are discarded.

     |-.-.-.-.| -> |----| where . is discarded data

     sample_mod_cnt dictates whether a wavelength gets cut (slower) or copied
     (faster) to keep the apparent sample speed similar to the original. This
     doesn't work nearly as well as the professional method of FFT -> shift
     fundamental freq -> resynthesize but its a damn sight simpler.

 ***/
void addSample(
	double vol,
	u_short win_width, double freq, int sample_mod_cnt, int reset)
{
	double i;
	double inc;
	double add;
	double synth;
	short sample;
	short prev_sample;
	int res;
	int pos;
	int prev_pos;
	int cnt;
	int snip;
	int crossed_zero;
	int copy_start;
	int copy_len;
	int set_prev_sample;
	int j;

	if (!do_sampling || !do_sound) return;

	if (reset) resetSoundBuffer();

	if (freq < 1) freq = 1;
	inc = SAMPLE_INC();

	prev_pos = 0;
	prev_sample = 0;
	snip = 0;
	crossed_zero = 0;
	copy_start = -1;
	set_prev_sample = 1;
	reset = 1;

	for(i=0;;)
	{
		pos = (int)floor(i);

		/* If inc > 1 then a simple i < SNDBUFF_SIZE test in for()
		   would lead to missing data at the end of the sndbuff array 
		   since it would terminate before all elements are filled */
		if (pos >= SNDBUFF_SIZE)
		{
			if (prev_pos >= SNDBUFF_SIZE - 1) break;
			pos = SNDBUFF_SIZE - 1;
		}
		if (set_prev_sample) prev_sample = sample;

		if (!getSampleShort(reset,&sample)) return;
		reset = 0;

		/* Play sound faster by discarding sample data */
		if (pos && pos == prev_pos)
		{
			i += inc;
			/* so we won't miss a crossing zero event */
			set_prev_sample = 0; 
			continue; 
		}
		set_prev_sample = 1;

		/* On slower than normal play snip out a single wavelength by 
		   counting how many times we cross the zero point so that
		   we keep the speed even though the frequency is reduced. On
		   faster than normal we duplicate waveforms for the same
		   reason */
		if (sample_mod_cnt)
		{
			if (SGN(sample) != SGN(prev_sample)) ++crossed_zero;

			/* Slower than normal */
			if (inc > 1)
			{
				if (snip)
				{
					if (crossed_zero == ONE_WAVELENGTH)
					{
						snip = 0;
						crossed_zero = 0;
					}
					else continue;
				}
				/* Wait until sample_mod_cnt crossings have 
				   occured before we snip */
				else if (crossed_zero == sample_mod_cnt)
				{
					snip = 1;
					crossed_zero = 0;
					continue;
				}
			}
			/* Faster than normal */
			else if (inc < 1)
			{
				if (copy_start == -1)
				{
					/* Have we found copy start point? */
					if (crossed_zero == sample_mod_cnt)
					{
						copy_start = pos;
						crossed_zero = 0;
					}
				}
				else if (crossed_zero == ONE_WAVELENGTH)
				{
					/* Copy the waveform onto the current 
					   end of buffer */
					copy_len = pos - copy_start;
					if (pos + copy_len > SNDBUFF_SIZE)
						copy_len = SNDBUFF_SIZE - pos;
					for(j=0;j < copy_len;++j)
					{
						res = gainCompression((int)sndbuff[pos+j] + sndbuff[copy_start+j]);
						CLIP_AND_SET_BUFFER(pos+j);
					}
					pos += j;
					i = pos;
					prev_pos = pos - 1;
					copy_start = -1;
					crossed_zero = 0;
				}
			}
		}

		res = gainCompression(vol / 10 * sample + sndbuff[pos]);
		CLIP_AND_SET_BUFFER(pos);

		/* For slower than normal play sound slower by synthesizing 
		   data because we've skipped some positions in the buffer */
		if ((cnt = pos - prev_pos) > 1)
		{
			synth = (double)sndbuff[pos];
			add = ((double)sndbuff[prev_pos] - synth) / cnt;
			for(j=pos-1;j >= prev_pos;--j)
			{
				synth += add;
				res = gainCompression(synth);
				CLIP_AND_SET_BUFFER(j);
			}
		}
		prev_pos = pos;
		i += inc;
	}
}



/**************************** SOUND MODIFICATION ****************************/

/*** Replaced my original algorithms with ones taken from wikipedia as
     they're much more efficient:

     http://en.wikipedia.org/wiki/Low-pass_filter
     http://en.wikipedia.org/wiki/High-pass_filter
 ***/
void filter(
	u_char highpass,
	double level, short *sbuff, int sbuff_size, short *prev_end)
{
	double res;
	short old_prev;
	short new_prev;
	int i;

	/* So 0 = max filtering, 1 = no filtering */
	level = 1 - level;

	if (level < 0) level = 0;
	else if (level > 1) level = 1;

	new_prev = prev_end[0];
	old_prev = prev_end[1];

	for(i=0;i < sbuff_size;++i)
	{
		if (highpass)
			res = level * (new_prev + sbuff[i] - old_prev);
		else
			res = level * (sbuff[i] - new_prev) + new_prev;

		if (res > MAX_SHORT) res = MAX_SHORT;
		else
		if (res < MIN_SHORT) res = MIN_SHORT;

		old_prev = sbuff[i];
		new_prev = sbuff[i] = (short)res;
	}
	prev_end[0] = new_prev;
	prev_end[1] = old_prev;
}




/*** This does my version of resonance. Whatever algorithm virtual modelling 
     synths use it probably isn't this ***/
void resonate(
	double zero_force_mult,
	double speed_mult, double damping_mult, int reset)
{
	static double speed = 0;
	static double amp = 0;
	static short prev = 0;
	double res;
	int i;

	if (reset)
	{
		speed = 0;
		prev = 0;
		amp = 0;
		return;
	}

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		/* Simulates a mass bouncing back and forth over zero with its
		   speed based on the difference of the amplitude of the
		   driving wave compared to last sample with a force pulling
		   the mass back to zero which gets greater the further 
		   away it is */
		speed += (sndbuff[i] - prev) * speed_mult - 
		         (amp * zero_force_mult);
		amp += speed;

		/* Damp, otherwise resonance won't change with freq */
		amp *= damping_mult;

		CLIP(amp);

		prev = sndbuff[i];
		res = gainCompression(sndbuff[i] + (short)amp);
		CLIP_AND_SET_BUFFER(i);
	}
}




/*** Distort by clipping the the waveforms at a given level ***/
void distort(u_char level)
{
	int max;
	int clip;
	int i;
	double mult;

	/* Find max level. Assume min level below zero line is a mirror 
	   image otherwise life gets very complex */
	max = MIN_SHORT;
	for(i=0;i < SNDBUFF_SIZE;++i)
		if (sndbuff[i] > max) max = sndbuff[i];

	/* 256 rather than 255 so mult never goes to zero */
	mult = (double)(256 - level) / MAX_UCHAR;
	clip = (int)round(mult * max);

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		if (sndbuff[i] > clip) sndbuff[i] = (short)clip;
		else
		if (sndbuff[i] < -clip) sndbuff[i] = (short)-clip;

		/* Maintain volume level */
		sndbuff[i] *= (1 / mult);
	}
}




/*** Add flanger by simply overlaying the soundbuffer on itself having been 
     shifted backwards in time slightly. Can't do forwards in time since don't
     have sndbuff from the future! Inverted subtracts otherwise add. The
     smoothing is required because during a sweep there is often a sudden
     change of direction of the waveform (and hence a click heard) between
     previous buffer end and new buffer start if we didn't bring in the
     overlaying data gradually, Only noticable with some waveforms. The 
     "dirty" flanger mode simply keeps the +ve/-ve of the shifted wave the
     same as the original waveform which produces interesting effects. ***/
void flanger(int type_mult, int offset, int dirty, int smooth)
{
	int i;
	int res;
	int pos;
	double add = 0;
	double mult;
	double buff_val;

	if (offset > PHASINGBUFF_SIZE) offset = PHASINGBUFF_SIZE;

	/* Store for for next flange */
	for(i=0;i < PHASINGBUFF_SIZE;++i)
		tmp_phasingbuff[i] = sndbuff[SNDBUFF_SIZE-PHASINGBUFF_SIZE+i];

	/* If smoothing required set params */
	if (smooth)
	{
		add = (double)1 / SMOOTHING_LEN;
		mult = 0;
	}
	else mult = 1;

	/* Go backwards through sound buffer and add to itself offset by the
	   given amount. Do it backwards otherwise we'll change an array 
	   element we'd use later */
	for(i=SNDBUFF_SIZE-1;i >= offset;--i)
	{
		if (dirty)
			buff_val = abs(sndbuff[i-offset]) * SGN(sndbuff[i]);
		else
			buff_val = sndbuff[i-offset];

		res = gainCompression(sndbuff[i] + buff_val * type_mult * mult);
		CLIP_AND_SET_BUFFER(i);

		/* Do smoothing */
		if (smooth)
		{
			if (i >= SMOOTHING_END)
			{
				mult += add;
				if (mult > 1) mult = 1;
			}
			else if (i <= SMOOTHING_LEN)
			{
				mult -= add;
				if (mult < 0) mult = 0;
			}
		}
	}

	/* Add data from previous iteration stored in flangerbuff */
	for(;i >= 0;--i)
	{
		pos = PHASINGBUFF_SIZE-(offset-i);
		if (dirty)
			buff_val = abs(phasingbuff[pos]) * SGN(sndbuff[i]);
		else 
			buff_val = phasingbuff[pos];

		res = gainCompression(sndbuff[i] + buff_val * type_mult * mult);
		CLIP_AND_SET_BUFFER(i);

		if (smooth && i <= SMOOTHING_LEN)
		{
			mult -= add;
			if (mult < 0) mult = 0;
		}
	}
	memcpy(phasingbuff,tmp_phasingbuff,sizeof(phasingbuff));
}




/*** Like a flanger but splits the signal into 2 seperate buffers, offsets
     these then filters one as low frequency and one as high then overlays them
     both back over the top of the main buffer. A dirty mode didn't seem 
     to produce anything different to what the dirty flanger produces so 
     its not been included. ***/
void phaser(
	int type_mult,
	int freq_sep, int high_offset, double low_off_mult, int smooth)
{
	/* Static so we don't waste time recreating them on each call */
	static short low_buff[SNDBUFF_SIZE];
	static short high_buff[SNDBUFF_SIZE];
	static short low_buff_end[2] = { 0,0 };
	static short high_buff_end[2] = { 0,0 };
	double add = 0;
	double mult;
	double filter_level;
	int low_offset;
	int res;
	int i;

	if (high_offset > PHASINGBUFF_SIZE) high_offset = PHASINGBUFF_SIZE;

	/* Adjust low frequency offset since they have a large freq and can
	   be offset less */
	low_offset = (int)round(low_off_mult * high_offset);

	if (smooth)
	{
		add = (double)1 / SMOOTHING_LEN;
		mult = 0;
	}
	else mult = 1;

	/* Store for for next phaser */
	for(i=0;i < PHASINGBUFF_SIZE;++i)
		tmp_phasingbuff[i] = sndbuff[SNDBUFF_SIZE-PHASINGBUFF_SIZE+i];

	/* Copy offset sndbuffer into low and high buffers then filter. Could
	   filter first then offset but what the hell... */
	for(i=0;i < low_offset;++i)
		low_buff[i] = phasingbuff[PHASINGBUFF_SIZE-low_offset+i];
	for(i=0;i < high_offset;++i)
		high_buff[i] = phasingbuff[PHASINGBUFF_SIZE-high_offset+i];

	for(i=low_offset;i < SNDBUFF_SIZE;++i)
		low_buff[i] = sndbuff[i - low_offset];
	for(i=high_offset;i < SNDBUFF_SIZE;++i)
		high_buff[i] = sndbuff[i - high_offset];

	/* Do filtering into a low and high freq buffers. The higher the
	   filter_level the higher the filtering so as freq_sep is increased 
	   the max low pass freq and min high pass frequencies move apart 
	   because there is more filtering occuring */
	filter_level = (double)freq_sep / MAX_UCHAR;
	filter(0,filter_level,low_buff,SNDBUFF_SIZE,low_buff_end);
	filter(1,filter_level,high_buff,SNDBUFF_SIZE,high_buff_end);

	/* Add the 2 buffers back onto the main buffer */
	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		res = gainCompression(sndbuff[i] + 
		      (double)(low_buff[i] + high_buff[i]) * type_mult * mult);
		CLIP_AND_SET_BUFFER(i);

		if (smooth)
		{
			if (i < SMOOTHING_LEN)
			{
				mult += add;
				if (mult > 1) mult = 1;
			}
			else if (i >= SMOOTHING_END)
			{
				mult -= add;
				if (mult < 0) mult = 0;
			}
		}
	}

	/* Remember for next time */
	memcpy(phasingbuff,tmp_phasingbuff,sizeof(phasingbuff));
}




/*** Shift the pitch of buffer up and down on a cosine LFO and add it to itself 
     N times depending on the number of layers. Each layer doubles the speed
     of the LFO. Produces an odd effect. More use for sound effects than
     music I suspect. ***/
void fmFlanger(int num_layers, int pitch_change)
{
	static double tmpbuff[SNDBUFF_SIZE];
	double pos;
	double ang;
	double ang_add;
	int res;
	int p1;
	int p2;
	int i;
	int l;

	if (num_layers < 1) num_layers = 1;
	if (pitch_change < 1) pitch_change = 1;

	bzero(tmpbuff,sizeof(tmpbuff));

	for(l=1;l <= num_layers;++l)
	{
		ang = 0;
		pos = 0;
		p1 = 0;
		p2 = 0;

		/* Each layer doubles the speed of the angle change. Can
		   only double since we need to end at a mulitple of 360
		   degrees so we use up a complete buffer */
		ang_add = (double)360 / SNDBUFF_SIZE * pitch_change * l;

		/* Create temp buffer */
		for(i=0;i < SNDBUFF_SIZE;++i)
		{
			p2 = (int)pos;
			if (!i || p2 != p1)
				tmpbuff[p2] = FMFL_MULT * tmpbuff[p2] + sndbuff[i];

			/* Don't miss any slots. Will never be more than 2
			   since max pos add = 1.5 from COS eqn below */
			if (i && p2 - p1 > 1)
			{
				++p1;
				tmpbuff[p1] = FMFL_MULT * tmpbuff[p1] +
				              (double)(sndbuff[i] + sndbuff[i-1]) / 2;
			}
			p1 = p2;
			pos += (1 + COS(ang) / 2);
			ang += ang_add;
		}

		/* It usually misses the last one */
		p2 = (int)pos;
		if (p2 < SNDBUFF_SIZE)
		{
			--i;
			if (p2 - p1 > 1)
			{
				++p1;
				tmpbuff[p1] = FMFL_MULT * tmpbuff[p1] + 
				              (double)(sndbuff[i] + sndbuff[i-1]) / 2;
			}
			tmpbuff[p2] = FMFL_MULT * tmpbuff[p2] + sndbuff[i];
		}
	}

	/* Copy tmpbuff into sndbuff */
	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		res = gainCompression(tmpbuff[i]);
		CLIP_AND_SET_BUFFER(i);
	}
}




/*** Rapid changing of frequency pitch ***/
short vibrato(short freq, double vib_mult, double ang_inc)
{
	if (vib_mult && ang_inc)
	{
		freq += round((double)freq * vib_mult * SIN(vibrato_sweep_ang));
		if (freq < 0) freq = 0;
		incAngle(&vibrato_sweep_ang,ang_inc);
	}
	else vibrato_sweep_ang = 0;

	return freq;
}




/*** Map the sound buffer amplitude to certain levels only producing a square
     wave type stepping effect ***/
void alias(u_char al, short peak_volume)
{
	int ialias;
	int i;

	/* Multiply al by itself to provide exponential increase. Use volume
	   so effect is the same proportionately for a given wave amplitude */
	ialias = (int)round((double)al * al / 2 * (double)peak_volume / MAX_SHORT);
	if (ialias < 2) ialias = 2;

	for(i=0;i < SNDBUFF_SIZE;++i)
		sndbuff[i] = sndbuff[i] / ialias * ialias;
}




/*** Reflect the points in the buffer around the level if they are above
     the level (or below the -level). The greater the smoothing the less
     sharp changes in amplitude ***/
void reflect(u_char rl, u_char rs, short peak_volume)
{
	double level;
	double val;
	double diff;
	double mult;
	double smoothing;
	int i;

	level = (double)peak_volume * (double)(MAX_UCHAR - rl) / MAX_UCHAR;
	smoothing = (double)rs / MAX_UCHAR;

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		val = (double)sndbuff[i];
		if (val > 0 && val > level)
		{
			diff = val - level;
			mult = diff / ((double)peak_volume * smoothing);
		}
		else if (val < 0 && val < -level)
		{
			diff = val + level;
			mult = -diff / ((double)peak_volume * smoothing);
		}
		else continue;

		if (mult < 1) diff *= mult;
		sndbuff[i] = (short)round(val - diff * 2);
	}
}




/*** Do ring modulation - multiply the sound buffer waveform by the ring 
     modulators waveform ***/
void ringModulate(int snd, int freq, int level)
{
	static void (*func[NUM_RING_MODES])(int freq, double dist) =
	{
		ringModSine,
		ringModSquare,
		ringModTriangle,
		ringModSawtooth
	};

	if (freq > 0) (*func[snd])(freq,(double)level / MAX_UCHAR);
}




void ringModSine(int freq, double dist)
{
	double mult;
	int i;

	if (freq != ring_sine_prev_freq)
	{
		if (freq < 1) freq = 1;
		ring_sin_inc = 360 / ((double)pcm_freq / freq);
		ring_sine_prev_freq = freq;
	}

	/* Don't need to clip since max/min we'll multiply by will only
	   ever be 1 or -1 */
	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		/* Convert from 1 -> -1 to 1 -> 0 */
		mult = (COS(ring_sin_ang) + 1) / 2;

		/* Shrink from 1 -> 0 to 1 -> N based on level. Eg if level 
		   is 20% (255 / 5 = 51) then want to go from 1 -> 0.8 */
		mult = 1 - mult * dist;

		sndbuff[i] = (short)round(mult * sndbuff[i]);
		incAngle(&ring_sin_ang,ring_sin_inc);
	}
}




void ringModSquare(int freq, double dist)
{
	int i;

	if (freq != ring_sqr_prev_freq)
	{
		ring_sq_period = (double)pcm_freq / freq / 2;
		ring_sqr_prev_freq = freq;
	}
	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		if (i >= ring_sq_next_edge)
		{
			ring_sq_next_edge += ring_sq_period;
			ring_sq_do_mult = !ring_sq_do_mult;
		}

		/* If do mult then set to min value else leave unchanged */
		if (ring_sq_do_mult)
			sndbuff[i] = (short)round((1 - dist) * sndbuff[i]);
	}
	ring_sq_next_edge -= SNDBUFF_SIZE;
	if (ring_sq_next_edge < 0) ring_sq_next_edge = 0;
}




/*** This is more complex than main triangle generator since we're modifying 
     the waveform, not adding to it and hence need to multiply, not add ***/
void ringModTriangle(int freq, double dist)
{
	int i;

	if (freq != ring_tri_prev_freq || dist != ring_tri_prev_dist)
	{
		ring_tri_inc = dist * ((double)freq / pcm_freq) * 2;
		ring_tri_min = 1 - dist;
		ring_tri_prev_freq = freq;
		ring_tri_prev_dist = dist;
	}

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		sndbuff[i] = (short)round(ring_tri_mult * sndbuff[i]);	
		ring_tri_mult += ring_tri_inc;
		if (ring_tri_mult >= 1)
		{
			ring_tri_mult = 1;
			ring_tri_inc = -ring_tri_inc;
		}
		else if (ring_tri_mult <= ring_tri_min)
		{
			ring_tri_mult = ring_tri_min;
			ring_tri_inc = -ring_tri_inc;
		}
	}
}




void ringModSawtooth(int freq, double dist)
{
	int i;

	if (freq != ring_saw_prev_freq || dist != ring_saw_prev_dist)
	{
		ring_saw_inc = dist * ((double)freq / pcm_freq);
		ring_saw_min = 1 - dist;
		ring_saw_prev_freq = freq;
		ring_saw_prev_dist = dist;
	}

	for(i=0;i < SNDBUFF_SIZE;++i)
	{
		sndbuff[i] = (short)round(ring_saw_mult * sndbuff[i]);	
		ring_saw_mult += ring_saw_inc;
		if (ring_saw_mult >= 1) ring_saw_mult = ring_saw_min;
	}
}




/*** Do some limiting on the volume ***/
int gainCompression(int val)
{
	double v;
	int comp_start;
	int aval;

	comp_start = MAX_CHAR * shm->compress_start;
	aval = abs(val);
	if (aval >= comp_start)
	{
		v = round(pow(aval - comp_start,
		             (double)shm->compress_exponent / MAX_UCHAR) + comp_start);
		return val < 0 ? -v : v;
	}
	return val;
}
