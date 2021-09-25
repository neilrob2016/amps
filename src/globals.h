/*** All global variables and definitions go here ***/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <search.h>
#include <errno.h>
#include <math.h>
#include <termios.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <pwd.h>
#include <libgen.h>
#include <assert.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define ALSA      1
#define OPENSOUND 2
#define OSX       3
#define NO_SOUND  4

#if SOUND==0
#error "The SOUND macro must be defined"
#elif SOUND==ALSA
#include <alsa/asoundlib.h>
#elif SOUND==OPENSOUND
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#elif SOUND==OSX
#include "AudioToolbox/AudioToolbox.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/Xdbe.h>

#include "build_date.h"

#define COPYRIGHT1   "Copyright (C) Neil Robertson 2013-2020"
#define COPYRIGHT2   "AMPS SYNTH - COPYRIGHT (C) NEIL ROBERTSON 2013-2020"
#define VERSION      "1.11.2"
#define PATCH_SUFFIX ".amp"

#define STDIN       0
#define ESCAPE_KEY  27

#define MAX_CHAR    0x7F
#define MAX_UCHAR   0xFF
#define NUM_UCHARS  0x100
#define MAX_SHORT   0x7FFF
#define MIN_SHORT   (short)0x8000
#define MAX_USHORT  0xFFFF

#define WIN_WIDTH   800
#define WIN_HEIGHT  600
#define WIN_HEIGHT2 (WIN_HEIGHT / 2)
#define FILTER_Y    (WIN_HEIGHT / 4)
#define TAIL_LEN    200
#define DIAM        20

#define TITLE_MAX_LEN 200

#define NUM_COLOURS   91
#define GREEN         0
#define LIGHT_GREEN   10
#define TURQUOISE     15
#define DIAL_COL      18
#define LIGHT_BLUE    20
#define BLUE          25
#define PURPLE        38
#define MAUVE         45
#define PINKY_RED     52
#define RED           60
#define ORANGE        68
#define YELLOW        75
#define MEDIUM_YELLOW 78
#define KHAKI         82
#define BLACK         (NUM_COLOURS - 1)
#define WHITE         NUM_COLOURS

#define BUTTON_HEIGHT 30
#define BUTTON_GAP    4
#define BUTTON_PRESS  8

#define DIAL_ANGLE_MIN        53
#define DIAL_ANGLE_MAX       (53+MAX_UCHAR)
#define DIAL_ANGLE_RANGE     (DIAL_ANGLE_MAX - DIAL_ANGLE_MIN)
#define FREQ_RANGE_DIAL_INIT  36
#define ANALYSER_RANGE_INIT   50
#define PHASER_LOW_MULT_RANGE 2

#define NUM_SWAP_EFFECTS      7
#define EFFECTS_SEQ_STR_LEN   (NUM_SWAP_EFFECTS * 3)

#define VOLUME_SHIFT 6
#define VOLUME_INIT  40

#define USEC_LOOP_DELAY  (u_int)20000

/* Used for timing double escape key press and double BASIC load click. 
   50 = 1 second if USEC_LOOP_DELAY = 20000 usecs */
#define PRESS_TIMEOUT_CNT 50

#define SGN(X) ((X) < 0 ? -1 : ((X) > 0 ? 1 : 0))
#define FREE(M) if (M) { free(M); M = NULL; }

#define pragma pack(1)

enum en_quit_reason
{
	QUIT_GUI,
	QUIT_KEYBOARD,
	QUIT_BASIC,
	QUIT_SIGNAL,
	QUIT_XKILL,

	NUM_QUIT_REASONS
};

enum en_mouse_button
{
	LEFT_MBUTTON = 1,
	CENTRE_MBUTTON,
	RIGHT_MBUTTON,
	WHEEL_UP,
	WHEEL_DOWN
};

enum en_button_type
{
	BTYPE_STATIC,  /* Stays on or off. May toggle another button off */
	BTYPE_CLICK,   /* Flashes briefly when clicked */
	BTYPE_BLINK,   /* Blinks continuously when on */
	BTYPE_DIAL     /* Round dial */
};

enum en_button
{
	/* Top layer */
	BUT_SINE_FM,
	BUT_SINE,
	BUT_SQUARE,
	BUT_TRIANGLE,
	BUT_SAWTOOTH,
	BUT_AAH,
	BUT_OOH,
	BUT_NOISE,
	BUT_SAMPLE,

	/* Bottom layer 1 */
	BUT_EVR_RECORD,
	BUT_EVR_PLAY,
	BUT_EVR_PLAY_PAUSE,
	BUT_EVR_LOOP,
	BUT_EVR_EVENTS_IN_SAVE,
	BUT_EVR_PATCH_RESET,
	BUT_EVR_PATCH_EQ_MAIN,
	BUT_EVR_CLEAR,

	/* Bottom layer 2 */
	BUT_SPECTRUM_ANALYSER,
	BUT_BUFFER_RESET,
	BUT_HIGHPASS_FILTER,
	BUT_ECHO_HIGHPASS_FILTER,
	BUT_HOLD_NOTE,
	BUT_SUB1_NOTE_OFFSET,
	BUT_SUB2_NOTE_OFFSET,
	BUT_SUBS_FOLLOW,
	BUT_RANDOMISE,
	BUT_RESTART_PROG,
	BUT_PAUSE_PROG,
	BUT_DEL_PROG,

	/* Bottom layer 3 */
	BUT_ECHO_CLEAR,
	BUT_ECHO_INVERT,
	BUT_ECHO_LEN,
	BUT_ECHO_DECAY,
	BUT_ECHO_FILTER,
	BUT_ECHO_STRETCH,
	BUT_CHORD,
	BUT_ARP_SEQ,
	BUT_ARP_SPACING_DEC,
	BUT_ARP_SPACING_INC,
	BUT_ARP_DELAY_DEC,
	BUT_ARP_DELAY_INC,
	BUT_SUB1_SOUND,
	BUT_SUB1_OFFSET,
	BUT_SUB1_VOL,
	BUT_SUB2_SOUND,
	BUT_SUB2_OFFSET,
	BUT_SUB2_VOL,
	BUT_VIB_SWEEP,
	BUT_VIB_LFO,
	BUT_SINE_CUTOFF,
	BUT_SINE_CUTOFF_LFO,
	BUT_SQUARE_WIDTH,
	BUT_SQUARE_WIDTH_LFO,
	BUT_SAW_FLATTEN,
	BUT_SAW_FLATTEN_LFO,

	/* Bottom layer 4 */
	BUT_FREQ_MODE,
	BUT_MAX_FREQ,
	BUT_COMPRESS_START,
	BUT_COMPRESS_EXP,
	BUT_EFFECTS_TO_SWAP,
	BUT_SWAP_EFFECTS,
	BUT_RESET_EFFECTS_SEQ,
	BUT_SAMPLE_MOD_DEC,
	BUT_SAMPLE_MOD_INC,
	BUT_FM_HARM_OFFSET,
	BUT_FM_MULT1,
	BUT_FM_MULT2,
	BUT_FM_OFFSET,
	BUT_FM_VOL1,
	BUT_FM_VOL2,
	BUT_FM_WIERD,
	BUT_PHASING_MODE,
	BUT_PHASING_OFFSET,
	BUT_PHASING_SWEEP,
	BUT_PHASING_LFO,
	BUT_PHASER_FREQ_SEP,
	BUT_PHASER_LOW_OFF_MULT,
	BUT_FILTER_SWEEP,
	BUT_FILTER_LFO,
	BUT_REFLECT_LEVEL,
	BUT_REFLECT_SMOOTHING,

	/* Bottom layer 5 */
	BUT_RES_MODE,
	BUT_RES_LEVEL,
	BUT_RES_FREQ,
	BUT_RES_DAMPING,
	BUT_GLIDE_DISTANCE,
	BUT_GLIDE_VELOCITY,
	BUT_DISTORTION,
	BUT_ALIASING,
	BUT_RING_RANGE,
	BUT_RING_MODE,
	BUT_RING_LEVEL,
	BUT_RING_FREQ,
	BUT_ATTACK,
	BUT_DECAY,
	BUT_VOLUME,
	BUT_ANALYSER_RANGE,
	BUT_SAVE_PATCH,
	BUT_SAVE_PROG,
	BUT_LOAD_PATCH,
	BUT_LOAD_PROG,
	BUT_RESET,
	BUT_QUIT,

	NUM_BUTTONS
};

enum en_wav_sounds
{
	WAV_AAH,
	WAV_OOH,

	NUM_WAV_SOUNDS
};
	
#define TOP_BUTTONS_END BUT_SAMPLE
#define NUM_TOP_BUTTONS (TOP_BUTTONS_END+1)

#define BOT_BUTTONS1_END  BUT_EVR_CLEAR
#define NUM_BOT_BUTTONS1 (BOT_BUTTONS1_END - TOP_BUTTONS_END)

#define BOT_BUTTONS2_END  BUT_DEL_PROG
#define NUM_BOT_BUTTONS2 (BOT_BUTTONS2_END - BOT_BUTTONS1_END)

#define BOT_BUTTONS3_END  BUT_SAW_FLATTEN_LFO
#define NUM_BOT_BUTTONS3 (BOT_BUTTONS3_END - BOT_BUTTONS2_END)

#define BOT_BUTTONS4_END  BUT_REFLECT_SMOOTHING
#define NUM_BOT_BUTTONS4 (BOT_BUTTONS4_END - BOT_BUTTONS3_END)

#define BOT_BUTTONS5_END  BUT_QUIT
#define NUM_BOT_BUTTONS5 (BOT_BUTTONS5_END - BOT_BUTTONS4_END)

/* Want buttons on 3 bottoms layers to be the same size regardless of how
   many there are on each layer */
#define LAYER_34_BUTTONS \
	(NUM_BOT_BUTTONS3 > NUM_BOT_BUTTONS4 ? NUM_BOT_BUTTONS3 : NUM_BOT_BUTTONS4)

#define LAYER_345_BUTTONS \
	(NUM_BOT_BUTTONS5 > LAYER_34_BUTTONS ? NUM_BOT_BUTTONS5 : LAYER_34_BUTTONS)
#define TOP_BUTTON_WIDTH  ((double)WIN_WIDTH / NUM_TOP_BUTTONS)
#define BOT_BUTTON_WIDTH1 ((double)WIN_WIDTH / NUM_BOT_BUTTONS1)
#define BOT_BUTTON_WIDTH2 ((double)WIN_WIDTH / NUM_BOT_BUTTONS2)
#define BOT_BUTTON_WIDTH3 ((double)WIN_WIDTH / LAYER_345_BUTTONS)
#define BOT_BUTTON_WIDTH4 ((double)WIN_WIDTH / LAYER_345_BUTTONS)
#define BOT_BUTTON_WIDTH5 ((double)WIN_WIDTH / LAYER_345_BUTTONS)

#define BOT_Y1 (WIN_HEIGHT - BUTTON_HEIGHT * 5)
#define BOT_Y2 (WIN_HEIGHT - BUTTON_HEIGHT * 4)
#define BOT_Y3 (WIN_HEIGHT - BUTTON_HEIGHT * 3)
#define BOT_Y4 (WIN_HEIGHT - BUTTON_HEIGHT * 2)
#define BOT_Y5 (WIN_HEIGHT - BUTTON_HEIGHT)

#define MESG_START_Y (BOT_Y1 - BUTTON_HEIGHT)

struct st_waveform
{
	int size;
	short data[1];
};

struct st_waveform *waveform[NUM_WAV_SOUNDS];

struct st_point
{
	short x;
	short y;
	int colour;
} tail[TAIL_LEN];

int tail_next;

struct st_button
{
	int type;
	int colour;
	int pressed;
	short x;
	short y;
	short text_x;
	short text_y;
	short underline_text_y;
	short mid_x;
	short mid_y;
	short width;
	double angle;
	double text_x_scale;
	double text_y_scale;
	char *underline_text;
};

struct st_button button[NUM_BUTTONS];

#ifdef MAINFILE
char *button_name[NUM_BUTTONS] =
{
	/* Top layer */
	"SINE FM",
	"SINE",
	"SQUARE",
	"TRIANGLE",
	"SAWTOOTH",
	"AAH",
	"OOH",
	"NOISE",
	"SAMPLE",

	/* Bottom layer 1 */
	"EVR RECORD",
	"EVR PLAY",
	"EVR PLAY PAUSE",
	"EVR LOOP",
	"EVR EVENTS IN SAVE",
	"EVR PATCH RESET",
	"EVR PATCH = MAIN",
	"EVR CLEAR",

	/* Bottom layer 2 */
	"SPECTRUM ANALYSER",
	"BUFFER RESET",
	"HIGHPASS FILTER",
	"ECHO HIGHPASS FILTER",
	"HOLD NOTE",
	"SUB1 NOTES",
	"SUB2 NOTES",
	"SUBS FOLLOW MAIN",
	"RANDOMISE",
	"RESTART PROG",
	"PAUSE PROG",
	"DEL PROG",

	/* Bottom layer 3 */
	"EC",
	"EI",
	"EL",
	"ED",
	"EF",
	"EST",
	"CH",
	"AR",
	"AS1",
	"AS2",
	"AD1",
	"AD2",
	"SB1",
	"SO1",
	"SV1",
	"SB2",
	"SO2",
	"SV2",
	"VS",
	"VL",
	"SC",
	"LS",
	"SW",
	"LW",
	"SF",
	"SL",

	/* Bottom layer 4 */
	"FQ",
	"MF",
	"CS",
	"CE",
	"ET",
	"ES", 
	"ER",
	"SM1",
	"SM2",
	"FH",
	"FM1",
	"FM2",
	"FO",
	"FV1",
	"FV2",
	"FW",
	"PH",
	"PO",
	"PS",
	"PL",
	"PF",
	"PD",
	"FS",
	"FL",
	"RT",
	"RS",

	/* Bottom layer 5 */
	"RE",
	"RL",
	"RF",
	"RD",
	"GD",
	"GV",
	"DI",
	"AL",
	"RI",
	"RM",
	"IL",
	"IF",
	"AT",
	"DE",
	"VOL",
	"SAR",
	"SVP",
	"SVB",
	"LDP",
	"LDB",
	"RST",
	"QU"
};
#else
extern char *button_name[NUM_BUTTONS];
#endif

/********************************* SOUND ************************************/

#define ALSA_DEVICE      "sysdefault"
#define PCM_FREQ         20000
#define NUM_CHANS        12

#define SAMPLES_PER_SEC   20  /* Sample is 1/20th of a second */
#define SNDBUFF_SIZE      (PCM_FREQ / SAMPLES_PER_SEC) 
#define SNDBUFF_BYTES     (SNDBUFF_SIZE * sizeof(short))
#define SND_X_ADD         ((double)WIN_WIDTH / (SNDBUFF_SIZE - 1))
#define SND_Y_MULT        ((double)WIN_HEIGHT / (MAX_SHORT * 4))
#define ECHOBUFF_SIZE     (SNDBUFF_SIZE * MAX_UCHAR)
#define PHASINGBUFF_SIZE  (MAX_UCHAR * 2)
#define FILTER_MULT       ((double)MAX_UCHAR / (BOT_Y1 - FILTER_Y))
#define NUM_KEYB_NOTES    12
#define FM_OFFSET_DIV     10

/* Delay is 1 second divided by number of samples in a second minus 
   a fiddle factor */
#define WRITE_DELAY  (1000000 / SAMPLES_PER_SEC - 500) 

#define SAMPLE_INC() ((double)shm->win_width / 2 / freq)

#define DEGS_PER_RADIAN  57.29578
#define SIN(A)           sin((double)(A) / DEGS_PER_RADIAN)
#define COS(A)           cos((double)(A) / DEGS_PER_RADIAN)

#define OCTAVE      7
#define NUM_OCTAVES 7
#define NUM_NOTES   (OCTAVE * NUM_OCTAVES)


/* Prevent wrapping - clip instead */
#define CLIP(X) \
	if (X > MAX_SHORT) X = MAX_SHORT; \
	else \
	if (X < MIN_SHORT) X = MIN_SHORT; \

#define CLIP_AND_SET_BUFFER(POS) \
	if (res > MAX_SHORT) sndbuff[POS] = MAX_SHORT; \
	else \
	if (res < MIN_SHORT) sndbuff[POS] = MIN_SHORT; \
	else \
	sndbuff[POS] = (short)res;

u_int pcm_freq;
	
/* PCM buffers */
short *sndbuff;
short freeze_sndbuff[SNDBUFF_SIZE];
short echobuff[ECHOBUFF_SIZE];
short phasingbuff[PHASINGBUFF_SIZE];

#if SOUND==ALSA
char *alsa_device;
snd_pcm_t *handle;
snd_pcm_t *mic_handle;
#elif SOUND==OPENSOUND
int sndfd;
#elif SOUND==OSX
#define NUM_OSX_BUFFERS 3
AudioQueueRef osx_queue;
AudioQueueBufferRef *osx_buffer;
int num_osx_buffers;
int osx_buff_num;
#endif

/* Sound generation vars */
double sin_ang[NUM_CHANS];
double sin_fm_ang1[NUM_CHANS];
double sin_fm_ang2[NUM_CHANS];

double tri_val[NUM_CHANS];
double tri_inc[NUM_CHANS];

double sq_vol[NUM_CHANS];
double sq_cnt[NUM_CHANS];
double sq_next_edge[NUM_CHANS];

double saw_val[NUM_CHANS];
double saw_flatten_cnt[NUM_CHANS];
int    saw_flatten_step_cnt[NUM_CHANS];

double wav_pos[NUM_CHANS];

double noise_prev_res;
double vibrato_sweep_ang;

double ring_sin_ang;
double ring_sin_inc;
double ring_tri_mult;
double ring_tri_inc;
double ring_tri_min;
double ring_saw_inc;
double ring_saw_mult;
double ring_saw_min;
double ring_sq_do_mult;
double ring_sq_period;
double ring_sq_next_edge;
double ring_tri_prev_dist;
double ring_saw_prev_dist;
int ring_sine_prev_freq;
int ring_sqr_prev_freq;
int ring_tri_prev_freq;
int ring_saw_prev_freq;

int prev_note_scale;

enum en_note_scale
{
	SCALE_C,   /* All the piano white keys */
	SCALE_CM,
	SCALE_CS,  /* All the piano black keys + C & F */
	SCALE_CSM,

	SCALE_D,
	SCALE_DM,
	SCALE_DSM,

	SCALE_EM,

	SCALE_FM,
	SCALE_FSM,

	SCALE_GM,
	SCALE_GSM,

	SCALE_A,

	SCALE_B,

	NUM_SCALES
};

/* Base octave note frequencies */
#define FREQ_C  32.70
#define FREQ_CS 34.65
#define FREQ_D  36.71
#define FREQ_DS 38.89
#define FREQ_E  41.20
#define FREQ_F  43.65
#define FREQ_FS 46.25
#define FREQ_G  49.00
#define FREQ_GS 51.91
#define FREQ_A  55.00
#define FREQ_AS 58.27
#define FREQ_B  61.74

/* Scale notes. Based on:
   http://www.cs.cmu.edu/~scottd/chords_and_scales/music.html

   Must start with C/C# as needs to start with lowest base frequency as defined
   by the values here.

   C   : C  D  E  F  G  A  B
   Cm  : C  D  D# F  G  G# A#
   C#  : C  C# D# F  F# G# A#
   C#m : C# D# E  F# G# A  B

   D   : C# D  E  F# G  A  B 
   Dm  : C  D  E  F  G  A  A# 
   D#  : same as C minor
   D#m : C# D# F  F# G# A# B  

   E   : same as C# minor
   Em  : C  D  E  F# G  A  B 

   F   : same as D minor
   Fm  : C  C# D# F  G  G# A#
   F#  : same as D# minor
   F#m : C# D  E  F# G# A  B 

   G   : same as E minor
   Gm  : C  D  D# F  G  A  A#
   G#  : same as F minor
   G#m : C# D# E  F# G# A# B

   A   : C# D  E  F# G# A  B
   Am  : same as C  
   A#  : same as G minor
   A#m : same as C# 

   B   : C# D# E  F# G# A# B
   Bm  : same as D 

   Frequency of lowest base octave notes which we create all octaves 
   from by multipling by 2, 4, 8, 16 etc. OCTAVE is 7 because we don't 
   include the note 1 octave higher */
#ifdef MAINFILE
double base_octave[NUM_SCALES][OCTAVE] =
{
	{ /* C   */ FREQ_C, FREQ_D, FREQ_E, FREQ_F, FREQ_G, FREQ_A, FREQ_B }, 
	{ /* Cm  */ FREQ_C, FREQ_D, FREQ_DS, FREQ_F, FREQ_G, FREQ_GS, FREQ_AS },
	{ /* C#  */ FREQ_C, FREQ_CS, FREQ_DS, FREQ_F, FREQ_FS, FREQ_GS, FREQ_AS },
	{ /* C#m */ FREQ_CS, FREQ_DS, FREQ_E, FREQ_FS, FREQ_GS, FREQ_A, FREQ_B },

	{ /* D   */ FREQ_CS, FREQ_D, FREQ_E, FREQ_FS, FREQ_G, FREQ_A, FREQ_B },
	{ /* Dm  */ FREQ_C, FREQ_D, FREQ_E, FREQ_F, FREQ_G, FREQ_A, FREQ_AS },
	{ /* D#m */ FREQ_CS, FREQ_DS, FREQ_F, FREQ_FS, FREQ_GS, FREQ_AS, FREQ_B },

	{ /* Em  */ FREQ_C, FREQ_D, FREQ_E, FREQ_FS, FREQ_G, FREQ_A, FREQ_B },

	{ /* Fm  */ FREQ_C, FREQ_CS, FREQ_DS, FREQ_F, FREQ_G, FREQ_GS, FREQ_AS },
	{ /* F#m */ FREQ_CS, FREQ_D, FREQ_E, FREQ_FS, FREQ_GS, FREQ_A, FREQ_B },

	{ /* Gm  */ FREQ_C, FREQ_D, FREQ_DS, FREQ_F, FREQ_G, FREQ_A, FREQ_AS },
	{ /* G#m */ FREQ_CS, FREQ_DS, FREQ_E, FREQ_FS, FREQ_GS, FREQ_AS, FREQ_B },

	{ /* A   */ FREQ_CS, FREQ_D, FREQ_E, FREQ_FS, FREQ_GS, FREQ_A, FREQ_B },

	{ /* B   */ FREQ_CS, FREQ_DS, FREQ_E, FREQ_FS, FREQ_GS, FREQ_AS, FREQ_B }
};

char *scale_name[NUM_SCALES] =
{
	"C / A minor", "C minor / D#", "C# / A# minor", "C# minor / E",
	"D / B minor", "D minor / F", "D# minor / F#",
	"E minor / G",
	"F minor / G#", "F# minor",
	"G minor / A#", "G# minor",
	"A", "B"
};

char *scale_short_name[NUM_SCALES] =
{
	"C","Cm","C#","C#m",
	"D","Dm","D#m",
	"Em",
	"Fm","F#m",
	"G#","G#m",
	"A","B"
};

char scale_letter[NUM_SCALES][OCTAVE] =
{
	"CDEFGAB", /* C / A minor */
	"CDDFGGA", /* C minor / D# */
	"CCDFFGA", /* C# / A# minor */
	"CDEFGAB", /* C# minor / E / F# minor */

	"CDEFGAB", /* D / B minor */
	"CDEFGAA", /* D minor / F */
	"CDFFGAB", /* D# minor / F# */
	
	"CDEFGAB", /* E minor */

	"CCDFGGA", /* F minor */
	"CDEFGAB", /* F# minor */

	"CDDFGAA", /* G minor / A# */
	"CDEFGAB", /* G# minor */

	"CDEFGAB", /* A  */
	"CDEFGAB"  /* B  */
};

#else

extern double base_octave[NUM_SCALES][OCTAVE];
extern char *scale_name[NUM_SCALES];
extern char *scale_short_name[NUM_SCALES];
extern char scale_letter[NUM_SCALES][OCTAVE];
#endif

int note_freq_scale[NUM_SCALES][NUM_NOTES];
int *note_freq;
int middle_c;

u_int dft_freq_level[MAX_UCHAR];

double freq_range_mult;
double key_spacing;
double key_char_add;
int key_cnt;

/* Effects order swapping */
struct st_swap_pos
{
	int pos1;
	int pos2;
};

#ifdef MAINFILE
char *effects_long_name[NUM_SWAP_EFFECTS] =
{
	"ALIASING",
	"REFLECTION",
	"PHASING",
	"FILTER",
	"RESONANCE",
	"DISTORTION",
	"RING MODULATION"
};

char *effects_short_name[NUM_SWAP_EFFECTS] =
{
	"AL","RT","PH","FI","RE","DI","RI"
};

struct st_swap_pos effects_pos[NUM_SWAP_EFFECTS] =
{
	{ 0, 1 },
	{ 1, 2 },
	{ 2, 3 },
	{ 3, 4 },
	{ 4, 5 },
	{ 5, 6 },
	{ 6, 0 }
};
#else
extern char *effects_long_name[NUM_SWAP_EFFECTS];
extern char *effects_short_name[NUM_SWAP_EFFECTS];
extern struct st_swap_pos effects_pos[NUM_SWAP_EFFECTS];
#endif

/* Non shared mem patch params used by main process that need to be saved to
   disk. */
struct st_params
{
	/* Only use for storing in a patch file */
	u_short win_height;

	/* Bitfields */
	u_char hold_note:1;
	u_char show_analyser:1;
	u_char do_decay:1;         /* No longer used */
	u_char subs_follow_main:1;
	u_char fill_waveform:1;

	u_char freq_range;
	char   key_start_note; /* Can go negative */
	u_char analyser_range;

	/* Location of former parameters */
	u_char spare1[6];

	u_char effects_swap;

	/* For future use */
	u_char spare2[2];
} params;

	
/* Shared mem control params for child/sound process */
struct st_sharmem
{
	u_short win_width;
	u_short freq;

	/* volatile stops GCC optimising spin loops on this down to while(1) */
	volatile u_char child_status;

	/* Bitmap flags */
	u_char play:1;
	u_char buffer_reset:1;
	u_char highpass_filter:1;
	u_char echo_clear:1;
	u_char echo_invert:1;
	u_char echo_highpass_filter:1;
	u_char sub1_note_offset:1;
	u_char sub2_note_offset:1;

	u_char sound;
	u_char note_scale;
	u_char freq_mode;
	u_char note;
	u_char vib_sweep;
	u_char vib_lfo;
	u_char volume;
	u_char attack;
	u_char decay;
	u_char filter_val;
	u_char filter_sweep;
	u_char filter_lfo;
	u_char echo_len;
	u_char echo_decay;
	u_char echo_filter;
	u_char chord;
	u_char arp_seq;
	u_char arp_delay;
	u_char arp_spacing;
	u_char arp_mod;
	u_char sub1_sound;
	u_char sub1_offset;
	u_char sub1_vol;
	u_char sub2_sound;
	u_char sub2_offset;
	u_char sub2_vol;
	u_char fm_harm_offset;
	u_char fm_mult1;
	u_char fm_mult2;
	u_char fm_offset;
	u_char fm_volume1;
	u_char fm_volume2;
	u_char phasing_mode;
	u_char phasing_offset;
	u_char phasing_sweep;
	u_char phasing_lfo;
	u_char phaser_freq_sep;
	u_char distortion;
	u_char res_mode;
	u_char res_level;
	u_char res_freq;
	u_char res_damping;
	u_char sample_mod_cnt;
	u_char square_width;
	u_char phaser_low_off_mult;
	u_char aliasing;
	u_char reflect_level;
	u_char reflect_smoothing;
	u_char sine_cutoff;
	u_char sine_cutoff_lfo;
	u_char square_width_lfo;
	u_char glide_distance;
	u_char glide_velocity;

	/* New for 1.6.0 */
	u_char compress_start;
	u_char compress_exponent;
	u_char echo_stretch;

	/* New for 1.7.0 */
	u_char ring_range;
	u_char ring_mode;
	u_char ring_freq;
	u_char ring_level;
	u_char do_reset:1;

	/* New for 1.9.0 */
	u_char saw_flatten;
	u_char saw_flatten_lfo;
	u_char fm_wierd;

	/* For future use */
	u_char spare[3];

	u_char effects_seq[NUM_SWAP_EFFECTS];

	short sndbuff[SNDBUFF_SIZE];
} *shm;

#define RANDOMISE_START &shm->note_scale
#define RANDOMISE_END   &shm->fm_wierd

enum en_status
{
	STAT_STOPPED,
	STAT_STARTING,
	STAT_RUN,
	STAT_PAUSE,
	STAT_PAUSED
};


enum en_freq_mode
{
	FREQ_NOTES,
	FREQ_STEPPED,
	FREQ_CONT,

	NUM_FREQ_MODES
};


enum en_sound_type
{
	SND_OFF,
	SND_SINE_FM,
	SND_SINE,
	SND_SQUARE,
	SND_TRIANGLE,
	SND_SAWTOOTH,
	SND_AAH,
	SND_OOH,
	SND_NOISE,
	SND_SAMPLE,

	NUM_SND_TYPES
};

/* Sub oscillators can't do sampling */
#define NUM_SUB_TYPES SND_SAMPLE


enum en_arp_seq
{
	ARP_OFF,

	ARP_2,

	ARP_3_UP_A,
	ARP_3_DOWN_A,
	ARP_3_UP_B,
	ARP_3_DOWN_B,
	ARP_3_UP_DOWN_A,
	ARP_3_UP_DOWN_B,

	ARP_4_UP,
	ARP_4_DOWN,
	ARP_4_UP_INTERLEAVED,
	ARP_4_DOWN_INTERLEAVED,
	ARP_4_UP_DOWN,

	ARP_8_UP,
	ARP_8_DOWN,
	ARP_8_UP_DOWN,

	NUM_ARP_SEQS
};


enum en_chord_type
{
	CHORD_OFF,

	CHORD_1_3,
	CHORD_1_4,
	CHORD_1_5,
	CHORD_1_6,
	CHORD_1_8,

	CHORD_1_2_5,
	CHORD_1_2_6,
	CHORD_1_3_5,
	CHORD_1_3_6,
	CHORD_1_3_8,
	CHORD_1_4_6,
	CHORD_1_4_7,
	CHORD_1_4_8,
	CHORD_1_5_8,
	CHORD_1_6_8,

	NUM_CHORDS
};


enum en_resonance
{
	RES_OFF,
	RES_INDEPENDENT_FREQ,
	RES_FOLLOW_MAIN_FREQ,

	NUM_RES_MODES
};


enum en_phasing_mode
{
	PHASING_OFF,
	PHASING_FLANGER_ADD,
	PHASING_FLANGER_SUB,
	PHASING_DIRTY_ADD,
	PHASING_DIRTY_SUB,
	PHASING_PHASER_ADD,
	PHASING_PHASER_SUB,
	PHASING_FM_FLANGER,

	NUM_PHASING_MODES
};


enum en_ring_range
{
	RING_OFF,
	RING_1,
	RING_2,
	RING_3,
	RING_4,
	RING_5,
	RING_6,
	RING_7,
	RING_8,
	RING_9,
	RING_10,

	NUM_RING_RANGES
};

#define MAX_RING_FREQ (NUM_UCHARS * (NUM_RING_RANGES - 1) - 1)

enum en_ring_mode
{
	RING_SINE,
	RING_SQUARE,
	RING_TRIANGLE,
	RING_SAWTOOTH,

	NUM_RING_MODES
};


#ifdef MAINFILE
char *sound_name[NUM_SND_TYPES] =
{
	"OFF",
	"SINE FM",
	"SINE",
	"SQUARE",
	"TRIANGLE",
	"SAWTOOTH",
	"AAH",
	"OOH",
	"NOISE",
	"SAMPLE"
};

char *chord_name[NUM_CHORDS] =
{
	"OFF",
	"1-3",
	"1-4",
	"1-5",
	"1-6",
	"1-8",
	"1-2-5",
	"1-2-6",
	"1-3-5",
	"1-3-6",
	"1-3-8",
	"1-4-6",
	"1-4-7",
	"1-4-8",
	"1-5-8",
	"1-6-8"
};

char *arp_seq_name[NUM_ARP_SEQS] =
{
	"OFF",
	"2 NOTES",
	"3 NOTES UP A",
	"3 NOTES DOWN A",
	"3 NOTES UP B",
	"3 NOTES DOWN B",
	"3 NOTES UP DOWN A",
	"3 NOTES UP DOWN B",
	"4 NOTES UP",
	"4 NOTES DOWN",
	"4 NOTES UP DOWN",
	"4 NOTES UP INTERLEAVED",
	"4 NOTES DOWN INTERLEAVED",
	"OCTAVE UP",
	"OCTAVE DOWN",
	"OCTAVE UP DOWN"
};

char *resonance_mode[NUM_RES_MODES] =
{
	"OFF",
	"INDEPENDENT FREQ",
	"FOLLOW MAIN FREQ"
};

char *phasing_mode[NUM_PHASING_MODES] =
{
	"OFF",
	"ADDITIVE FLANGER",
	"SUBTRACTIVE FLANGER",
	"ADDITIVE DIRTY FLANGER",
	"SUBTRACTIVE DIRTY FLANGER",
	"ADDITIVE PHASER",
	"SUBTRACTIVE PHASER",
	"FM FLANGER"
};

char *freq_mode[NUM_FREQ_MODES] =
{
	"NOTES",
	"STEPPED",
	"CONTINUOUS"
};

char *ring_range[NUM_RING_RANGES] =
{
	"OFF",
	"RANGE 1",
	"RANGE 2",
	"RANGE 3",
	"RANGE 4",
	"RANGE 5",
	"RANGE 6",
	"RANGE 7",
	"RANGE 8",
	"RANGE 9",
	"RANGE 10"
};

char *ring_mode[NUM_RING_MODES] =
{
	"SINE",
	"SQUARE",
	"TRIANGLE",
	"SAWTOOTH"
};

char *onoff[2] = { "OFF","ON" };

#else
extern char *sound_name[NUM_SND_TYPES];
extern char *chord_name[NUM_CHORDS];
extern char *arp_seq_name[NUM_ARP_SEQS];
extern char *resonance_mode[NUM_RES_MODES];
extern char *phasing_mode[NUM_PHASING_MODES];
extern char *freq_mode[NUM_FREQ_MODES];
extern char *ring_range[NUM_RING_RANGES];
extern char *ring_mode[NUM_RING_MODES];
extern char *onoff[2];
#endif

/* General globals */
char build_options[100];
char sound_system[20];
double boot_time;

/******************************* RECORDER ************************************/

enum en_recorder_state_flags
{
	RECORDER_STOPPED,
	RECORDER_RECORD,
	RECORDER_PLAY,
	RECORDER_PLAY_PAUSED,

	NUM_RECORDER_STATES 
} evr_state;

/* Could use a union here with mouse and key specific members, but it would
   only save 4 bytes (2 * short). Not worth the hassle. */
struct st_recorder_event
{
	uint32_t time_offset;
	uint32_t ksym;
	short x;
	short y;
	u_char type;
	char key_mb;  /* Dual key and mouse button */
	u_char spare;  /* For future use */
} *evr_events;

struct st_recorder_flags
{
	u_char loop:1;
	u_char patch_reset:1;
	u_char events_in_save:1;
} evr_flags;

u_int evr_events_cnt;
u_int evr_next_play_event;
struct st_params evr_params;
struct st_sharmem evr_sharmem;

#ifdef MAINFILE
char *evr_state_name[NUM_RECORDER_STATES] =
{
	"Stopped",
	"Record",
	"Play",
	"Play paused"
};
#else
extern char *evr_state_name[NUM_RECORDER_STATES];
#endif

/********************* Patch disk LOAD/SAVE control **********************/

enum en_disk
{
	DISK_NO_OP,
	DISK_LOAD_PATCH,
	DISK_SAVE_PATCH,
	DISK_LOAD_PROG,
	DISK_SAVE_PROG
};

struct st_disk
{
	enum en_disk op;
	int pos;
	/* PATH_MAX because it can include the home directory as a ~ */
	char filename[PATH_MAX+1];
} disk;
	

/********************** Text & character definitions *************************/

#define MAX_MESGS 20
#define CHAR_SIZE 10  /* Should be 11 , my mistake. Too late to fix now. */
#define CHAR_GAP  5

struct st_mesg
{
	char *text;
	int len;
	double mesg_time;
	double y;
	double col;
	double x_scale;
	double y_scale;
} mesg[MAX_MESGS];

int next_mesg;

typedef struct 
{
	int cnt;
	XPoint data[1];
} st_char_template;

st_char_template *ascii_table[NUM_UCHARS];

#ifdef MAINFILE
struct st_space { int cnt; } char_space = { 0 };

/*** LETTERS ***/

struct st_a { int cnt; XPoint data[16]; } char_a =
{
	16,
	{{ 9,8 }, { 7,10 },
	{ 7,10 }, { 2,10 },
	{ 2,10 }, { 0,9 },
	{ 0,9 }, { 0,5 },
	{ 0,5 }, { 2,4 },
	{ 2,4 }, { 9,4 },
	{ 9,3 }, { 9,8 },
	{ 9,8 }, { 10,10 }}
};

struct st_A { int cnt; XPoint data[6]; } char_A = 
{
	6, 
	{{ 0,10 }, { 5,0 },
	{ 5,0 }, { 10,10 },
	{ 2,6 }, { 8,6 }}
};

struct st_b { int cnt; XPoint data[14]; } char_b =
{
	14,
	{{ 0,0 }, { 0,10 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 8,10 },
	{ 8,10 }, { 10,9 },
	{ 10,9 }, { 10,5 },
	{ 10,5 }, { 8,4 },
	{ 8,4 }, { 0,4 }}
};

struct st_B { int cnt; XPoint data[20]; } char_B = 
{
	20,
	{{ 0,0 }, { 8,0 },
	{ 8,0 }, { 10,1 },
	{ 10,1 }, { 10,4 },
	{ 10,4 }, { 5,5 },
	{ 5,5 }, { 10,6 },
	{ 10,6 }, { 10,9 },
	{ 10,9 }, { 8,10 },
	{ 8,10 }, { 0,10 },
	{ 0,10 }, { 0,0 },
	{ 0,5 }, { 5,5 }}
};

struct st_c { int cnt; XPoint data[14]; } char_c = 
{
	14,
	{{10,5 } , { 9,4 },
	{ 9,4 } , { 2,4 },
	{ 2,4 }, { 0,5 },
	{ 0,5 }, { 0,9 },
	{ 0,9 }, { 2,10 },
	{ 2,10 }, { 9,10 },
	{ 9,10 }, { 10,9 }}
};

struct st_C { int cnt; XPoint data[14]; } char_C = 
{
	14,
	{{10,1 }, { 9,0 },
	{ 9,0 }, { 2,0 },
	{ 2,0 }, { 0,2 },
	{ 0,2 }, { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 9,10 },
	{ 9,10 }, { 10,9 }}
};

struct st_d { int cnt; XPoint data[14]; } char_d = 
{
	14,
	{{ 10,0 }, { 10,10 },
	{ 10,8 }, { 8,10 },
	{ 8,10 }, { 2,10 },
	{ 2,10 }, { 0,9 },
	{ 0,9 }, { 0,5 },
	{ 0,5 }, { 2,4 },
	{ 2,4 }, { 10,4 }}
};

struct st_D { int cnt; XPoint data[12]; } char_D = 
{
	12,
	{{ 0,0 }, { 7,0 },
	{ 7,0 }, { 10,2 },
	{ 10,2 }, { 10,8 },
	{ 10,8 }, { 7,10 },
	{ 7,10 }, { 0,10 },
	{ 0,10 }, { 0,0 }}
};

struct st_e { int cnt; XPoint data[16]; } char_e = 
{
	16,
	{{ 0,9 }, { 0,5 },
	{ 0,5 }, { 2,4 },
	{ 2,4 }, { 8,4 },
	{ 8,4 }, { 10,5 },
	{ 10,5 }, { 10,7 },
	{ 10,7 }, { 0,7 },
	{ 0,9 }, { 2,10 },
	{ 2,10 },{ 10,10 }}
};

struct st_E { int cnt; XPoint data[8]; } char_E = 
{
	8,
	{{ 0,0 }, { 10,0 },
	{ 0,0 } , { 0,10 },
	{ 0,5 }, { 7,5 },
	{ 0,10 }, { 10,10 }}
};

struct st_f { int cnt; XPoint data[10]; } char_f =
{
	10,
	{{ 0,10 }, { 0,4 },
	{ 0,4 }, { 2,2 },
	{ 2,2 }, { 4,1 },
	{ 4,1 }, { 10,1 },
	{ 0,5 }, { 9,5 }}
};

struct st_F { int cnt; XPoint data[6]; } char_F =
{
	6,
	{{ 0,0 } , { 10,0 },
	{ 0,0 }, { 0,10 },
	{ 0,5 }, { 7,5 }}
};

struct st_g { int cnt; XPoint data[22]; } char_g =
{ 
	22,
	{{ 0,4 }, { 2,3 },
	{ 2,3 }, { 8,3 },
	{ 8,3 }, { 10,3 },
	{ 10,3 }, { 10,6 },
	{ 10,6 }, { 8,7 },
	{ 8,7 }, { 2,7 },
	{ 2,7 }, { 0,6 },
	{ 0,6 }, { 0,4 },
	{ 10,5 }, { 10,9 },
	{ 10,9 }, { 8,10 },
	{ 8,10 }, { 3,10 }}
};

struct st_G { int cnt; XPoint data[18]; } char_G =
{ 
	18,
	{{ 10,2 } , { 7,0 },
	{ 7,0 }, { 2,0 },
	{ 2,0 }, { 0,2 },
	{ 0,2 }, { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 7,10 },
	{ 7,10 }, { 10,7 },
	{ 10,7 }, { 10,5 },
	{ 10,5 }, { 5,5 }}
};

struct st_h { int cnt; XPoint data[10]; } char_h =
{
	10,
	{{ 0,0 }, { 0,10 },
	{ 0,6 }, { 2,5 },
	{ 2,5 }, { 8,5 },
	{ 8,5 }, { 10,6 },
	{ 10,6 }, { 10,10 }}
};

struct st_H { int cnt; XPoint data[6]; } char_H =
{
	6,
	{{ 0,0 }, { 0,10 },
	{ 0,5 }, { 10,5 },
	{ 10,0 }, { 10,10 }}
};

struct st_i { int cnt; XPoint data[6]; } char_i =
{
	6,
	{{ 5,0 }, { 5,2 },
	{ 5,5 }, { 5,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_I { int cnt; XPoint data[6]; } char_I =
{
	6,
	{{ 0,0 }, { 10,0 },
	{ 5,0 }, { 5,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_j { int cnt; XPoint data[12]; } char_j =
{
	10, 
	{{ 8,2 }, { 8,7 },
	{ 8,7 }, { 5,10 },
	{ 5,10 }, { 2,10 },
	{ 2,10 }, { 1,8 },
	{ 1,8 }, { 0,7 }}
};

struct st_J { int cnt; XPoint data[12]; } char_J =
{
	12, 
	{{ 0,0 }, { 10,0 },
	{ 7,0 }, { 7,7 },
	{ 7,7 }, { 5,10 },
	{ 5,10 }, { 2,10 },
	{ 2,10 }, { 1,8 },
	{ 1,8 }, { 1,6 }}
};

struct st_k { int cnt; XPoint data[6]; } char_k =
{
	6,
	{{ 0,1 }, { 0,10 },
	{ 0,7 }, { 8,5 },
	{ 0,7 }, { 8,10 }}
};

struct st_K { int cnt; XPoint data[6]; } char_K =
{
	6,
	{{ 0,0 }, { 0,10 },
	{ 0,7 }, { 10,0 },
	{ 3,5 }, { 10,10 }}
};

struct st_l { int cnt; XPoint data[8]; } char_l =
{
	8,
	{{ 0,1 }, { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 7,10 },
	{ 7,10 }, { 9,8 }}
};

struct st_L { int cnt; XPoint data[4]; } char_L =
{
	4,
	{{ 0,0 }, { 0,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_m { int cnt; XPoint data[12]; } char_m =
{
	12,
	{{ 0,10 }, { 0,6 },
	{ 0,6 }, { 1,4 },
	{ 1,4 }, { 9,4 },
	{ 9,4 }, { 10,6 },
	{ 10,6 }, { 10,10 },
	{ 5,4 }, { 5,10 }}
};

struct st_M { int cnt; XPoint data[8]; } char_M =
{
	8,
	{{ 0,0 } , { 0,10 },
	{ 0,0 }, { 5,5 },
	{ 5,5 }, { 10,0 },
	{ 10,0 }, { 10,10 }}
};

struct st_n { int cnt; XPoint data[10]; } char_n =
{
	10,
	{{ 0,10 }, { 0,6 },
	{ 0,6 }, { 1,4 },
	{ 1,4 }, { 9,4 },
	{ 9,4 }, { 10,6 },
	{ 10,6 }, { 10,10 }}
};
	
struct st_N { int cnt; XPoint data[6]; } char_N =
{
	6,
	{{ 0,0 }, { 0,10 },
	{ 0,0 }, { 10,10 },
	{ 10,10 }, { 10,0 }}
};

struct st_o { int cnt; XPoint data[16]; } char_o =
{
	16,
	{{ 3,4 }, { 7,4 },
	{ 7,4 }, { 10,6 },
	{ 10,6 }, { 10,8 },
	{ 10,8 }, { 7,10 },
	{ 7,10 }, { 3,10 },
	{ 3,10 }, { 0,8 },
	{ 0,8 }, { 0,6 },
	{ 0,6 }, { 3,4 }}
};

struct st_O { int cnt; XPoint data[16]; } char_O =
{
	16,
	{{ 3,0 }, { 7,0 },
	{ 7,0 }, { 10,3 },
	{ 10,3 }, { 10,7 },
	{ 10,7 }, { 7,10 },
	{ 7,10 }, { 3,10 },
	{ 3,10 }, { 0,7 },
	{ 0,7 }, { 0,3 },
	{ 0,3 }, { 3,0 }}
};

struct st_p { int cnt; XPoint data[12]; } char_p =
{
	12,
	{{ 0,3 }, { 7,3 },
	{ 7,3 }, { 8,4 },
	{ 8,4 }, { 8,5 },
	{ 8,5 }, { 7,7 },
	{ 7,7 }, { 0,7 },
	{ 0,3 }, { 0,10 }}
};

struct st_P { int cnt; XPoint data[12]; } char_P =
{
	12,
	{{ 0,0 }, { 7,0 },
	{ 7,0 }, { 10,2 },
	{ 10,2 }, { 10,4 },
	{ 10,4 }, { 7,6 },
	{ 7,6 }, { 0,6 },
	{ 0,0 }, { 0,10 }}
};

struct st_q { int cnt; XPoint data[16]; } char_q =
{
	16,
	{{ 0,3 }, { 2,2 },
	{ 2,2 }, { 6,2 },
	{ 6,2 }, { 6,7 },
	{ 6,7 }, { 2,7 },
	{ 2,7 }, { 0,6 },
	{ 0,6 }, { 0,3 },
	{ 6,7 }, { 6,10 },
	{ 6,10 }, { 10,6 }}
};

struct st_Q { int cnt; XPoint data[18]; } char_Q =
{
	18,
	{{ 3,0 }, { 7,0 },
	{ 7,0 }, { 10,3 },
	{ 10,3 }, { 10,7 },
	{ 10,7 }, { 7,10 },
	{ 7,10 }, { 3,10 },
	{ 3,10 }, { 0,7 },
	{ 0,7 }, { 0,3 },
	{ 0,3 }, { 3,0 },
	{ 0,10 }, { 4,6 }}
};

struct st_r { int cnt; XPoint data[14]; } char_r =
{
	8,
	{{ 0,10 }, { 0,6 },
	{ 0,6 }, { 1,4 },
	{ 1,4 }, { 9,4 },
	{ 9,4 }, { 10,5 }}
};

struct st_R { int cnt; XPoint data[14]; } char_R =
{
	14,
	{{ 0,0 }, { 7,0 },
	{ 7,0 }, { 10,2 },
	{ 10,2 }, { 10,4 },
	{ 10,4 }, { 7,6 },
	{ 7,6 }, { 0,6 }, 
	{ 0,0 }, { 0,10 },
	{ 3,6 }, { 9,10 }}
};

struct st_s { int cnt; XPoint data[20]; } char_s =
{
	20,
	{{ 10,5 }, { 9,4 },
	{ 9,4 }, { 1,4 },
	{ 1,4 }, { 0,5 },
	{ 0,5 }, { 1,7 },
	{ 1,7 }, { 9,7 },
	{ 9,7 }, { 10,8 },
	{ 10,8 }, { 10,9 },
	{ 10,9 }, { 9,10 }, 
	{ 9,10 }, { 1,10 },
	{ 1,10 }, { 0,9 }}
};

struct st_S { int cnt; XPoint data[22]; } char_S =
{
	22,
	{{ 10,1 }, { 9,0 },
	{ 9,0 }, { 1,0 },
	{ 1,0 }, { 0,1 },
	{ 0,1 }, { 0,4 }, 
	{ 0,4 }, { 1,5 },
	{ 1,5 }, { 9,5 },
	{ 9,5 }, { 10,6 },
	{ 10,6 }, { 10,9 },
	{ 10,9 }, { 9,10 }, 
	{ 9,10 }, { 1,10 },
	{ 1,10 }, { 0,9 }}
};

struct st_t { int cnt; XPoint data[10]; } char_t =
{
	10,
	{{ 0,1 }, { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 7,10 },
	{ 7,10 }, { 9,8 },
	{ 0,4 }, { 7,4 }}
};

struct st_T { int cnt; XPoint data[4]; } char_T =
{
	4,
	{{ 0,0 }, { 10,0 },
	{ 5,0 }, { 5,10 }}
};

struct st_u { int cnt; XPoint data[10]; } char_u =
{
	10,
	{{ 0,4 } , { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 8,10 },
	{ 8,10 }, { 10,8 },
	{ 10,8 }, { 10,4 }}
};

struct st_U { int cnt; XPoint data[10]; } char_U =
{
	10,
	{{ 0,0 } , { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 8,10 },
	{ 8,10 }, { 10,8 },
	{ 10,8 }, { 10,0 }}
};

struct st_v { int cnt; XPoint data[4]; } char_v =
{
	4,
	{{ 0,4 }, { 5,10 },
	{ 5,10 }, { 10,4 }}
};

struct st_V { int cnt; XPoint data[4]; } char_V =
{
	4,
	{{ 0,0 }, { 5,10 },
	{ 5,10 }, { 10,0 }}
};

struct st_w { int cnt; XPoint data[8]; } char_w =
{
	8,
	{{ 0,4 }, { 2,10 },
	{ 2,10 }, { 5,6 },
	{ 5,6 }, { 8,10 },
	{ 8,10 }, { 10,4 }}
};

struct st_W { int cnt; XPoint data[8]; } char_W =
{
	8,
	{{ 0,0 }, { 2,10 },
	{ 2,10 }, { 5,4 },
	{ 5,4 }, { 8,10 },
	{ 8,10 }, { 10,0 }}
};

struct st_x { int cnt; XPoint data[4]; } char_x =
{
	4,
	{{ 0,4 }, { 10,10 },
	{ 10,4 }, { 0,10 }}
};

struct st_X { int cnt; XPoint data[4]; } char_X =
{
	4,
	{{ 0,0 }, { 10,10 },
	{ 10,0 }, { 0,10 }}
};

struct st_y { int cnt; XPoint data[16]; } char_y =
{
	16,
	{{ 0,4 }, { 0,6 },
	{ 0,6 }, { 1,7 },
	{ 1,7 }, { 9,7 },
	{ 9,7 }, { 10,5 },
	{ 10,5 }, { 10,4 },
	{ 10,5 }, { 10,8 },
	{ 10,8 }, { 9,10 },
	{ 9,10 }, { 3,10 }}
};

struct st_Y { int cnt; XPoint data[4]; } char_Y =
{
	4,
	{{ 0,0 }, { 6,4 },
	{ 10,0 }, { 2,10 }}
};

struct st_z { int cnt; XPoint data[6]; } char_z =
{
	6,
	{{ 0,4 }, { 10,4 },
	{ 10,4 }, { 0,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_Z { int cnt; XPoint data[6]; } char_Z =
{
	6,
	{{ 0,0 }, { 10,0 },
	{ 10,0 }, { 0,10 },
	{ 0,10 }, { 10,10 }}
};


/**** NUMBERS ***/

struct st_1 { int cnt; XPoint data[6]; } char_1 =
{
	6,
	{{ 2,3 }, { 5,0 },
	{ 5,0 }, { 5,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_2 { int cnt; XPoint data[12]; } char_2 =
{
	12,
	{{ 0,2 }, { 2,0 },
	{ 2,0 }, { 8,0 },
	{ 8,0 }, { 10,2 },
	{ 10,2 }, { 10,4 },
	{ 10,4 }, { 0,10 },
	{ 0,10 }, { 10,10 }}
};

struct st_3 { int cnt; XPoint data[16]; } char_3 =
{
	16,
	{{ 0,1 }, { 1,0 },
	{ 1,0 }, { 9,0 },
	{ 9,0 }, { 10,1 },
	{ 10,1 }, { 10,9 },
	{ 10,9 }, { 9,10 },
	{ 9,10 }, { 1,10 },
	{ 1,10 }, { 0,9 },
	{ 3,5 }, { 10,5 }}
};

struct st_4 { int cnt; XPoint data[6]; } char_4 =
{
	6,
	{{ 10,5 }, { 0,5 },
	{ 0,5 }, { 5,0 },
	{ 5,0 }, { 5,10 }}
};

struct st_5 { int cnt; XPoint data[14]; } char_5 =
{
	14,
	{{ 10,0 }, { 0,0 },
	{ 0,0 }, { 0,5 },
	{ 0,5 }, { 9,5 },
	{ 9,5 }, { 10,6 },
	{ 10,6 }, { 10,9 },
	{ 10,9 }, { 9,10 },
	{ 9,10 }, { 0,10 }}
};

struct st_6 { int cnt; XPoint data[18]; } char_6 =
{
	18,
	{{ 10,0 }, { 1,0 },
	{ 1,0 }, { 0,1 },
	{ 0,1 }, { 0,9 },
	{ 0,9 }, { 1,10 },
	{ 1,10 }, { 9,10 },
	{ 9,10 }, { 10,9 },
	{ 10,9 }, { 10,6 },
	{ 10,6 }, { 9,5 },
	{ 9,5 }, { 0,5 }}
};	

struct st_7 { int cnt; XPoint data[4]; } char_7 =
{ 
	4,
	{{ 0,0 }, { 10,0 },
	{ 10,0 }, { 2,10 }}
};

struct st_8 { int cnt; XPoint data[30]; } char_8 =
{
	30,
	{{ 1,0 }, { 9,0 },
	{ 9,0 }, { 10,1 },
	{ 10,1 }, { 10,4 },
	{ 10,4 }, { 9,5 },
	{ 9,5 }, { 10,6 },
	{ 10,6 }, { 10,9 },
	{ 10,9 }, { 9,10 },
	{ 9,10 }, { 1,10 },
	{ 1,10 }, { 0,9 },
	{ 0,9 }, { 0,6 },
	{ 0,6 }, { 1,5 },
	{ 1,5 }, { 0,4 },
	{ 0,4 }, { 0,1 },
	{ 0,1 }, { 1,0 }, 
	{ 1,5 }, { 9,5 }}
};

struct st_9 { int cnt; XPoint data[18]; } char_9 =
{
	18,
	{{ 1,10 }, { 9,10 },
	{ 9,10 }, { 10,9 },
	{ 10,9 }, { 10,1 },
	{ 10,1 }, { 9,0 },
	{ 9,0 }, { 1,0 },
	{ 1,0 }, { 0,1 },
	{ 0,1 }, { 0,4 },
	{ 0,4 }, { 1,5 },
	{ 1,5 }, { 10,5 }}
};

struct st_0 { int cnt; XPoint data[18]; } char_0 =
{
	18,
	{{ 3,0 }, { 7,0 },
	{ 7,0 }, { 10,3 },
	{ 10,3 }, { 10,7 },
	{ 10,7 }, { 7,10 }, 
	{ 7,10 }, { 3,10 },
	{ 3,10 }, { 0,7 },
	{ 0,7 }, { 0,3 },
	{ 0,3 }, { 3,0 },
	{ 3,10 }, { 7,0 }}
};

/*** PUNCTUATION ***/

struct st_qmark { int cnt; XPoint data[16]; } char_qmark =
{
	16,
	{{ 0,2 }, { 2,0 },
	{ 2,0 }, { 9,0 },
	{ 9,0 }, { 10,1 },
	{ 10,1 }, { 10,4 },
	{ 10,4 }, { 9,5 },
	{ 9,5 }, { 5,5 },
	{ 5,5 }, { 5,7 },
	{ 5,9 }, { 5,10 }}
};

struct st_exmark { int cnt; XPoint data[8]; } char_exmark =
{
	8,
	{{ 4,0 }, { 6,0 },
	{ 6,0 }, { 5,7 },
	{ 5,7 }, { 4,0 },
	{ 5,9 }, { 5,10 }}
};

struct st_plus { int cnt; XPoint data[4]; } char_plus =
{
	4,
	{{ 5,0 }, { 5,10 },
	{ 0,5 }, { 10,5 }}
};

struct st_minus { int cnt; XPoint data[2]; } char_minus =
{
	2,
	{{ 0,5 }, { 10,5 }}
};

struct st_star { int cnt; XPoint data[8]; } char_star =
{
	8,
	{{ 0,5 }, { 10,5 },
	{ 5,0 }, { 5,10 },
	{ 1,1 }, { 9,9 },
	{ 1,9 }, { 9,1 }}
};

struct st_equals { int cnt; XPoint data[4]; } char_equals =
{
	4,
	{{ 0,3 }, { 10,3 },
	{ 0,7 }, { 10,7 }}
};

struct st_dot { int cnt; XPoint data[10]; } char_dot =
{
	8,
	{{ 4,8 }, { 6,8 },
	{ 6,8 }, { 6,10 },
	{ 6,10 }, { 4,10 },
	{ 4,10 }, { 4,8 }}
};

struct st_comma { int cnt; XPoint data[4]; } char_comma =
{
	4,
	{{ 7,6 }, { 6,9 },
	{ 6,9 }, { 4,10 }}
};

struct st_lrbracket { int cnt; XPoint data[10]; } char_lrbracket =
{
	10,
	{{ 6,0 }, { 5,0 },
	{ 5,0 }, { 3,2 },
	{ 3,2 }, { 3,8 },
	{ 3,8 }, { 5,10 },
	{ 5,10 }, { 6,10 }}
};

struct st_rrbracket { int cnt; XPoint data[10]; } char_rrbracket =
{
	10,
	{{ 3,0 }, { 4,0 },
	{ 4,0 }, { 6,2 },
	{ 6,2 }, { 6,8 },
	{ 6,8 }, { 4,10 },
	{ 4,10 }, { 3,10 }}
};

struct st_lcbracket { int cnt; XPoint data[12]; } char_lcbracket =
{
	12,
	{{ 6,0 }, { 4,1 },
	{ 4,1 }, { 4,4 },
	{ 4,4 }, { 2,5 },
	{ 2,5 }, { 4,6 },
	{ 4,6 }, { 4,9 },
	{ 4,9 }, { 6,10 }}
};

struct st_rcbracket { int cnt; XPoint data[12]; } char_rcbracket =
{
	12,
	{{ 4,0 }, { 6,1 },
	{ 6,1 }, { 6,4 },
	{ 6,4 }, { 8,5 },
	{ 8,5 }, { 6,6 },
	{ 6,6 }, { 6,9 },
	{ 6,9 }, { 4,10 }}
};

struct st_lsbracket { int cnt; XPoint data[6]; } char_lsbracket =
{
	6,
	{{ 7,0 }, { 3,0 },
	{ 3,0 }, { 3,10 },
	{ 3,10 }, { 7,10 }}
};

struct st_rsbracket { int cnt; XPoint data[6]; } char_rsbracket =
{
	6,
	{{ 3,0 }, { 7,0 },
	{ 7,0 }, { 7,10 },
	{ 7,10 }, { 3,10 }}
};

struct st_dollar { int cnt; XPoint data[26]; } char_dollar =
{
	26,
	{{ 10,2 }, { 9,1 },
	{ 9,1 }, { 1,1 },
	{ 1,1 }, { 0,2 },
	{ 0,2 }, { 0,4 }, 
	{ 0,4 }, { 1,5 },
	{ 1,5 }, { 9,5 },
	{ 9,5 }, { 10,6 },
	{ 10,6 }, { 10,8 },
	{ 10,8 }, { 9,9 }, 
	{ 9,9 }, { 1,9 },
	{ 1,9 }, { 0,8 },
	{ 4,0 }, { 4,10 },
	{ 6,0 }, { 6,10 }}
};

struct st_hash { int cnt; XPoint data[8]; } char_hash =
{
	8,
	{{ 0,3 }, { 10,3 },
	{ 0,7 }, { 10,7 },
	{ 3,0 }, { 3,10 },
	{ 7,0 }, { 7,10 }}
};

struct st_fslash { int cnt; XPoint data[4]; } char_fslash =
{
	2,
	{{ 10,0 }, { 0,10 }}
};

struct st_bslash { int cnt; XPoint data[4]; } char_bslash =
{
	2,
	{{ 0,0 }, { 10,10 }}
};

struct st_less { int cnt; XPoint data[4]; } char_less =
{
	4,
	{{ 10,0 }, { 0,5 },
	{ 0,5 }, { 10,10 }}
};

struct st_greater { int cnt; XPoint data[4]; } char_greater =
{
	4,
	{{ 0,0 }, { 10,5 },
	{ 10,5 }, { 0,10 }}
};

struct st_underscore { int cnt; XPoint data[2]; } char_underscore =
{
	2,
	{{ 0,10 }, { 10,10 }}
};

struct st_bar { int cnt; XPoint data[2]; } char_bar =
{
	2,
	{{ 5,0 }, { 5,10 }}
};

struct st_squote { int cnt; XPoint data[2]; } char_squote =
{
	2,
	{{ 6,0 }, { 4,3 }}
};

struct st_dquote { int cnt; XPoint data[4]; } char_dquote =
{
	4,
	{{ 3,0 }, { 3,2 },
	{ 7,0 }, { 7,2 }}
};

struct st_bquote { int cnt; XPoint data[2]; } char_bquote =
{
	2,
	{{ 4,0 }, { 6,3 }}
};

struct st_colon { int cnt; XPoint data[16]; } char_colon =
{
	16,
	{{ 4,1 }, { 6,1 },
	{ 6,1 }, { 6,3 },
	{ 6,3 }, { 4,3 },
	{ 4,3 }, { 4,1 },
	{ 4,7 }, { 6,7 },
	{ 6,7 }, { 6,9 },
	{ 6,9 }, { 4,9 },
	{ 4,9 }, { 4,7 }}
};

struct st_semicolon { int cnt; XPoint data[12]; } char_semicolon =
{
	12,
	{{ 4,1 }, { 6,1 },
	{ 6,1 }, { 6,3 },
	{ 6,3 }, { 4,3 },
	{ 4,3 }, { 4,1 },
	{ 6,6 }, { 6,8 },
	{ 6,8 }, { 4,10 }}
};	

struct st_at { int cnt; XPoint data[36]; } char_at =
{
	36,
	{{ 7,6 }, { 7,4 },
	{ 7,4 }, { 6,3 },
	{ 6,3 }, { 4,3 },
	{ 4,3 }, { 3,4 },
	{ 3,4 }, { 3,6 },
	{ 3,6 }, { 4,7 },
	{ 4,7 }, { 6,7 },
	{ 6,7 }, { 7,6 },
	{ 7,6 }, { 8,7 },
	{ 8,7 }, { 9,6 },
	{ 9,6 }, { 9,2 },
	{ 9,2 }, { 7,0 },
	{ 7,0 }, { 2,0 },
	{ 2,0 }, { 0,2 },
	{ 0,2 }, { 0,8 },
	{ 0,8 }, { 2,10 },
	{ 2,10 }, { 8,10 },
	{ 8,10 }, { 10,8 }}
};

struct st_hat { int cnt; XPoint data[4]; } char_hat =
{
	4,
	{{ 5,0 }, { 1,5 },
	{ 5,0 }, { 9,5 }}
};

struct st_tilda { int cnt; XPoint data[6]; } char_tilda =
{
	6,
	{{ 1,6 }, { 4,4 },
	{ 4,4 }, { 7,6 },
	{ 7,6 }, { 10,4 }}
};

struct st_ampersand { int cnt; XPoint data[26]; } char_ampersand =
{
	26,
	{{ 9,9 }, { 8,10 },
	{ 8,10 }, { 1,3 },
	{ 1,3 }, { 1,1 }, 
	{ 1,1 }, { 2,0 },
	{ 2,0 }, { 5,0 },
	{ 5,0 }, { 6,1 },
	{ 6,1 }, { 6,3 },
	{ 6,3 }, { 0,7 },
	{ 0,7 }, { 0,9 },
	{ 0,9 }, { 1,10 }, 
	{ 1,10 }, { 6,10 },
	{ 6,10 }, { 8,8 },
	{ 8,8 }, { 8,7 }}
};

struct st_percent { int cnt; XPoint data[18]; } char_percent =
{
	18,
	{{ 0,10 }, { 10,0 },

	{ 0,0 }, { 4,0 },
	{ 4,0 }, { 4,4 },
	{ 4,4 }, { 0,4 },
	{ 0,4 }, { 0,0 },

	{ 6,6 }, { 10,6 },
	{ 10,6 }, { 10,10 },
	{ 10,10 }, { 6,10 },
	{ 6,10 }, { 6,6 }}
};
#endif


/************************** Other global vars ************************/

/* X */
Display *display;
Window win;
Drawable drw;
GC gc[NUM_COLOURS+1];
int display_width;
int display_height;

/* Command line */
int do_sound;
int use_write_delay;
int write_delay;
int win_width;
int win_height;
int win_refresh;
int x_mode;
int events_debug;
int draw_tail;
char *basic_file;
char *basic_arg;

/* Draw */
char freq_text[20];
char sample_speed_text[20];
double g_x_scale;
double g_y_scale;
double g_avg_scale;
int freq_text_len;
int freq_col;
int intro_cnt;

/* Hover text */
char *hover_str;
double hover_x_scale;
int hover_x;
int hover_y;

/* Misc */
struct termios saved_tio;
int win_mapped;
int escape_cnt;
int load_prog_cnt;
int show_credits;
int show_section_status;
int first_mouse_button;
int ignore_mouse_release;
int do_messages;
int do_sampling;
int freeze_waveform;
char *home_dir;
DIR *list_dir;
char *patch_file;
char title_str[TITLE_MAX_LEN+1];
char dirbuff[PATH_MAX+1];
char effects_seq_str[EFFECTS_SEQ_STR_LEN];

/******************************** BASIC *******************************/

#define TAB               9
#define WORD_STR_REALLOC  5
#define WORD_LIST_REALLOC 20
#define RETURNS_ALLOC     5
#define YIELD_AFTER       10
#define BASIC_SUFFIX      ".bas"
#define NUM_PAD_TOKENS    3
#define NO_PROG_LOADED    "No program loaded"

#define IS_OP_TYPE(O,T) ((O)->token_type == TOK_OP && (O)->token_num == T)
#define IS_COM_TYPE(C,T) ((C)->token_type == TOK_COM && (C)->token_num == T)


enum en_result_type
{
	NO_RESULT,
	RESULT_NUM,
	RESULT_STR
};

enum en_token
{
	TOK_SECTION,
	TOK_VAR,
	TOK_COM,
	TOK_FUNC,
	TOK_OP,
	TOK_STR,
	TOK_NUM,
	TOK_LABEL,
	TOK_PADDING,

	NUM_TYPES
};


typedef struct st_value
{
	double val;
	char *strval;
} st_value;


/*** SECTIONS ***/
enum en_section
{
	SECTION_INIT,
	SECTION_DIAL,
	SECTION_BUTTON,
	SECTION_PLAY,
	SECTION_RELEASE,
	SECTION_SCALE,
	SECTION_KEYSTART,
	SECTION_FUNCKEY,
	SECTION_FILTER,
	SECTION_WINDOW,
	SECTION_EVENT,

	SECTION_TIMER0,
	SECTION_TIMER1,
	SECTION_TIMER2,
	SECTION_TIMER3,
	SECTION_TIMER4,

	SECTION_MAIN0,
	SECTION_MAIN1,
	SECTION_MAIN2,
	SECTION_MAIN3,
	SECTION_MAIN4,

	NUM_SECTIONS
};

#define NUM_TIMERS (SECTION_MAIN0 - SECTION_TIMER0)

#define IS_TIMER_SECTION(S) ((S) >= SECTION_TIMER0 && (S) <= SECTION_TIMER4)
#define IS_MAIN_SECTION(S)  ((S) >= SECTION_MAIN0)

enum en_getset
{
	/* 0 */
	GS_YIELD,
	GS_FILTER,
	GS_FREQ,
	GS_NOTE,
	GS_SCALE,

	/* 5 */
	GS_PLAY,
	GS_MESG,
	GS_IO_MESG,
	GS_TITLE,
	GS_KEY_START_NOTE,

	/* 10 */
	GS_MAIN_OSC,
	GS_MOUSE_TAIL,
	GS_FREEZE,
	GS_FILL,
	GS_DIR,

	/* 15 */
	GS_TIMER_CATCHUP,
	GS_EFFECTS_SEQ,
	GS_WIN_WIDTH,
	GS_WIN_HEIGHT,

	NUM_GETSET_FIELDS
};


typedef struct 
{
	char *name;
	int start_loc;
	int read_loc;
	int restore_loc;
	int runnable;

	/* For MAIN section only */
	int pc;

	/* For TIMER section only */
	double timer_interval; 

	/* For MAIN & TIMER sections. double so 8 bytes on 64 or 
	   32 bit systems */
	double sleep_until; 

	/* For GOSUB */
	int returns_alloc;
	int num_returns;
	int *return_loc;  /* Array of return locations for nested gosubs */
} st_section;

#ifdef MAINFILE
st_section section[NUM_SECTIONS] = 
{
	{ "INIT",    0,0,0,0,0,0,0,0,0,NULL },
	{ "DIAL",    0,0,0,0,0,0,0,0,0,NULL },
	{ "BUTTON",  0,0,0,0,0,0,0,0,0,NULL },
	{ "PLAY",    0,0,0,0,0,0,0,0,0,NULL },
	{ "RELEASE", 0,0,0,0,0,0,0,0,0,NULL },
	{ "SCALE",   0,0,0,0,0,0,0,0,0,NULL },
	{ "KEYSTART",0,0,0,0,0,0,0,0,0,NULL },
	{ "FUNCKEY", 0,0,0,0,0,0,0,0,0,NULL },
	{ "FILTER",  0,0,0,0,0,0,0,0,0,NULL },
	{ "WINDOW",  0,0,0,0,0,0,0,0,0,NULL },
	{ "EVENT",   0,0,0,0,0,0,0,0,0,NULL },
	{ "TIMER0",  0,0,0,0,0,0,0,0,0,NULL },
	{ "TIMER1",  0,0,0,0,0,0,0,0,0,NULL },
	{ "TIMER2",  0,0,0,0,0,0,0,0,0,NULL },
	{ "TIMER3",  0,0,0,0,0,0,0,0,0,NULL },
	{ "TIMER4",  0,0,0,0,0,0,0,0,0,NULL },
	{ "MAIN0",   0,0,0,0,0,0,0,0,0,NULL },
	{ "MAIN1",   0,0,0,0,0,0,0,0,0,NULL },
	{ "MAIN2",   0,0,0,0,0,0,0,0,0,NULL },
	{ "MAIN3",   0,0,0,0,0,0,0,0,0,NULL },
	{ "MAIN4",   0,0,0,0,0,0,0,0,0,NULL }
};
#else
extern st_section section[NUM_SECTIONS];
#endif


/*** BASIC system variables. Update array in createSystemVariables() if this
     enum is updated ***/
enum en_system_vars
{
	/* 0 */
	NO_VAR,
	SVAR_VERSION,
	SVAR_BUILD_DATE,
	SVAR_BUILD_OPTIONS,
	SVAR_TIME,

	/* 5 */
	SVAR_UPTIME,
	SVAR_PI,
	SVAR_E,
	SVAR_NUM_SND_TYPES,
	SVAR_NUM_CHORDS,

	/* 10 */
	SVAR_NUM_ARP_SEQS,
	SVAR_NUM_PHASING_MODES,
	SVAR_NUM_RES_MODES,
	SVAR_NUM_FREQ_MODES,
	SVAR_NUM_NOTES,

	/* 15 */
	SVAR_KEY_START_NOTE,
	SVAR_NUM_SCALES,
	SVAR_NUM_RING_RANGES,
	SVAR_NUM_RING_MODES,
	SVAR_NUM_TIMERS,

	/* 20 */
	SVAR_TIMER_INTERVAL,
	SVAR_ARG,
	SVAR_TRUE,
	SVAR_FALSE,
	SVAR_EVENT_TYPE,

	/* 25 */
	SVAR_DIAL,
	SVAR_DIAL_VALUE,
	SVAR_BUTTON,
	SVAR_BUTTON_VALUE,
	SVAR_BUTTON_VALUE_NAME,

	/* 30 */
	SVAR_FREQ,
	SVAR_NOTE,
	SVAR_NOTE_NAME,
	SVAR_SCALE,
	SVAR_SCALE_NAME,

	/* 35 */
	SVAR_FUNCTION_KEY,
	SVAR_SECTION,
	SVAR_RESTART_NUM,
	SVAR_SPAN,
	SVAR_SPAN_MAX_INDEX,

	/* 40 */
	SVAR_FILTER,
	SVAR_EFFECTS_SEQ,
	SVAR_WIN_MAPPED,
	SVAR_WIN_WIDTH,
	SVAR_WIN_HEIGHT,

	/* 45 */
	SVAR_MODE,
	SVAR_SOUND_ENABLED,
	SVAR_SOUND_SYSTEM,
	SVAR_SAMPLING_ENABLED,
	SVAR_COM_EXEC_CNT,

	NUM_SYSTEM_VARS
};


/* Dictionary elements for variables. Doubly linked list so they can be
   deleted ad hoc */
typedef struct st_dict
{
	char *key;
	st_value val;
	struct st_dict *prev;
	struct st_dict *next;
} st_dict;

typedef struct st_var
{
	char *name;
	st_value *arr;
	int array_size;
	int sysvar_num;
	int dict_size;
	st_dict **dict_first;
	st_dict **dict_last;

	struct st_var *next;
} st_var;

st_var *first_var[NUM_UCHARS];
st_var *last_var[NUM_UCHARS];
st_var *system_var[NUM_SYSTEM_VARS];


typedef struct 
{
	int then_loc;
	int else_loc;
	int fi_loc;
} st_if;


/* WHILE-WEND, DO-UNTIL, FOR-NEXT */
typedef struct
{
	int start_loc;  /* WHILE, DO, FOR, LOOP */
	int end_loc;    /* WEND, UNTIL, NEXT, LEND */

	/* FOR-NEXT & LOOP-LEND blocks only */
	int to_loc;
	int step_loc;
	int looped;
	double from_val;
	double to_val;
	double step_val;
} st_loop;


typedef struct 
{
	char *str;
	int token_type;
	int token_num;
	int file_linenum;
	int basic_linenum;
	int checked;

	double val;
	int negative;

	/* Points to variable struct if TOK_VAR */
	st_var *var;

	/* Location of next Basic line (either seperated by '\n' or ':') which
	   has something to run */
	int line_start_loc;
	int line_end_loc;
	int next_line_loc;

	/* Position of next comma in line */
	int comma_loc;

	/* Mutually exclusive fields */
	union
	{
		st_if *if_block;
		st_loop *loop_block;

		/* Position of matching closing bracket */
		int close_bracket_loc;
	} u;

} st_token;

st_token *token_list;

/*** Stores labels and their locations. The reason that gotos don't just use
     jump_loc in st_token is the goto value can be an expression which could
     be a different label each time ***/
typedef struct st_label
{
	char *name;
	int loc;
	struct st_label *next;
} st_label;

st_label *first_label, *last_label;


/*** Commands. Normal commands are COM_, sub commands (ie commands that must
     be part of a clause) are COM_SUB_ ***/
enum en_com
{
	/* 0 */
	COM_SECTION,
	COM_VAR,
	COM_DIM,
	COM_LET,
	COM_IF,

	/* 5 */
	COM_SUB_THEN,
	COM_ELSE,  /* strictly speaking this is a SUB but denotes line ends */
	COM_FI,
	COM_FIALL,
	COM_FOR,

	/* 10 */
	COM_SUB_TO,
	COM_SUB_STEP,
	COM_NEXT,
	COM_WHILE,
	COM_WEND,

	/* 15 */
	COM_DO,
	COM_UNTIL,
	COM_LOOP,
	COM_LEND,
	COM_BREAK,

	/* 20 */
	COM_CONTINUE,
	COM_GOTO,
	COM_GOSUB,
	COM_RETURN,
	COM_DATA,

	/* 25 */
	COM_READ,
	COM_RESTORE,
	COM_AUTORESTORE,
	COM_PRINT,
	COM_MESG,

	/* 30 */
	COM_PRINTMESG,
	COM_SLEEP,
	COM_EXIT,
	COM_QUIT,
	COM_PAUSE,

	/* 35 */
	COM_RESTART,
	COM_YIELD,
	COM_SUB_LOCK,
	COM_SUB_UNLOCK,
	COM_CLEAR,

	/* 40 */
	COM_RESET,
	COM_REM,
	COM_SUB_PROG,

	NUM_COMMANDS
};
	
/* Command functions */
int comVar(int sect, int pc, st_token *token);
int comLet(int sect, int pc, st_token *token);
int comIf(int sect, int pc, st_token *token);
int comThen(int sect, int pc, st_token *token);
int comElse(int sect, int pc, st_token *token);
int comFor(int sect, int pc, st_token *token);
int comNext(int sect, int pc, st_token *token);
int comWhile(int sect, int pc, st_token *token);
int comWend(int sect, int pc, st_token *token);
int comDo(int sect, int pc, st_token *token);
int comUntil(int sect, int pc, st_token *token);
int comLoop(int sect, int pc, st_token *token);
int comLend(int sect, int pc, st_token *token);
int comBreak(int sect, int pc, st_token *token);
int comContinue(int sect, int pc, st_token *token);
int comGotoGosub(int sect, int pc, st_token *token);
int comReturn(int sect, int pc, st_token *token);
int comRead(int sect, int pc, st_token *token);
int comRestore(int sect, int pc, st_token *token);
int comPrint(int sect, int pc, st_token *token);
int comSleep(int sect, int pc, st_token *token);
int comExit(int sect, int pc, st_token *token);
int comQuit(int sect, int pc, st_token *token);
int comYield(int sect, int pc, st_token *token);
int comBlock(int sect, int pc, st_token *token);
int comUnblock(int sect, int pc, st_token *token);
int comClear(int sect, int pc, st_token *token);
int comPause(int sect, int pc, st_token *token);
int comRestart(int sect, int pc, st_token *token);
int comReset(int sect, int pc, st_token *token);
int comSubCommand(int sect, int pc, st_token *token);
int comDefault(int sect, int pc, st_token *token);

typedef struct 
{
	char *name;
	int (*func)(int sect, int pc, st_token *token);
} st_com;

/* Commands. Not all are runnable , eg else */
#ifdef MAINFILE
st_com command[NUM_COMMANDS] =
{
	/* 0 */
	{ "SECTION",     comDefault },
	{ "VAR",         comVar },
	{ "DIM",         comVar },
	{ "LET",         comLet },
	{ "IF",          comIf },

	/* 5 */
	{ "THEN",        comThen },
	{ "ELSE",        comElse },
	{ "FI",          comDefault },
	{ "FIALL",       comDefault },
	{ "FOR",         comFor },

	/* 10 */
	{ "TO",          comSubCommand },
	{ "STEP",        comSubCommand },
	{ "NEXT",        comNext },
	{ "WHILE",       comWhile },
	{ "WEND",        comWend },

	/* 15 */
	{ "DO",          comDo },
	{ "UNTIL",       comUntil },
	{ "LOOP",        comLoop },
	{ "LEND",        comLend },
	{ "BREAK",       comBreak },

	/* 20 */
	{ "CONTINUE",    comContinue },
	{ "GOTO",        comGotoGosub },
	{ "GOSUB",       comGotoGosub },
	{ "RETURN",      comReturn },
	{ "DATA",        comDefault },

	/* 25 */
	{ "READ",        comRead },
	{ "RESTORE",     comRestore },
	{ "AUTORESTORE", comRestore },
	{ "PRINT",       comPrint },
	{ "MESG",        comPrint },

	/* 30 */
	{ "PRINTMESG",   comPrint },
	{ "SLEEP",       comSleep },
	{ "EXIT",        comExit },
	{ "QUIT",        comQuit },
	{ "PAUSE",       comPause },

	/* 35 */
	{ "RESTART",     comRestart },
	{ "YIELD",       comYield },
	{ "BLOCK",       comBlock },
	{ "UNBLOCK",     comUnblock },
	{ "CLEAR",       comClear },

	/* 40 */
	{ "RESET",       comReset },
	{ "REM",         comDefault },
	{ "PROGRAM",     comSubCommand }
};
#else
extern st_com command[NUM_COMMANDS];
#endif


/*** FUNCTIONS ***/
enum en_func
{
	/* 0 */
	FUNC_GET,
	FUNC_SET,
	FUNC_SIN,
	FUNC_COS,
	FUNC_TAN,

	/* 5 */
	FUNC_ARCSIN,
	FUNC_ARCCOS,
	FUNC_ARCTAN,
	FUNC_SRQT,
	FUNC_ABS,

	/* 10 */
	FUNC_SGN,
	FUNC_RANDOM,
	FUNC_POW,
	FUNC_LOG,
	FUNC_LOG2,

	/* 15 */
	FUNC_LOG10,
	FUNC_ROUND,
	FUNC_FLOOR,
	FUNC_CEIL,
	FUNC_INT,

	/* 20 */
	FUNC_INSTR,
	FUNC_SUBSTR,
	FUNC_TOSTR,
	FUNC_INTSTR,
	FUNC_OCTSTR,

	/* 25 */
	FUNC_HEXSTR,
	FUNC_BINSTR,
	FUNC_TONUM,
	FUNC_ISNUM,
	FUNC_ISNUMTYPE,

	/* 30 */
	FUNC_UPPERSTR,
	FUNC_LOWERSTR,
	FUNC_STRLEN,
	FUNC_HASKEY,
	FUNC_DELKEY,

	/* 35 */
	FUNC_GETKEY,
	FUNC_KEYCNT,
	FUNC_FIELD,
	FUNC_FIELD1,
	FUNC_FIELDCNT,

	/* 40 */
	FUNC_FIELDCNT1,
	FUNC_ARRSIZE,
	FUNC_LOAD,
	FUNC_SAVE,
	FUNC_ASC,

	/* 45 */
	FUNC_CHR,
	FUNC_MAX,
	FUNC_MIN,
	FUNC_GETLONGSCALENAME,
	FUNC_GETSHORTSCALENAME,

	/* 50 */
	FUNC_GETSCALEBYNAME,
	FUNC_GETNOTENAME,
	FUNC_GETNOTEBYNAME,
	FUNC_GETNOTEFREQ,
	FUNC_GETNOTEBYFREQ,

	/* 55 */
	FUNC_GETSOUNDNAME,
	FUNC_GETSOUNDBYNAME,
	FUNC_GETCHORDNAME,
	FUNC_GETCHORDBYNAME,
	FUNC_GETARPSEQNAME,

	/* 60 */
	FUNC_GETARPSEQBYNAME,
	FUNC_GETRESMODENAME,
	FUNC_GETRESMODEBYNAME,
	FUNC_GETPHASINGMODENAME,
	FUNC_GETPHASINGMODEBYNAME,
	
	/* 65 */
	FUNC_GETRINGRANGENAME,
	FUNC_GETRINGRANGEBYNAME,
	FUNC_GETRINGMODENAME,
	FUNC_GETRINGMODEBYNAME,

	NUM_FUNCTIONS
};

int funcGet(int func, int arg_cnt, st_value *arg);
int funcSet(int func, int arg_cnt, st_value *arg);
int funcTrig(int func, int arg_cnt, st_value *arg);
int funcArcTrig(int func, int arg_cnt, st_value *arg);
int funcSqrt(int func, int arg_cnt, st_value *arg);
int funcAbs(int func, int arg_cnt, st_value *arg);
int funcSgn(int func, int arg_cnt, st_value *arg);
int funcRandom(int func, int arg_cnt, st_value *arg);
int funcPow(int func, int arg_cnt, st_value *arg);
int funcLog(int func, int arg_cnt, st_value *arg);
int funcRound(int func, int arg_cnt, st_value *arg);
int funcFloor(int func, int arg_cnt, st_value *arg);
int funcCeil(int func, int arg_cnt, st_value *arg);
int funcInt(int func, int arg_cnt, st_value *arg);
int funcInStr(int func, int arg_cnt, st_value *arg);
int funcSubStr(int func, int arg_cnt, st_value *arg);
int funcNumStr(int func, int arg_cnt, st_value *arg);
int funcBinStr(int func, int arg_cnt, st_value *arg);
int funcToNum(int func, int arg_cnt, st_value *arg);
int funcIsNumStr(int func, int arg_cnt, st_value *arg);
int funcIsNumType(int func, int arg_cnt, st_value *arg);
int funcUpperLowerStr(int func, int arg_cnt, st_value *arg);
int funcStrLen(int func, int arg_cnt, st_value *arg);
int funcHasKey(int func, int arg_cnt, st_value *arg);
int funcDelKey(int func, int arg_cnt, st_value *arg);
int funcGetKey(int func, int arg_cnt, st_value *arg);
int funcKeyCnt(int func, int arg_cnt, st_value *arg);
int funcField(int func, int arg_cnt, st_value *arg);
int funcFieldCnt(int func, int arg_cnt, st_value *arg);
int funcArrSize(int func, int arg_cnt, st_value *arg);
int funcLoadSave(int func, int arg_cnt, st_value *arg);
int funcAsc(int func, int arg_cnt, st_value *arg);
int funcChr(int func, int arg_cnt, st_value *arg);
int funcMaxMin(int func, int arg_cnt, st_value *arg);
int funcGetScaleName(int func, int arg_cnt, st_value *arg);
int funcGetScaleByName(int func, int arg_cnt, st_value *arg);
int funcGetNoteInfo(int func, int arg_cnt, st_value *arg);
int funcGetNoteByName(int func, int arg_cnt, st_value *arg);
int funcGetNoteByFreq(int func, int arg_cnt, st_value *arg);
int funcGetSoundName(int func, int arg_cnt, st_value *arg);
int funcGetSoundByName(int func, int arg_cnt, st_value *arg);
int funcGetChordName(int func, int arg_cnt, st_value *arg);
int funcGetChordByName(int func, int arg_cnt, st_value *arg);
int funcGetARPSeqName(int func, int arg_cnt, st_value *arg);
int funcGetARPSeqByName(int func, int arg_cnt, st_value *arg);
int funcGetResModeName(int func, int arg_cnt, st_value *arg);
int funcGetResModeByName(int func, int arg_cnt, st_value *arg);
int funcGetPhasingModeName(int func, int arg_cnt, st_value *arg);
int funcGetPhasingModeByName(int func, int arg_cnt, st_value *arg);
int funcGetRingRangeName(int func, int arg_cnt, st_value *arg);
int funcGetRingRangeByName(int func, int arg_cnt, st_value *arg);
int funcGetRingModeName(int func, int arg_cnt, st_value *arg);
int funcGetRingModeByName(int func, int arg_cnt, st_value *arg);

#define MAX_FUNC_PARAMS 10

enum en_func_param_type
{
	PARAM_VAR,
	PARAM_STR,
	PARAM_NUM,
	PARAM_EITHER
};

typedef struct 
{
	char *name;
	int num_params;
	int param_type[MAX_FUNC_PARAMS];
	int (*func)(int func, int arg_cnt, st_value *arg);
} st_func;


#ifdef MAINFILE
st_func func_info[NUM_FUNCTIONS] =
{
	/* 0 */
	{ "GET",      1, { PARAM_STR }, funcGet },
	{ "SET",      2, { PARAM_STR, PARAM_EITHER }, funcSet },
	{ "SIN",      1, { PARAM_NUM }, funcTrig },
	{ "COS",      1, { PARAM_NUM }, funcTrig },
	{ "TAN",      1, { PARAM_NUM }, funcTrig },

	/* 5 */
	{ "ARCSIN",   1, { PARAM_NUM }, funcArcTrig },
	{ "ARCCOS",   1, { PARAM_NUM }, funcArcTrig },
	{ "ARCTAN",   1, { PARAM_NUM }, funcArcTrig },
	{ "SQRT",     1, { PARAM_NUM }, funcSqrt },
	{ "ABS",      1, { PARAM_NUM }, funcAbs },

	/* 10 */
	{ "SGN",      1, { PARAM_NUM }, funcSgn },
	{ "RANDOM",   1, { PARAM_NUM }, funcRandom },
	{ "POW",      2, { PARAM_NUM, PARAM_NUM}, funcPow },
	{ "LOG",      1, { PARAM_NUM }, funcLog },
	{ "LOG2",     1, { PARAM_NUM }, funcLog },

	/* 15 */
	{ "LOG10",    1, { PARAM_NUM }, funcLog },
	{ "ROUND",    1, { PARAM_NUM }, funcRound },
	{ "FLOOR",    1, { PARAM_NUM }, funcFloor },
	{ "CEIL",     1, { PARAM_NUM }, funcCeil },
	{ "INT",      1, { PARAM_NUM }, funcInt },

	/* 20 */
	{ "INSTR",    3, { PARAM_STR, PARAM_STR, PARAM_NUM }, funcInStr },
	{ "SUBSTR",   3, { PARAM_STR, PARAM_NUM, PARAM_NUM }, funcSubStr },
	{ "TOSTR",    1, { PARAM_NUM }, funcNumStr },
	{ "INTSTR",   1, { PARAM_NUM }, funcNumStr },
	{ "OCTSTR",   1, { PARAM_NUM }, funcNumStr },

	/* 25 */
	{ "HEXSTR",   1, { PARAM_NUM }, funcNumStr },
	{ "BINSTR",   1, { PARAM_NUM }, funcBinStr },
	{ "TONUM",    1, { PARAM_STR }, funcToNum },
	{ "ISNUMSTR", 1, { PARAM_STR }, funcIsNumStr },
	{ "ISNUMTYPE",1, { PARAM_EITHER }, funcIsNumType },

	/* 30 */
	{ "UPPERSTR", 1, { PARAM_STR }, funcUpperLowerStr },
	{ "LOWERSTR", 1, { PARAM_STR }, funcUpperLowerStr },
	{ "STRLEN",   1, { PARAM_STR }, funcStrLen },
	{ "HASKEY",   2, { PARAM_VAR, PARAM_STR }, funcHasKey },
	{ "DELKEY",   2, { PARAM_VAR, PARAM_STR }, funcDelKey },

	/* 35 */
	{ "GETKEY",   2, { PARAM_VAR, PARAM_NUM }, funcGetKey },
	{ "KEYCNT",   1, { PARAM_VAR }, funcKeyCnt },
	{ "FIELD",    3, { PARAM_STR, PARAM_NUM, PARAM_STR }, funcField },
	{ "FIELD1",   3, { PARAM_STR, PARAM_NUM, PARAM_STR }, funcField },
	{ "FIELDCNT", 2, { PARAM_STR, PARAM_STR }, funcFieldCnt },

	/* 40 */
	{ "FIELDCNT1",2, { PARAM_STR, PARAM_STR }, funcFieldCnt },
	{ "ARRSIZE",  1, { PARAM_VAR }, funcArrSize },
	{ "LOAD",     1, { PARAM_STR }, funcLoadSave },
	{ "SAVE",     1, { PARAM_STR }, funcLoadSave },
	{ "ASC",      1, { PARAM_STR }, funcAsc },

	/* 45. For variadic functions the negative param count is the min
	       number of parameters required */
	{ "CHR",               1, { PARAM_NUM }, funcChr },
	{ "MAX",              -2, { PARAM_NUM }, funcMaxMin },
	{ "MIN",              -2, { PARAM_NUM }, funcMaxMin },
	{ "GETLONGSCALENAME",  1, { PARAM_NUM }, funcGetScaleName },
	{ "GETSHORTSCALENAME", 1, { PARAM_NUM }, funcGetScaleName },

	/* 50 */
	{ "GETSCALEBYNAME",1, { PARAM_STR }, funcGetScaleByName },
	{ "GETNOTENAME",   2, { PARAM_NUM, PARAM_NUM }, funcGetNoteInfo },
	{ "GETNOTEBYNAME", 2, { PARAM_STR, PARAM_NUM }, funcGetNoteByName },
	{ "GETNOTEFREQ",   2, { PARAM_NUM, PARAM_NUM }, funcGetNoteInfo },
	{ "GETNOTEBYFREQ", 2, { PARAM_NUM, PARAM_NUM }, funcGetNoteByFreq },

	/* 55 */
	{ "GETSOUNDNAME",  1, { PARAM_NUM }, funcGetSoundName },
	{ "GETSOUNDBYNAME",1, { PARAM_STR }, funcGetSoundByName },
	{ "GETCHORDNAME",  1, { PARAM_NUM }, funcGetChordName },
	{ "GETCHORDBYNAME",1, { PARAM_STR }, funcGetChordByName },
	{ "GETARPSEQNAME", 1, { PARAM_NUM }, funcGetARPSeqName },

	/* 60 */
	{ "GETARPSEQBYNAME",     1, { PARAM_STR }, funcGetARPSeqByName },
	{ "GETRESMODENAME",      1, { PARAM_NUM }, funcGetResModeName },
	{ "GETRESMODEBYNAME",    1, { PARAM_STR }, funcGetResModeByName },
	{ "GETPHASINGMODENAME",  1, { PARAM_NUM }, funcGetPhasingModeName },
	{ "GETPHASINGMODEBYNAME",1, { PARAM_STR }, funcGetPhasingModeByName },

	/* 65 */
	{ "GETRINGRANGENAME",  1, { PARAM_NUM }, funcGetRingRangeName },
	{ "GETRINGRANGEBYNAME",1, { PARAM_STR }, funcGetRingRangeByName },
	{ "GETRINGMODENAME",   1, { PARAM_NUM }, funcGetRingModeName },
	{ "GETRINGMODEBYNAME", 1, { PARAM_STR }, funcGetRingModeByName }
};
#else
extern st_func func_info[NUM_FUNCTIONS];
#endif

enum en_op
{
	/* 0 */
	OP_COMMA,
	OP_COLON,
	OP_L_RND_BRACKET,
	OP_R_RND_BRACKET,
	OP_L_SQR_BRACKET,

	/* 5 */
	OP_R_SQR_BRACKET,
	OP_NOT,
	OP_AND,
	OP_OR,
	OP_XOR,

	/* 10 */
	OP_EQUALS,
	OP_NOT_EQUALS,
	OP_GREATER_EQUALS,
	OP_LESS_EQUALS,
	OP_GREATER,

	/* 15 */
	OP_LESS,
	OP_ADD,
	OP_SUB,
	OP_MULT,
	OP_DIV,

	/* 20 */
	OP_BIT_AND,
	OP_BIT_OR,
	OP_BIT_XOR,
	OP_BIT_COMPL,
	OP_LEFT_SHIFT,

	/* 25 */
	OP_RIGHT_SHIFT,
	OP_MOD,
	OP_SEMI_COLON,

	NUM_OPS
};

	
struct st_op
{
	char *str;
	char prec;
};


#ifdef MAINFILE
struct st_op op_info[NUM_OPS] =
{
	/* 0 */
	{ ",",   0 },
	{ ":",   0 },
	{ "(",   0 },
	{ ")",   0 },
	{ "[",   0 },

	/* 5 */
	{ "]",   0 },
	{ "NOT", 0 },
	{ "AND", 1 },
	{ "OR",  1 },
	{ "XOR", 1 },

	/* 10 */
	{ "=",   2 },
	{ "<>",  2 },
	{ ">=",  2 },
	{ "<=",  2 },
	{ ">",   2 },

	/* 15 */
	{ "<",   2 },
	{ "+",   3 },
	{ "-",   3 },
	{ "*",   4 },
	{ "/",   4 },

	/* 20 */
	{ "&",   5 },
	{ "|",   5 },
	{ "^",   5 },
	{ "~",   0 },
	{ "<<",  5 },

	/* 25 */
	{ ">>",  5 },
	{ "%",   6 },
	{ ";",   0 }
};
#else
extern struct st_op op_info[NUM_OPS];
#endif


enum en_error
{
	/* 0 */
	OK,
	ERR_SYNTAX,
	ERR_INVALID_ARGUMENT,
	ERR_INVALID_VAR,
	ERR_INVALID_ARR_INDEX,

	/* 5 */
	ERR_INVALID_NEGATIVE,
	ERR_ARR_INDEX_OOB,
	ERR_INVALID_ARR_SIZE,
	ERR_ARGUMENT_MISMATCH,
	ERR_UNDEFINED_VAR,

	/* 10 */
	ERR_DUPLICATE_DECL,
	ERR_READ_ONLY,
	ERR_INVALID_LABEL,
	ERR_UNDEFINED_LABEL,
	ERR_NO_DATA,

	/* 15 */
	ERR_DATA_EXHAUSTED,
	ERR_MISSING_TO,
	ERR_MISSING_NEXT,
	ERR_MISSING_WEND,
	ERR_MISSING_UNTIL,

	/* 20 */
	ERR_MISSING_LEND,
	ERR_MISSING_THEN,
	ERR_MISSING_FI,
	ERR_MISSING_BRACKET,
	ERR_UNEXPECTED_THEN,

	/* 25 */
	ERR_UNEXPECTED_ELSE,
	ERR_UNEXPECTED_FI,
	ERR_UNEXPECTED_NEXT,
	ERR_UNEXPECTED_WEND,
	ERR_UNEXPECTED_UNTIL,

	/* 30 */
	ERR_UNEXPECTED_LEND,
	ERR_UNEXPECTED_BREAK_CONT,
	ERR_UNEXPECTED_RETURN,
	ERR_INVALID_SECTION,
	ERR_DUPLICATE_SECTION,

	/* 35 */
	ERR_SECTION_NOT_FIRST,
	ERR_STACK_OVERFLOW,
	ERR_DIVIDE_BY_ZERO,
	ERR_EMPTY_EXPRESSION,
	ERR_MISSING_PARAMS,

	/* 40 */
	ERR_TOO_MANY_PARAMS,
	ERR_MAX_RECURSION,
	ERR_INVALID_GET_FIELD,
	ERR_INVALID_SET_FIELD,
	ERR_INVALID_SET_VALUE,

	/* 45 */
	ERR_KEY_NOT_FOUND,
	ERR_FILENAME_TOO_LONG,
	ERR_CANT_INIT_ARRAY,
	ERR_FUNCTION_UNAVAILABLE,
	ERR_SAMPLING_UNAVAILABLE,

	/* 50 */
	ERR_INVALID_WAVEFORM,
	ERR_INVALID_CHORD,
	ERR_INVALID_NOTE,
	ERR_INVALID_NOTE_FOR_SCALE,
	ERR_INVALID_SCALE,

	/* 55 */
	ERR_RESTART_IN_INIT,
	ERR_SLEEP_IN_TIMER,
	ERR_PERIOD_MISSING,
	ERR_INVALID_PERIOD,
	ERR_PERIOD_TOO_SMALL,

	/* 60 */
	ERR_OUT_OF_RANGE,
	ERR_MISSING_END_QUOTES,
	ERR_INVALID_EFFECTS_SEQ,
	ERR_INVALID_EFFECT_NAME,
	ERR_DUPLICATE_EFFECT_NAME,

	NUM_ERRORS
};


#ifdef MAINFILE
char *error_mesg[NUM_ERRORS] =
{
	/* 0 */
	"OK",
	"Syntax error",
	"Invalid argument",
	"Invalid variable name",
	"Invalid array index",

	/* 5 */
	"Invalid negative",
	"Array index out of bounds",
	"Invalid array size",
	"Argument mismatch",
	"Undefined variable or function",

	/* 10 */
	"Duplicate variable declaration",
	"Cannot set read only variable",
	"Invalid label name",
	"Undefined label",
	"No DATA following RESTORE label",

	/* 15 */
	"DATA exhausted",
	"Missing TO",
	"Missing NEXT",
	"Missing WEND",
	"Missing UNTIL",

	/* 20 */
	"Missing LEND",
	"Missing THEN",
	"Missing FI",
	"Missing bracket",
	"Unexpected THEN",

	/* 25 */
	"Unexpected ELSE",
	"Unexpected FI",
	"Unexpected NEXT",
	"Unexpected WEND",
	"Unexpected UNTIL",

	/* 30 */
	"Unexpected LEND",
	"Unexpected BREAK or CONTINUE",
	"Unexpected RETURN",
	"Invalid section",
	"Duplicate section",

	/* 35 */
	"Section header must come first",
	"Stack overflow",
	"Divide by zero",
	"Empty expression",
	"Missing function parameter(s)",

	/* 40 */
	"Too many function parameters",
	"GOSUB recursion limit reached",
	"Invalid GET field",
	"Invalid SET field",
	"Invalid SET value",

	/* 45 */
	"Dictionary key not found",
	"Filename too long",
	"Cannot initialise arrays",
	"Function unavailable",
	"Sampling unavailable",

	/* 50 */
	"Invalid waveform",
	"Invalid chord",
	"Invalid note",
	"Invalid note for current scale",
	"Invalid scale",

	/* 55 */
	"RESTART not allowed in INIT section",
	"SLEEP not allowed in TIMER section",
	"Time period for TIMER section missing",
	"Invalid time period for TIMER section",
	"Time period for TIMER section less than minimum resolution",

	/* 60 */
	"Value out of range",
	"Missing end quotes",
	"Invalid effects sequence",
	"Invalid effect name",
	"Duplicate effect name"
};
#else
extern char *error_mesg[NUM_ERRORS];
#endif

enum st_num_type
{
	NOT_NUM,
	NUM_OCT,
	NUM_DEC,
	NUM_HEX
};


int num_tokens;
int num_tokens_alloc;
int call_basic;
int yield_after;  
int com_exec_cnt;
int blocking_section;
int runnable_sections;
int restart_cnt;
int auto_messages;
int io_messages;
int pause_program;
int deferred_init;
int do_timer_catchup;


/************************* Forward declarations **************************/

/* main.c */
void paramsReset(int reset_disk, int reset_evr);
void windowResized();

/* sound_init.c */
void startSoundDaemon();
void resetSharedMemory();
void createNotes();

/* sound_if.c */
void pauseSoundProcess();
void restartSoundProcess();
void playOn(int call_basic);
void playOff(int call_basic);
void setNote(int note);
void setNoteFreqArray();
void setNoteByKey(char key);
void setFreqByX(short x, int mouse_button);
void setNoteAndFreq(int note);
void setFreq(short freq);
void setFilter(int val, int call_basic);
void setFilterValueByY(int y);
void setFilterSensitivity(u_char sens);
void setMainOsc(u_char snd);
void incField(u_char *field, int by);
void setField(u_char *field, int val, int max);
int  setScale(char *name, int scale);
void cycleScale(int mouse_button);
void cycleChord(int mouse_button);
int  setOrCycleChord(int mouse_button, char *strval, int val);
int  setOrCycleARP(int mouse_button, char *strval, int val);
int  setOrCyclePhasingMode(int mouse_button, char *strval, int val);
int  setOrCycleFreqMode(int mouse_button, char *strval, int val);
int  setOrCycleRingModRange(int mouse_button, char *strval, int val);
int  setOrCycleRingModMode(int mouse_button, char *strval, int val);
void setRingModFreq(int freq);
void cycleMainOsc();
int  setOrCycleSubOsc(int osc, int mouse_button, char *strval, int val);
int  setOrCycleResMode(int mouse_button, char *strval, int val);
void setOrCycleEffectsSeq(int mouse_button, int val);
int  setEffectsSeqByString(char *seq);
void swapEffects();
void resetEffectsSeq(int press);
void setFieldToDialAngle(u_char *field, enum en_button but);
void setByRange();
void resync();
int  findNote(int freq);
void normalisePatch(int do_window);
void randomiseSettings();

double getFMOffset();
double getPhaserLOM();
int    getAnalyserRange();
int    getMaxFrequency();
double getResonanceDamping();
double getGainCompressionExponent();
double getEchoStretch();
int    getRingModFreq(int dial_freq);

/* sound_run.c */
void soundLoop();
void resetSoundBuffer();
#if SOUND==OSX
void osxAudioCallback(void *ptr, AudioQueueRef q, AudioQueueBufferRef buf);
#endif

/* sound_gen.c */
void addSine(int ch, double vol, double freq, int cutoff_ang1, int reset);
void addSineFM(
	int ch,
	double vol,
	double freq,
	double fm_vol,
	double fm_freq, double fm_vol_exp, int cutoff_ang1, int reset);
void addTriangle(int ch, double vol, double freq, int reset);
void addSquare(int ch, double vol, double freq, double width, int reset);
void addSawtooth(
	int ch, double vol, double freq, double flatten_ratio, int reset);
void addWav(int ch, int snd, double vol, double freq, int reset);
void addNoise(double vol, double freq, int reset);
void addSample(
	double vol,
	u_short win_width, double freq, int sample_mod_cnt, int reset);
void filter(
	u_char highpass,
	double level, short *sbuff, int sbuff_size, short *prev_end);
void resonate(
	double zero_force_mult,
	double speed_mult, double damping_mult, int reset);
void distort(u_char level);
void flanger(int type_mult, int offset, int dirty, int smooth);
void phaser(
	int type_mult,
	int freq_div, int high_offset, double low_off_mult, int smooth);
void fmFlanger(int layers, int pitch_change);
short vibrato(short freq, double vib_mult, double ang_inc);
void alias(u_char al, short peak_volume);
void reflect(u_char rl, u_char rs, short peak_volume);
void ringModulate(int snd, int freq, int level);
int gainCompression(int val);

/* sound_misc.c */
void  setKeyStartNote(char val);
char *getNoteName(int note, int scale);
int   getNoteFromName(char *name, int scale);
int   getScaleFromName(char *name);
int   getSoundNumberFromName(char *str);

/* dft.c */
void calcDFT();

/* mouse.c */
void mousePressed(int mouse_button, short x, short y);
void mouseReleased(int mouse_button);
void mouseData(int mouse_button, short x, short y, char motion);

/* keyboard.c */
int  processConsoleData(int *is_note_key);
void keyPressed(XEvent *event);
void processKeyPressed(KeySym ksym, char key);
int  processKeyByAsciiValue(int key, int *is_note_key);
void keyReleased(XEvent *event);
void processKeyReleased(char key);
void resetKeyboard();
void resetConsole();

/* buttons.c */
void setupButtons();
void resetButtons(int init, int reset_evr);
void updateClickButtons();
void updateMainOscButtons();
int  buttonPressed(int mouse_button, char motion, double wx, double wy);
void setHoverString(short x, short y);
double getButtonValue(int but);
int  setButtonField(int but, char *strval, double val);
void setButtonByParam(enum en_button but, u_char param);
int  getButton(char *name);

/* disk.c */
void loadProgramMode();
void loadPatchMode();
int  loadFile(int patch);
int  saveFile(int patch);
void listFiles(char *suffix);
void savePatchMode();
void saveProgramMode();
void clearDiskStruct();
void addFilenameChar(KeySym ksym, char key);
void addFileSuffix();

/* draw.c */
void draw();
void setLineWidth(int col, int w);

/* mesg.c */
void dualMessage(char *fmt, ...);
void message(char *fmt, ...);
void updateMessages();

/* recorder.c */
void evrInit();
void evrClear(int press);
void evrStop();
void evrPause();
void evrRecord(int on);
void evrPlay();
void evrLoop(u_char val);
void evrEventsInSave(u_char val);
void evrPatchReset(u_char val);
void evrSetPatchToMain();
void evrAddMotionEvent(int mouse_button, short x, short y);
void evrAddMousePressEvent(int mouse_button, short x, short y);
void evrAddMouseReleaseEvent(int mouse_button);
void evrAddKeyPressEvent(KeySym ksym, char key);
void evrAddKeyReleaseEvent(KeySym ksym, char key);
struct st_recorder_event *evrCreateNewEvent();
void evrPlayEvents(u_int now);

/* misc.c */
u_int  getUsecTime();
double getUsecTimeAsDouble();
void   incAngle(double *ang, double add);
double getTextPixelLen(int len, double scale);
void   setTitleBar(char *str);
void   setFreezeWaveform(int frz);
void   setFillWaveform(int fill);
void   reset(int reset_evr);
void   quit(int reason);

/* bas_init.c */
void loadProgram(int startup);
void deleteProgram(int loading);
void initBasic();
void restartProgram();

/* bas_tokeniser.c */
int tokenise(char *filename);

/* bas_run.c */
void runEventSection(int sect, int but, double val, char *strval);
void runNonEventSections();
void runSection(int sect);
void stopSection(int sect, int err);
void pauseProgram(int on);
void stopProgram();

/* bas_variables.c */
void initVariableArrays();
void createSystemVariables();
st_var *createVariable(int *pc, int *is_array);
int getVariableValue(int *pc, st_value *value);
st_var *getVariableAndIndex(int *pc, st_value *index);
st_var *getVariable(char *name);
int setVariable(
	st_var *var,
	st_value *index, int force, double num, char *str, st_token *token);
int setVariableByValue(
	st_var *var,
	st_value *index, int force, st_value *result, st_token *token);
void clearVariable(st_var *var);
void deleteVariables();
st_dict *getDictionaryElementByKey(st_var *var, char *key);
st_dict *getDictionaryElementByNumber(st_var *var, int pos);
int deleteDictionaryElement(st_var *var, char *key);

/* bas_expressions.c */
int evalExpression(int start, int end, st_value *result);

/* bas_functions.c */
void setupGSFields();
int callFunction(int *pc, int end, st_value *result);

/* bas_values.c */
void initValue(st_value *result);
void setValue(st_value *result, double val, char *strval);
void setValueByValue(st_value *value1, st_value *value2);
void setDirectStringValue(st_value *value, char *strval);
void appendStringValue(st_value *value1, st_value *value2);
char *multStringValue(st_value *value, int cnt);
void clearValue(st_value *value);
int trueValue(st_value *value);

/* bas_labels.c */
void addLabel(char *name, int loc);
int getLabelLocation(char *name);

/* bas_misc.c */
int isNumber(char *str, int allow_neg);
void basicError(int err, st_token *token);
