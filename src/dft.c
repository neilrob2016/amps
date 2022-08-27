/*** Discrete Fourier Transform. 

     Unless you have a whole seconds worth of data the maths doesn't work too 
     well. Eg for 10 hertz it needs 10 waveforms to give a sensible result 
     otherwise all sorts of spurious signals crop up. However the sound buffer 
     is 1/20th of a second so we actually look for "frequencies" of 1/20th of 
     the actual frequency.

     Eg: To check for 1000hz we actually look for 50 cycles in a 1/20th of a 
         second sound buffer.

     However this isn't a get out of jail free card since incrementing the
     frequency by < 1 hz produces more spurious signals so we can only look
     at a frequency band, not specific frequencies.

     I used DFT instead of the far more efficient FFT because I have no idea 
     how to implement FFT and I don't want to rely on a 3rd party library
     or cut and paste code from wikipedia.
 ***/

#include "globals.h"


/*** Returns the amount of the given frequency present in the waveform ***/
double dft(int freq)
{
	/* These are static because they have to be */
	static double sinval[360];
	static double cosval[360];
	static int do_setup = 1;

	/* These are static to avoid stack allocation overhead. A tiny 
	   performance improvement but every little bit helps */
	static double anginc;
	static double ang;
	static double sin_total;
	static double cos_total;
	static double level;
	static int x;

	/* Have static trig tables to speed DFT up at the expense of a slight 
	   loss of accuracy */
	if (do_setup)
	{
		for(x=0;x < 360;++x)
		{
			sinval[x] = SIN(x);
			cosval[x] = COS(x);
		}
		do_setup = 0;
	}

	ang = 0;
	sin_total = 0;
	cos_total = 0;
	anginc = (double)360 / SNDBUFF_SIZE * freq;

	/* Multiply by sin & cos waveforms and integrate. ie get the area
	   under the waveform */
	for(x=0;x < SNDBUFF_SIZE;++x)
	{
		level = (double)sndbuff[x];

		/* Use sin and cos since waveform may not be in phase with
		   either so need summed result */
		sin_total += (level * sinval[(int)ang]);
		cos_total += (level * cosval[(int)ang]);

		ang += anginc;
		if (ang >= 360) ang -= 360;
	}

	return fabs(sin_total) + fabs(cos_total);
}




/*** Default analyser_range is 50 which shows frequencies from 20 -> 1000hz ***/
void calcDFT()
{
	int f;

	for(f=0;f < params.analyser_range;++f) dft_freq_level[f] = dft(f+1);
}
