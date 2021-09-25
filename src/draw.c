/*** Draws the window and all the widgets inside ***/

#include "globals.h"

#define CIRCLE              23040
#define DFT_MAX_VALUE       10000000

static void drawCreditsPage();
static void drawAsciiChars();
static void drawMessages();
static void drawKeys();
static void drawInfoLine();
static void drawSpectrumAnalyser();
static void drawFilterBar();
static void drawWaveform();
static void drawEventCounts();
static void drawTail();
static void drawButtons();
static void drawBasicSectionStatus();
static void drawSaveLoad();
static void drawIntroBanner();
static void drawHoverString();

static void drawText(
	char *text, int col, double x_scale, double y_scale, double x, double y);
static void drawChar(
	int c, int col, double x_scale, double y_scale, double x, double y);

static void drawLine(int col, double xf, double yf, double xt, double yt);
static void drawRectangle(int col, double w, double h, double x, double y);
static void fillRectangle(int col, double w, double h, double x, double y);
static void fillPolygon(
	int col, 
	int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
static void drawCircle(int col, double w, double h, double x, double y);

/****************************************************************************/

/*** Main draw function called from main.c ***/
void draw()
{
	if (show_credits)
	{
		if (show_credits == 1)
			drawCreditsPage();
		else
			drawAsciiChars();
		return;
	}

	drawMessages();
	setLineWidth(PURPLE,3);
	if (shm->freq_mode == FREQ_NOTES) drawKeys();

	/* Filter cut off line & bar */
	drawLine(PURPLE,0,FILTER_Y,WIN_WIDTH,FILTER_Y);
	setLineWidth(PURPLE,1);
	drawFilterBar();

	/* Zero amplitude line */
	drawLine(RED,0,WIN_HEIGHT2,WIN_WIDTH,WIN_HEIGHT2);

	drawInfoLine();

	if (params.show_analyser) drawSpectrumAnalyser();
	drawWaveform();
	drawEventCounts();
	drawButtons();

	/* Save/load filename */
	if (disk.op != DISK_NO_OP) drawSaveLoad();

	if (intro_cnt < 70 * win_refresh) drawIntroBanner();
	if (hover_str) drawHoverString();
	if (show_section_status) drawBasicSectionStatus();

	if (draw_tail) drawTail();
}




/*** Show credits etc ***/
void drawCreditsPage()
{
	static int col = 0;

	drawText(
		"*** Analogue Modelling Painted Sound (AMPS) synth ***",
		col,0.9,2,45,10);

	drawText(COPYRIGHT1,NUM_COLOURS - col,1,2,115,50);

	col = (col + win_refresh) % NUM_COLOURS;

	drawText("Version:",GREEN,1,2,300,90);
	drawText(VERSION,MAUVE,1,2,430,90);

	drawText(
		"This started out as fairly simple sound doodle but",
		WHITE,1,2,10,150);
	drawText("I found it more interesting than I expected so I",
		WHITE,1,2,10,180);
	drawText("continued to develop it. And here is the result!",
		WHITE,1,2,10,210);
	drawText("I hope you like it and if you've found any bugs or",
		WHITE,1,2,10,270);
	drawText("have any ideas please email: neil@ogham.demon.co.uk",
		WHITE,1,2,10,300);
	drawText(
		"Press any key to return to main screen",
		BLUE,1,2,120,WIN_HEIGHT - 30);
}




/*** Dump the font ascii characters to the screen ***/
void drawAsciiChars()
{
	int c;
	int x;
	int y;

	drawText("Font ASCII chars:",TURQUOISE,1,2,10,10);

	x = 10;
	y = 40;
	for(c=32;c < 256;++c)
	{
		drawChar(c,WHITE,2,2,x,y);
		x += CHAR_SIZE * 2 + CHAR_GAP;
		if (x > WIN_WIDTH - CHAR_SIZE * 2)
		{
			x = 0;
			y += CHAR_SIZE * 2 + CHAR_GAP;
		}
	}
}




/*** Draw whatever is in the message array ***/
void drawMessages()
{
	double x;
	double now;
	int i;

	now = getUsecTimeAsDouble();

	for(i=0;i < MAX_MESGS;++i)
	{
		if (mesg[i].text && mesg[i].mesg_time <= now) 
		{
			/* Keep text centred so update x based on scale */
			x = (WIN_WIDTH / 2) - 
			    getTextPixelLen(mesg[i].len,mesg[i].x_scale) / 2;

			drawText(
				mesg[i].text,
				mesg[i].col,
				mesg[i].x_scale,mesg[i].y_scale,x,mesg[i].y);
		}
	}
}




/*** Draw the keys of the virtual keyboard ***/
void drawKeys()
{
	static int scale_start_pos[NUM_SCALES] =
	{
		0,0,1,0,
		1,1,1,
		2,
		3,3,
		4,4,
		5,6
	};
	double key_x;
	double x_scale;
	int note;
	int len;
	int i;
	char *note_name;

	x_scale = 1;
	if (key_cnt > 40) x_scale -= (double)(key_cnt - 40) / 40;

	for(i=0,key_x=0;i <= key_cnt;++i)
	{
		note = i % OCTAVE;

		/* Mark the scale start by a longer dividing line */
		len = (note == scale_start_pos[shm->note_scale] ? 70 : 50);

		/* Fill in note currently being played */
		if (i == shm->note)
		{
			fillRectangle(
				BLUE,key_spacing,len,key_x,FILTER_Y - len);
		}

		/* Draw vertical key linecnt. Mark out notes that can be
		   played on the keyboard */
		if (i >= params.key_start_note &&
		    i <= params.key_start_note + NUM_KEYB_NOTES)
		{
			if (i == params.key_start_note ||
			    i == params.key_start_note + NUM_KEYB_NOTES)
				setLineWidth(PURPLE,6);

			drawLine(
				PURPLE,
				key_x,FILTER_Y,key_x,FILTER_Y - len);

			setLineWidth(PURPLE,3);
		}
		else
		{
			drawLine(
				WHITE,
				key_x,FILTER_Y,key_x,FILTER_Y - len);
		}

		/* Draw key letters */
		note_name = getNoteName(note,shm->note_scale);
		drawChar(
			note_name[1],
			WHITE,x_scale,1,key_x + key_char_add,FILTER_Y - 15);

		/* Draw the sharp above the letter */
		if (note_name[2] == '#')
		{
			drawChar(
				'#',WHITE,x_scale,1,
				key_x + key_char_add,FILTER_Y - 30);
		}
		key_x += key_spacing;
	}
}




/*** Draw the current sub oscillator sound types, phasing mode, chord, the 
     sequence the effects are operating in ***/
void drawInfoLine()
{
	static char *sub_sound[NUM_SND_TYPES] =
	{
		"",
		"FM",
		"SIN",
		"SQR",
		"TRI",
		"SAW",
		"AAH",
		"OOH",
		"NOI",
		"", /* Subs can't do sampling */
	};
	static char *phasing_mode[NUM_PHASING_MODES] =
	{
		"",
		"AFL",
		"SFL",
		"ADFL",
		"SDFL",
		"APH",
		"SPH",
		"FMFL"
	};
	double tlen;
	int i;
	int x;
	int p1;
	int p2;

	/* Show frequency or sample speed */
	if (shm->sound == SND_SAMPLE)
		drawText(sample_speed_text,freq_col,2,2,BUTTON_GAP,BUTTON_HEIGHT);
	else
	{
		drawText(freq_text,freq_col,2,2,BUTTON_GAP,BUTTON_HEIGHT);
		tlen = getTextPixelLen(freq_text_len,2);
		drawText("Hz",freq_col,1,1.2,BUTTON_GAP+CHAR_GAP+tlen,BUTTON_HEIGHT);
	}

	/* Show scale */
	if (shm->freq_mode != FREQ_CONT)
		drawText(scale_name[shm->note_scale],RED,1,2,160,BUTTON_HEIGHT);

	/* Show sub oscillator sounds */
	drawText(
		sub_sound[shm->sub1_sound],
		PURPLE,1,2,WIN_WIDTH-430,BUTTON_HEIGHT);
	drawText(
		sub_sound[shm->sub2_sound],
		MAUVE,1,2,WIN_WIDTH-375,BUTTON_HEIGHT);
	drawText(
		phasing_mode[shm->phasing_mode],
		BLUE,1,2,WIN_WIDTH-320,BUTTON_HEIGHT);

	/* Show chord */
	if (shm->chord != CHORD_OFF)
	{
		drawText(
			chord_name[shm->chord],
			YELLOW,1,2,WIN_WIDTH-250,BUTTON_HEIGHT);
	}

	/* Show effects sequence and which pair can currently be swapped ***/
	x = WIN_WIDTH - 160;
	p1 = effects_pos[params.effects_swap].pos1;
	p2 = effects_pos[params.effects_swap].pos2;

	for(i=0;i < NUM_SWAP_EFFECTS;++i)
	{
		drawText(
			effects_short_name[shm->effects_seq[i]],
			i == p1 || i == p2 ? MAUVE : TURQUOISE,
			0.5,2,x,BUTTON_HEIGHT);
		x += 12;
		if (i < NUM_SWAP_EFFECTS - 1)
		{
			drawText("->",GREEN,0.4,1.2,x,BUTTON_HEIGHT+5);
			x += 12;
		}
	}
}




/*** Draw the DFT histogram ***/
void drawSpectrumAnalyser()
{
	double w;
	double x;
	double h;
	int i;
	int col;

	w = (double)WIN_WIDTH / params.analyser_range;
	x = 0;
	col = BLUE;

	for(i=0;i < params.analyser_range;++i)
	{
		h = ((double)dft_freq_level[i] / DFT_MAX_VALUE) * WIN_HEIGHT2;
		if (h)
		{
			if (h > WIN_HEIGHT2) h = WIN_HEIGHT2;
			fillRectangle(col,w,h,x,BOT_Y1-h);
		}
		x += w;
		col = (col == BLUE ? PURPLE : BLUE);
	}
}




/*** Draw the bar on the right that shows the level of filtering being sent to
     the sound engine. Shows BASIC changes but not LFO because the latter is
     done internally in the engine at a lower level ***/
void drawFilterBar()
{
	double bar_y;

	setLineWidth(ORANGE,3);
	bar_y = FILTER_Y + shm->filter_val;
	drawLine(ORANGE,WIN_WIDTH-10,bar_y,WIN_WIDTH,bar_y);
	setLineWidth(ORANGE,1);
}




/*** Draw an oscilloscope style waveform ***/
void drawWaveform()
{
	short snd;
	short *buffptr;
	int prev_sx;
	int prev_sy;
	int prev_prev_sy;
	double sx;
	double sy;
	int i;

	prev_sx = 0;
	prev_sy = 0;
	prev_prev_sy = 0;
	buffptr = freeze_waveform ? freeze_sndbuff : sndbuff;

	for(i=0,sx=0;i < SNDBUFF_SIZE;++i,sx += SND_X_ADD)
	{
		snd = buffptr[i];

		sy = WIN_HEIGHT2 - snd * SND_Y_MULT;
		if (i)
		{
			/* Don't overdraw areas of linecnt we've already drawn
			   as this just slows X down */
			if ((int)sx != prev_sx ||
			    ((int)sy < prev_sy && (int)sy < prev_prev_sy) ||
			    ((int)sy > prev_sy && (int)sy > prev_prev_sy))
			{
				if (params.fill_waveform)
				{
					fillPolygon(
						TURQUOISE,
						prev_sx,prev_sy,
						sx,sy,
						sx,WIN_HEIGHT2,	
						prev_sx,WIN_HEIGHT2);
				}
				else drawLine(TURQUOISE,prev_sx,prev_sy,sx,sy);
			}
		}
		prev_prev_sy = prev_sy;
		prev_sy = (int)sy;
		prev_sx = (int)sx;
	}
}




/* Show event playing and number of events stored. This has to go under the 
   filter line as there's no more room above. Draw after waveform and analyser
   so we can see it */
void drawEventCounts()
{
	static char str[100];

	sprintf(str,"%u/%u",evr_next_play_event,evr_events_cnt);
	setLineWidth(MAUVE,2);
	drawText(str,MAUVE,1.2,1.2,BUTTON_GAP,FILTER_Y + 5);
	setLineWidth(MAUVE,1);
}




/*** Draw the tail that follows the mouse pointer ***/
void drawTail()
{
	int i;
	int pos;

	pos = tail_next;
	for(i=0;i < TAIL_LEN;++i)
	{
		if (tail[pos].colour < BLACK)
		{
			XFillArc(
				display,drw,gc[tail[pos].colour],
				tail[pos].x,tail[pos].y,
				DIAM * g_x_scale,DIAM * g_y_scale,0,CIRCLE);

			tail[pos].colour += win_refresh;
		}
		pos = (pos + 1) % TAIL_LEN;
	}
}




/*** Draw all the buttons & dials ***/
void drawButtons()
{
	double angle;
	int i;
	int x;
	int y;
	int w;
	int h;

	for(i=0;i < NUM_BUTTONS;++i)
	{
		x = button[i].x + BUTTON_GAP;
		y = button[i].y + BUTTON_GAP;
		w = button[i].width - BUTTON_GAP * 2;
		h = BUTTON_HEIGHT - BUTTON_GAP * 2;

		if (button[i].type != BTYPE_DIAL)
		{
			if (i == BUT_QUIT ||
			    i == BUT_RESET || 
			    i == BUT_SAVE_PATCH || 
			    i == BUT_SAVE_PROG || 
			    i == BUT_LOAD_PATCH ||
			    i == BUT_LOAD_PROG)
			{
				setLineWidth(button[i].colour,3);
			}
			drawRectangle(button[i].colour,w,h,x,y);

			if (i == BUT_QUIT ||
			    i == BUT_RESET || 
			    i == BUT_SAVE_PATCH || 
			    i == BUT_SAVE_PROG || 
			    i == BUT_LOAD_PATCH ||
			    i == BUT_LOAD_PROG)
			{
				setLineWidth(button[i].colour,1);
			}

			if (button[i].pressed &&
			    button[i].pressed <= BUTTON_PRESS)
			{
				fillRectangle(button[i].colour,w,h,x,y);
			}

			/* Draw underline */
			if (button[i].underline_text)
			{
				drawText(
					button[i].underline_text,
					WHITE,
					button[i].text_x_scale,
					button[i].text_y_scale,
					button[i].text_x,
					button[i].underline_text_y);
			}
		}

		/* Draw button text */
		drawText(
			button_name[i],WHITE,
			button[i].text_x_scale,
			button[i].text_y_scale,
			button[i].text_x,button[i].text_y);

		if (button[i].type != BTYPE_DIAL) continue;

		/* Draw dial circle */
		drawCircle(button[i].colour,w,h,x,y);

		/* Draw position line */
		angle = button[i].angle + 180;
		x = button[i].mid_x + SIN(angle) * 0.5 * w;
		y = button[i].mid_y - COS(angle) * 0.5 * h;
		setLineWidth(DIAL_COL,4);
		drawLine(DIAL_COL,button[i].mid_x,button[i].mid_y,x,y);
		setLineWidth(DIAL_COL,1);
	}
}




/*** Show which sections are currently runnable, sleeping or stopped ***/
void drawBasicSectionStatus()
{
	st_section *sec;
	char str[50];
	char *status;
	int tokcnt;
	int linecnt;
	int col;
	int i;
	int x;
	int y;

	drawText("*** BASIC Section Status ***",
		LIGHT_BLUE,0.6,2,270,WIN_HEIGHT2 - 55);

	/* Number of tokens and number of linecnt */
	if (num_tokens)
	{
		tokcnt = num_tokens - NUM_PAD_TOKENS;
		linecnt = token_list[tokcnt-1].file_linenum;
	}
	else tokcnt = linecnt = 0;
	
	sprintf(str,"Tokens: %d, Lines: %d",tokcnt,linecnt);
	drawText(str,ORANGE,0.6,2,305,WIN_HEIGHT2 - 25);

	x = 10;
	y = WIN_HEIGHT2 + 10;

	for(i=0;i < NUM_SECTIONS;++i,y+=25)
	{
		if (i > 0 && !(i % 5))
		{
			y = WIN_HEIGHT2 + 10;
			x += 160;
		}
		sec = &section[i];
		
		sprintf(str,"%-8s:",sec->name);
		drawText(str,WHITE,0.45,1.5,x,y);

		if (sec->runnable)
		{
			if (blocking_section != SECTION_INIT && 
			    i != blocking_section)
			{
				status = "BLOCKED";
				col = PURPLE;
			}
			else if (sec->sleep_until)
			{
				status = "SLEEP";
				col = ORANGE;
			}
			else
			{
				status = "RUN";
				col = GREEN;
			}
			sprintf(str,"%4d",token_list[sec->pc].file_linenum);
			drawText(str,LIGHT_BLUE,0.45,1,x+105,y);
		}
		else
		{
			status = "STOP";
			col = RED;
		}
		drawText(status,col,0.45,1.5,x+65,y);
	}
}




/*** Draw the save/load patch filename text ***/
void drawSaveLoad()
{
	static int cursor = 0;
	static char text[PATH_MAX+1]; /* static saves on stack allocs */
	int draw_len;
	int len;

	switch(disk.op)
	{
	case DISK_LOAD_PROG:
		strcpy(text,"LOAD PROGRAM: ");
		draw_len = 38;
		break;

	case DISK_LOAD_PATCH:
		strcpy(text,"LOAD PATCH & EVENTS: ");
		draw_len = 31;
		break;

	case DISK_SAVE_PROG:
		strcpy(text,"SAVE PATCH AS PROGRAM: ");
		draw_len = 29;
		break;

	case DISK_SAVE_PATCH:
		if (evr_flags.events_in_save && evr_events_cnt)
		{
			strcpy(text,"SAVE PATCH & EVENTS: ");
			draw_len = 31;
		}
		else
		{
			strcpy(text,"SAVE PATCH: ");
			draw_len = 40;
		}
		break;

	default:
		assert(0);
	}

	if ((len = strlen(disk.filename)) > draw_len)
		strcat(text,disk.filename + (len - draw_len));
	else
		strcat(text,disk.filename);

	if (cursor < 10) strcat(text,"_");
	setLineWidth(WHITE,2);
	drawText(text,WHITE,1,2,BUTTON_GAP,BOT_Y1 - 30);
	setLineWidth(WHITE,1);

	cursor = (cursor + win_refresh) % 20;
}




/*** Name and copyright scroller ***/
void drawIntroBanner()
{
	static int intro_x = 55;
	static int intro_col = GREEN;
	static double intro_x_sub = 1;

	setLineWidth(intro_col,3);
	drawText(COPYRIGHT2,intro_col,0.9,4,intro_x,200);
	setLineWidth(intro_col,1);

	intro_col = (intro_col + 2 * win_refresh) % YELLOW;
	if ((intro_cnt += win_refresh) > 50) 
	{
		intro_x -= intro_x_sub * win_refresh;
		if (intro_x_sub < 50) intro_x_sub *= 1.5;
	}
}




/*** Show the value of a button when hovered over ***/
void drawHoverString()
{
	setLineWidth(TURQUOISE,3);
	drawText(hover_str,TURQUOISE,hover_x_scale,2,hover_x+5,hover_y-20);
	setLineWidth(TURQUOISE,1);
}



/***************************** TEXT FUNCTIONS ******************************/

void drawText(
	char *text, int col, double x_scale, double y_scale, double x, double y)
{
	int i;
	int len;

	len = strlen(text);

	for(i=0;i < len;++i)
	{
		drawChar((int)text[i],col,x_scale,y_scale,x,y);
		x += ((double)CHAR_SIZE * x_scale + (double)CHAR_GAP * x_scale);
	}
}




void drawChar(int c, int col, double x_scale, double y_scale, double x, double y)
{
	double xf;
	double yf;
	double xt;
	double yt;
	int i;

	if (!ascii_table[c]) c = '?';

	for(i=0;i < ascii_table[c]->cnt - 1;i += 2)
	{
		xf = x + ascii_table[c]->data[i].x * x_scale;
		yf = y + ascii_table[c]->data[i].y * y_scale;
		xt = x + ascii_table[c]->data[i+1].x * x_scale;
		yt = y + ascii_table[c]->data[i+1].y * y_scale;

		drawLine(col,xf,yf,xt,yt);
	}
}



/******************************* LOW LEVEL ***********************************/

void drawLine(int col, double xf, double yf, double xt, double yt)
{
	XDrawLine(
		display,drw,gc[col],
		(int)round(xf * g_x_scale),
		(int)round(yf * g_y_scale),
		(int)round(xt * g_x_scale),
		(int)round(yt * g_y_scale));
}




void drawRectangle(int col, double w, double h, double x, double y)
{
	XDrawRectangle(
		display,drw,gc[col],
		(int)round(x * g_x_scale),
		(int)round(y * g_y_scale),
		(u_int)round(w * g_x_scale),
		(u_int)round(h * g_y_scale));
}




void fillRectangle(int col, double w, double h, double x, double y)
{
	XFillRectangle(
		display,drw,gc[col],
		(int)round(x * g_x_scale),
		(int)round(y * g_y_scale),
		(u_int)round(w * g_x_scale),
		(u_int)round(h * g_y_scale));
}




void fillPolygon(
	int col, 
	int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
{
	XPoint pnt[4];

	pnt[0].x = (short)round(x1 * g_x_scale);
	pnt[0].y = (short)round(y1 * g_y_scale);
	pnt[1].x = (short)round(x2 * g_x_scale);
	pnt[1].y = (short)round(y2 * g_y_scale);
	pnt[2].x = (short)round(x3 * g_x_scale);
	pnt[2].y = (short)round(y3 * g_y_scale);
	pnt[3].x = (short)round(x4 * g_x_scale);
	pnt[3].y = (short)round(y4 * g_y_scale);

	XFillPolygon(display,drw,gc[col],pnt,4,Complex,CoordModeOrigin);
}




void drawCircle(int col, double w, double h, double x, double y)
{
	XDrawArc(
		display,drw,gc[col],
		(int)round(x * g_x_scale),
		(int)round(y * g_y_scale),
		(u_int)round(w * g_x_scale),
		(u_int)round(h * g_y_scale),
		0,CIRCLE);
}


/******************************** MISC X FUNCTIONS **************************/

void setLineWidth(int col, int w)
{
	XGCValues gcvals;

	gcvals.line_width = (int)round((double)w * g_avg_scale);
	if (gcvals.line_width < 1) gcvals.line_width = 1;
	XChangeGC(display,gc[col],GCLineWidth,&gcvals);
}
