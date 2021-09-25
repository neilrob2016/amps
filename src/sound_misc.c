/*** Any sound specific functionality that doesn't belong in the other sound_
     files goes here ***/

#include "globals.h"


void setKeyStartNote(char val)
{
	if (val > NUM_NOTES - NUM_KEYB_NOTES)
		params.key_start_note = NUM_NOTES - NUM_KEYB_NOTES;
	else if (val < 0)
		params.key_start_note = 0;
	else
		params.key_start_note = val;

	message("Key start note = %d",params.key_start_note);
	runEventSection(SECTION_KEYSTART,params.key_start_note,0,NULL);
}




/*** Get the octave + name of a note given the numeric value. 
     eg: '0C', '2A#' ***/
char *getNoteName(int note, int scale)
{
	static char note_name[4];
	int basefreq;

	/* Get octave number */
	note_name[0] = '0' + (note / OCTAVE);

	/* Get note letter */
	note %= OCTAVE;
	note_name[1] = scale_letter[scale][note];

	/* See if its a sharp */
	basefreq = (int)base_octave[scale][note];
	if (basefreq == (int)FREQ_CS ||
	    basefreq == (int)FREQ_DS ||
	    basefreq == (int)FREQ_FS ||
	    basefreq == (int)FREQ_GS ||
	    basefreq == (int)FREQ_AS)
	{
		note_name[2] = '#';
		note_name[3] = 0;
	}
	else note_name[2] = 0;

	return note_name;
}



#define NUM_FREQS_PER_OCTAVE 12

/*** Get the note number given the name plus the octave. Octaves go from zero
     to six. Format is <octave><note>[#]. Eg: 2C# ***/
int getNoteFromName(char *name, int scale)
{
	/* Map note name to base frequency. Cast the frequencies to ints since 
	   doubleing point equality comparisons frequenctly fail. Luckily no
	   2 frequencies have the same integer component. */
	static struct 
	{
		char *name;
		int freq;
	} freq_map[NUM_FREQS_PER_OCTAVE] =
	{
		{ "C",  (int)FREQ_C },
		{ "C#", (int)FREQ_CS },
		{ "D",  (int)FREQ_D },
		{ "D#", (int)FREQ_DS },
		{ "E",  (int)FREQ_E },
		{ "F",  (int)FREQ_F },
		{ "F#", (int)FREQ_FS },
		{ "G",  (int)FREQ_G },
		{ "G#", (int)FREQ_GS },
		{ "A",  (int)FREQ_A },
		{ "A#", (int)FREQ_AS },
		{ "B",  (int)FREQ_B }
	};
	int len;
	int octave;
	int n;
	int i;

	if ((len = strlen(name)) < 1 || len > 3) return -ERR_INVALID_NOTE;

	/* Check first char is a valid octave */
	octave = name[0] - '0';
	if (octave < 0 || octave >= NUM_OCTAVES) return -ERR_INVALID_NOTE;

	/* Check 2nd char is valid note */
	name[1] = toupper(name[1]);
	if (name[1] < 'A' || name[1] > 'G') return -ERR_INVALID_NOTE;

	/* If 3rd digit is a sharp then do sanity check - there is no such 
	   thing as E# or B# in this program. */
	if (name[2] == '#' && (name[1] == 'E' || name[1] == 'B'))
		return -ERR_INVALID_NOTE;

	/* Check note is valid for the current scale. First get its frequency
	   then look for this in base_octaves freq list for current scale */
	for(n=0;n < NUM_FREQS_PER_OCTAVE && strcmp(name+1,freq_map[n].name);++n);
	assert(n < NUM_FREQS_PER_OCTAVE);

	for(i=0;i < OCTAVE && 
	        (int)base_octave[scale][i] != freq_map[n].freq;++i);
	if (i == OCTAVE) return -ERR_INVALID_NOTE_FOR_SCALE;
	return i + (OCTAVE * octave);
}




/*** Looks up on long and short name ***/
int getScaleFromName(char *str)
{
	int scale;

	/* Try short name then long name */
	for(scale=0;
	    scale < NUM_SCALES && strcasecmp(str,scale_short_name[scale]);
	    ++scale);
	if (scale == NUM_SCALES)
	{
		for(scale=0;
		    scale < NUM_SCALES && strcasecmp(str,scale_name[scale]);
		    ++scale);
		if (scale == NUM_SCALES) return -1;
	}
	return scale;
}




int getSoundNumberFromName(char *str)
{
	int i;
	for(i=0;i < NUM_SND_TYPES;++i)
		if (!strcasecmp(str,sound_name[i])) return i;
	return -1;
}
