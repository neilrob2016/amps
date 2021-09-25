/****************************************************************************
 The Analogue Modelling Painted Sound (AMPS) monophonic Synthesizer

 Copyright (C) Neil Robertson 2013-2020

 A mouse driven analogue modelling synth with the following features:
	Sine, Square, Triangle, Sawtooth waveforms
	FM
	Echo
	Attack & decay envelopes
	High & low pass filtering
	Resonance
	Ring modulator
	Flanger 
        Phaser
	Arpeggiator
	Chord generator
	Sampler

 ****************************************************************************/

#define MAINFILE
#include "globals.h"
#include "waveforms.h"

char *disp;
XdbeSwapInfo swapinfo;

/*** Forward declarations ***/
void setBuildOptions();
void parseCmdLine(int argc, char **argv);
void showInfo(int extra);
void init();
void Xinit();
void Xloop();
void consoleLoop();
void sigHandler(int sig);


/******************************* START HERE *********************************/
int main(int argc, char **argv)
{
	setBuildOptions();
	parseCmdLine(argc,argv);
	showInfo(1);
	init();
	if (x_mode) Xinit();

	/* Must run after Xinit in case it calls loadFile() which will try 
	   and set the title bar. We don't use loadFile() to load BASIC here
	   because disk.filename could be set to a patch name */
	loadProgram(1);

	/* Start child process after BASIC initialisation has run else can
	   get race conditions in child main loop */
	startSoundDaemon();

	/* Set up signal handler for parent */
	signal(SIGCHLD,sigHandler);
	signal(SIGQUIT,sigHandler);
	signal(SIGINT,sigHandler);
	signal(SIGTERM,sigHandler);

	/* Wait for child to start */
	while(shm->child_status != STAT_RUN);

	if (x_mode) 
		Xloop();
	else
		consoleLoop();
	return 0;
}




void setBuildOptions()
{
	build_options[0] = 0;
#if SOUND==ALSA
	strcat(build_options,"ALSA ");
#elif SOUND==OPENSOUND
	strcat(build_options,"OPENSOUND ");
#elif SOUND==OSX
	strcat(build_options,"OSX ");
#elif SOUND==NO_SOUND
	strcpy(build_options,"NO_SOUND ");
#endif

#ifdef NO_LOG2
	strcat(build_options,"NO_LOG2 ");
#endif
	/* Get rid of trailing space */
	if (build_options[0])
		build_options[strlen(build_options)-1] = 0;
}




void parseCmdLine(int argc, char **argv)
{
	enum
	{
		OPT_VER,
		OPT_NOSND,
		OPT_CON,
		OPT_DE,
		OPT_NT,
		OPT_ADEV,
		OPT_WRD,
		OPT_D,
		OPT_W,
		OPT_H,
		OPT_SIZE,
		OPT_DIR,
		OPT_BAS,
		OPT_ARG,
		OPT_REF,
		OPT_PAT,
		OPT_OSXB,

		NUM_OPTS
	};
	char *opt[NUM_OPTS] =
	{
		"ver",
		"nosnd",
		"con",
		"de",
		"nt",
		"adev",
		"wrd",
		"d",
		"w",
		"h",
		"size",
		"dir",
		"bas",
		"arg",
		"ref",
		"pat",
		"osxb"
	};
		
	double size;
	int i;
	int o;

	disp = NULL;
	use_write_delay = 0;
	write_delay = WRITE_DELAY;
	win_width = WIN_WIDTH;
	win_height = WIN_HEIGHT;
	win_refresh = 1;
	events_debug = 0;
	x_mode = 1;
	draw_tail = 1;
	size = 0;
	basic_file = NULL;
	basic_arg = "";
	do_sound = 1;

#if SOUND==ALSA
	alsa_device = ALSA_DEVICE;
#elif SOUND==OSX
	num_osx_buffers = NUM_OSX_BUFFERS;
#elif SOUND==NO_SOUND
	do_sound = 0;
#endif
	bzero(&disk,sizeof(disk));

	for(i=1;i < argc;++i)
	{
		if (argv[i][0] != '-') goto USAGE;
		for(o=0;o < NUM_OPTS;++o)
			if (!strcasecmp(opt[o],argv[i]+1)) break;
		if (o == NUM_OPTS) goto USAGE;

		/* Flag options */
		switch(o)
		{
		case OPT_VER:
			showInfo(0);
			exit(0);

		case OPT_NOSND:
			do_sound = 0;
			continue;

		case OPT_CON:
			x_mode = 0;
			continue;

		case OPT_DE:
			events_debug = 1;
			continue;

		case OPT_NT:
			draw_tail = 0;
			continue;

		case OPT_ADEV:
#if SOUND==ALSA
			alsa_device = argv[++i];
			break;
#else
			puts("ERROR: The -adev option is not available in this build");
			exit(1);
#endif

		case OPT_WRD:
#if SOUND != OPENSOUND
			puts("ERROR: The -wrd option is not available in this build");
			exit(1);
#else
			use_write_delay = 1;
			if (i < argc - 1 && argv[i+1][0] != '-') 
				write_delay = atoi(argv[++i]);
			continue;
#endif
		}
		if (i == argc - 1) goto USAGE;

		/* Non flag options */
		switch(o)
		{
		case OPT_D:
			disp = argv[++i];
			break;
		
		case OPT_W:
			win_width = atoi(argv[++i]);
			break;

		case OPT_H:
			win_height = atoi(argv[++i]);
			break;

		case OPT_SIZE:
			size = atof(argv[++i]);
			win_height = size * WIN_HEIGHT;
			win_width = size * WIN_WIDTH;
			break;

		case OPT_DIR:
			if (chdir(argv[++i]) == -1)
			{
				perror("ERROR: chdir()");
				exit(1);
			}
			break;

		case OPT_BAS:
			/* Set basic_file */
			basic_file = strdup(argv[++i]);
			break;

		case OPT_ARG:
			/* Set value for BASIC variable $arg */
			basic_arg = argv[++i];
			break;

		case OPT_REF:
			if ((win_refresh = atoi(argv[++i])) < 1)
			{
				printf("ERROR: Invalid refresh value %s\n",argv[i]);
				exit(1);
			}
			break;

		case OPT_PAT:
			strcpy(disk.filename,argv[++i]);
			disk.pos = strlen(disk.filename);
			disk.op = DISK_LOAD_PATCH;
			addFileSuffix();
			break;
#if SOUND==OSX
		case OPT_OSXB:
			if ((num_osx_buffers = atoi(argv[++i])) < 1)
			{
				puts("ERROR: Invalid OSX audio buffer count. Must be > 0.\n");
				exit(1);
			}
			break;
#endif
		default:
			goto USAGE;
		}
	}
	if (win_width < 0 || win_height < 0) 
	{
		printf("ERROR: Invalid window size %d,%d\n",win_width,win_height);
		exit(1);
	}
	if (write_delay < 0)
	{
		printf("ERROR: Invalid write delay value %d\n",write_delay);
		exit(1);
	}
	return;

	USAGE:
	printf("Usage: %s\n"
	       "       -d    <X display> : Defaults to NULL\n"
	       "       -w    <pixels>    : Window width (0 = screen width). Default = %d\n"
	       "       -h    <pixels>    : Window height (0 = screen height). Default = %d\n"
	       "       -size <0 -> N>    : An alternative to -w/h. Set relative window size\n"
	       "                           compared to default. Eg 0.5 means half size, 2 means\n"
	       "                           double size.\n"
	       "       -dir  <directory> : Set the working directory\n"
	       "       -pat  <.amp file> : Patch file to load at startup\n"
	       "       -bas  <.bas file> : BASIC program to load at startup\n"
	       "       -arg  <string>    : Sets the BASIC variable $arg to this string.\n"
#if SOUND==ALSA
	       "       -adev <device>    : Sets the ALSA device to use\n"
#elif SOUND==OPENSOUND
	       "       -wrd  [<usec>]    : Have a delay after each write() when using OSS. Use\n"
	       "                           this if the sound is playing catch up with the mouse\n"
	       "                           or keyboard which is caused by too many write loops.\n"
	       "                           Default = %d usecs\n"
#elif SOUND==OSX
	       "       -osxb <count>     : Number of OS/X audio buffers to use. Default = %d\n"
#endif
	       "       -ref  <iter>      : Refresh window every 'iter' mainloop iterations. Use\n"
	       "                           this if your X server can't cope with the refresh\n"
	       "                           rate. Higher iter = lower refresh rate. Default = 1\n"
	       "       -nosnd            : Don't open sound device - visuals only\n"
	       "       -con              : Run in console only mode - ie no X windows. Just get\n"
	       "                           input from the keyboard.\n"
	       "       -de               : Dump events playing to console\n"
	       "       -nt               : Don't draw the coloured mouse pointer tail\n"
	       "       -ver              : Exit after printing build details\n"
		,argv[0]
		,WIN_WIDTH
		,WIN_HEIGHT
#if SOUND==OPENSOUND
		,WRITE_DELAY
#elif SOUND==OSX
		,NUM_OSX_BUFFERS
#endif
		);
	exit(1);
}




void showInfo(int extra)
{
	puts("\n-={ AMPS Synth }=-\n");
	puts(COPYRIGHT1);
	printf("\nBuild date   : %s\n",BUILD_DATE);
	printf("Build options: %s\n",
		build_options[0] ? build_options : "<none>");
	printf("Version      : %s\n",VERSION);
#if SOUND==ALSA
	strcpy(sound_system,"ALSA");
#elif SOUND==OPENSOUND
	strcpy(sound_system,"OpenSound");
#elif SOUND==OSX
	strcpy(sound_system,"OSX");
#elif SOUND==NO_SOUND
	strcpy(sound_system,"NO SOUND");
#endif
	printf("Sound system : %s\n",sound_system);

	if (extra)
	{
		printf("Working dir  : %s\n",getcwd(dirbuff,sizeof(dirbuff)));
		printf("Main PID     : %u\n",getpid());
	}
	else putchar('\n');
}




/*** General setup ***/
void init()
{
	struct termios tio;
	int shmid;
	int i;

	boot_time = getUsecTimeAsDouble();

	// Use usec in case 2 or more instances started together when time(0)
	// wouldn't be granular enough
	srandom(getUsecTime());  

	initBasic();
	show_section_status = 0;

	/* Set up keyboard non buffered input if we're going to run in console
	   mode. Do this before BASIC init section runs */
	if (!x_mode)
	{
		tcgetattr(0,&tio);
		saved_tio = tio;
		tio.c_lflag &= ~ICANON;
		tio.c_lflag &= ~ECHO;
		tio.c_cc[VTIME] = 0;
		tio.c_cc[VMIN] = 1;
		tcsetattr(0,TCSANOW,&tio);

		win_width = 0;
		win_height = 0;
		display_width = 0;
		display_height = 0;
	}

	/* Set up shared memory. 20 attempts at finding a random key that
	   is currently unused. Using ftok() is a waste of time */
	for(i=0,shmid=-1;i < 20 && shmid == -1;++i)
	{
		shmid = shmget(
			(key_t)random(),
			sizeof(struct st_sharmem),IPC_CREAT | IPC_EXCL | 0600
			);
	}
	if (shmid == -1)
	{
		printf("ERROR: shmget(): %s\n",strerror(errno));
		exit(1);
	}

	if ((shm = (struct st_sharmem *)shmat(shmid,NULL,0)) == (void *)-1)
	{
		printf("ERROR: shmat(): %s\n",strerror(errno));
		exit(1);
	}

	/* Mark for deletion when both processes have exited */
	shmctl(shmid,IPC_RMID,0);
	sndbuff = shm->sndbuff;
	resetSharedMemory();
	shm->child_status = STAT_STOPPED;

	createNotes();

	win_mapped = 0;
	intro_cnt = 0;
	do_messages = 1;
	freeze_waveform = 0;
	list_dir = NULL;
	patch_file = NULL;
	evr_events = NULL;
	prev_note_scale = -1;

	/* Get home directory - try $HOME first. Required for ~ in load/save */
	if (!(home_dir = getenv("HOME")) || !home_dir[0])
		home_dir = getpwuid(getuid())->pw_dir;

	for(i=0;i < TAIL_LEN;++i)
	{
		tail[i].x = -DIAM;
		tail[i].y = -DIAM;
		tail[i].colour = BLACK;
	}
	tail_next = 0;

	/* Set up waveform table */
	waveform[0] = (struct st_waveform *)&snd_AAH;
	waveform[1] = (struct st_waveform *)&snd_OOH;

	/* Set up text & ascii tables */
	bzero(mesg,sizeof(mesg));
	next_mesg = 0;
	for(i=0;i < 256;++i) ascii_table[i] = NULL;

	ascii_table[(int)'A'] = (st_char_template *)&char_A;
	ascii_table[(int)'B'] = (st_char_template *)&char_B;
	ascii_table[(int)'C'] = (st_char_template *)&char_C;
	ascii_table[(int)'D'] = (st_char_template *)&char_D;
	ascii_table[(int)'E'] = (st_char_template *)&char_E;
	ascii_table[(int)'F'] = (st_char_template *)&char_F;
	ascii_table[(int)'G'] = (st_char_template *)&char_G;
	ascii_table[(int)'H'] = (st_char_template *)&char_H;
	ascii_table[(int)'I'] = (st_char_template *)&char_I;
	ascii_table[(int)'J'] = (st_char_template *)&char_J;
	ascii_table[(int)'K'] = (st_char_template *)&char_K;
	ascii_table[(int)'L'] = (st_char_template *)&char_L;
	ascii_table[(int)'M'] = (st_char_template *)&char_M;
	ascii_table[(int)'N'] = (st_char_template *)&char_N;
	ascii_table[(int)'O'] = (st_char_template *)&char_O;
	ascii_table[(int)'P'] = (st_char_template *)&char_P;
	ascii_table[(int)'Q'] = (st_char_template *)&char_Q;
	ascii_table[(int)'R'] = (st_char_template *)&char_R;
	ascii_table[(int)'S'] = (st_char_template *)&char_S;
	ascii_table[(int)'T'] = (st_char_template *)&char_T;
	ascii_table[(int)'U'] = (st_char_template *)&char_U;
	ascii_table[(int)'V'] = (st_char_template *)&char_V;
	ascii_table[(int)'W'] = (st_char_template *)&char_W;
	ascii_table[(int)'X'] = (st_char_template *)&char_X;
	ascii_table[(int)'Y'] = (st_char_template *)&char_Y;
	ascii_table[(int)'Z'] = (st_char_template *)&char_Z;

	ascii_table[(int)'a'] = (st_char_template *)&char_a;
	ascii_table[(int)'b'] = (st_char_template *)&char_b;
	ascii_table[(int)'c'] = (st_char_template *)&char_c;
	ascii_table[(int)'d'] = (st_char_template *)&char_d;
	ascii_table[(int)'e'] = (st_char_template *)&char_e;
	ascii_table[(int)'f'] = (st_char_template *)&char_f;
	ascii_table[(int)'g'] = (st_char_template *)&char_g;
	ascii_table[(int)'h'] = (st_char_template *)&char_h;
	ascii_table[(int)'i'] = (st_char_template *)&char_i;
	ascii_table[(int)'j'] = (st_char_template *)&char_j;
	ascii_table[(int)'k'] = (st_char_template *)&char_k;
	ascii_table[(int)'l'] = (st_char_template *)&char_l;
	ascii_table[(int)'m'] = (st_char_template *)&char_m;
	ascii_table[(int)'n'] = (st_char_template *)&char_n;
	ascii_table[(int)'o'] = (st_char_template *)&char_o;
	ascii_table[(int)'p'] = (st_char_template *)&char_p;
	ascii_table[(int)'q'] = (st_char_template *)&char_q;
	ascii_table[(int)'r'] = (st_char_template *)&char_r;
	ascii_table[(int)'s'] = (st_char_template *)&char_s;
	ascii_table[(int)'t'] = (st_char_template *)&char_t;
	ascii_table[(int)'u'] = (st_char_template *)&char_u;
	ascii_table[(int)'v'] = (st_char_template *)&char_v;
	ascii_table[(int)'w'] = (st_char_template *)&char_w;
	ascii_table[(int)'x'] = (st_char_template *)&char_x;
	ascii_table[(int)'y'] = (st_char_template *)&char_y;
	ascii_table[(int)'z'] = (st_char_template *)&char_z;

	ascii_table[(int)'0'] = (st_char_template *)&char_0;
	ascii_table[(int)'1'] = (st_char_template *)&char_1;
	ascii_table[(int)'2'] = (st_char_template *)&char_2;
	ascii_table[(int)'3'] = (st_char_template *)&char_3;
	ascii_table[(int)'4'] = (st_char_template *)&char_4;
	ascii_table[(int)'5'] = (st_char_template *)&char_5;
	ascii_table[(int)'6'] = (st_char_template *)&char_6;
	ascii_table[(int)'7'] = (st_char_template *)&char_7;
	ascii_table[(int)'8'] = (st_char_template *)&char_8;
	ascii_table[(int)'9'] = (st_char_template *)&char_9;

	ascii_table[(int)' '] = (st_char_template *)&char_space;
	ascii_table[(int)'?'] = (st_char_template *)&char_qmark;
	ascii_table[(int)'!'] = (st_char_template *)&char_exmark;
	ascii_table[(int)'+'] = (st_char_template *)&char_plus;
	ascii_table[(int)'-'] = (st_char_template *)&char_minus;
	ascii_table[(int)'*'] = (st_char_template *)&char_star;
	ascii_table[(int)'='] = (st_char_template *)&char_equals;
	ascii_table[(int)'.'] = (st_char_template *)&char_dot;
	ascii_table[(int)','] = (st_char_template *)&char_comma;
	ascii_table[(int)'('] = (st_char_template *)&char_lrbracket;
	ascii_table[(int)')'] = (st_char_template *)&char_rrbracket;
	ascii_table[(int)'{'] = (st_char_template *)&char_lcbracket;
	ascii_table[(int)'}'] = (st_char_template *)&char_rcbracket;
	ascii_table[(int)'['] = (st_char_template *)&char_lsbracket;
	ascii_table[(int)']'] = (st_char_template *)&char_rsbracket;
	ascii_table[(int)'$'] = (st_char_template *)&char_dollar;
	ascii_table[(int)'#'] = (st_char_template *)&char_hash;
	ascii_table[(int)'/'] = (st_char_template *)&char_fslash;
	ascii_table[(int)'\\'] = (st_char_template *)&char_bslash;
	ascii_table[(int)'>'] = (st_char_template *)&char_greater;
	ascii_table[(int)'<'] = (st_char_template *)&char_less;
	ascii_table[(int)'_'] = (st_char_template *)&char_underscore;
	ascii_table[(int)'|'] = (st_char_template *)&char_bar;
	ascii_table[(int)'\''] = (st_char_template *)&char_squote;
	ascii_table[(int)'"'] = (st_char_template *)&char_dquote;
	ascii_table[(int)'`'] = (st_char_template *)&char_bquote;
	ascii_table[(int)':'] = (st_char_template *)&char_colon;
	ascii_table[(int)';'] = (st_char_template *)&char_semicolon;
	ascii_table[(int)'@'] = (st_char_template *)&char_at;
	ascii_table[(int)'^'] = (st_char_template *)&char_hat;
	ascii_table[(int)'~'] = (st_char_template *)&char_tilda;
	ascii_table[(int)'&'] = (st_char_template *)&char_ampersand;
	ascii_table[(int)'%'] = (st_char_template *)&char_percent;

	/* Set up hash table for button and GS field names */
	hcreate((NUM_BUTTONS + NUM_GETSET_FIELDS) * 2);

	setupButtons();
	setupGSFields();
	paramsReset(0,1);
	evrInit();
}




/*** Called from init() & reset button ***/
void paramsReset(int reset_disk, int reset_evr)
{
	if (reset_disk) bzero(&disk,sizeof(disk));
	escape_cnt = 0;
	load_prog_cnt = 0;
	show_credits = 0;
	strcpy(freq_text,"0");
	strcpy(sample_speed_text,"x1.00");
	freq_text_len = 1;

	bzero(&params,sizeof(params));
	params.win_height = win_height;
	params.freq_range = FREQ_RANGE_DIAL_INIT;
	params.analyser_range = ANALYSER_RANGE_INIT;
	params.key_start_note = middle_c;

	hover_str = NULL;
	if (reset_evr) evrStop();
	setByRange();
	resetButtons(1,reset_evr);
}




/*** Set up X ***/
void Xinit()
{
	Atom delete_notify;
	XGCValues gcvals;
	XColor colour;
	XColor unused;
	Colormap cmap;
	int screen;
	int white;
	int black;
	int stage;
	int col;
	u_char r,g,b;
	char colstr[5];

	if (!(display = XOpenDisplay(disp)))
	{
		printf("ERROR: Can't connect to: %s\n",XDisplayName(disp));
		exit(1);
	}
	screen = DefaultScreen(display);
	black = BlackPixel(display,screen);
	white = WhitePixel(display,screen);
	cmap = DefaultColormap(display,screen);

	display_width = DisplayWidth(display,screen);
	display_height = DisplayHeight(display,screen);
	if (!win_width) win_width = display_width;
	if (!win_height) win_height = display_height;

	win = XCreateSimpleWindow(
		display,
		RootWindow(display,screen),
		0,0,win_width,win_height,0,white,black);

	XSetWindowBackground(display,win,black);
	setTitleBar(NULL);

	drw = (Drawable)XdbeAllocateBackBufferName(
		display,win,XdbeBackground);
	swapinfo.swap_window = win;
	swapinfo.swap_action = XdbeBackground;

	r = 0;
	g = 0xF;
	b = 0;
	stage = 1;

	gcvals.foreground = white;
	gcvals.line_width = 1;
	gc[WHITE] = XCreateGC(display,win,GCForeground | GCLineWidth,&gcvals);

	for(col=0;col < NUM_COLOURS;++col)
	{
		sprintf(colstr,"#%01X%01X%01X",r,g,b);

		if (!XAllocNamedColor(display,cmap,colstr,&colour,&unused))
		{
			printf("WARNING: Can't allocate colour %s\n",colstr);
			gcvals.foreground = white;
		}
		else gcvals.foreground = colour.pixel;

		gcvals.line_width = 1;
		gc[col] = XCreateGC(display,win,GCForeground | GCLineWidth,&gcvals);

		switch(stage)
		{
		case 1:
			/* Green to turquoise */
			if (++b == 0xF) stage = 2;
			break;

		case 2:
			/* Turquoise to blue */
			if (!--g) stage = 3;
			break;

		case 3:
			/* Blue to mauve */
			if (++r == 0xF) stage = 4;
			break;

		case 4:
			/* Mauve to red */
			if (!--b) stage = 5;
			break;

		case 5:
			/* Red to yellow */
			if (++g == 0xF) stage = 6;
			break;

		case 6:
			/* Yellow to black */
			--r;
			--g;
		}
	}
	windowResized();

	XSelectInput(display,win,
		ExposureMask | 
		StructureNotifyMask |
		PointerMotionMask |
		KeyPressMask | KeyReleaseMask |
		ButtonPressMask | ButtonReleaseMask);

	/* Don't kill process when window closed, send ClientMessage event
	   instead */
	delete_notify = XInternAtom(display,"WM_DELETE_WINDOW",True);
	XSetWMProtocols(display,win,&delete_notify,1);

	XMapWindow(display,win);
	XAutoRepeatOff(display);
}



/********************************** RUN *************************************/

/*** Mainloop if in X mode. Get the events and draw the points ***/
void Xloop()
{
	XEvent event;
	int cnt;
	short mouse_x;
	short mouse_y;
	u_int last_motion;
	u_int now;
	int usec;

	first_mouse_button = 0;
	ignore_mouse_release = 0;
	mouse_x = 0;
	mouse_y = 0;
	last_motion = getUsecTime();

	/* If we have a patch to load then do it */
	if (disk.op == DISK_LOAD_PATCH) loadFile(1);

	for(cnt=0;;++cnt)
	{
		now = getUsecTime();

		/* If recorder playing then deal with its events */
		if (evr_state == RECORDER_PLAY) evrPlayEvents(now);

		/* Deal with X Events */
		while(XPending(display))
		{
			XNextEvent(display,&event);

			switch(event.type)
			{
			case ConfigureNotify:
				/* Don't call event if the size hasn't 
				   changed */
				if (event.xconfigure.width != win_width ||
				    event.xconfigure.height != win_height)
				{
					win_width = event.xconfigure.width;
					win_height = event.xconfigure.height;
					windowResized();
					runEventSection(SECTION_WINDOW,0,0,NULL);
				}
				break;

			case MapNotify:
				win_mapped = 1;
				runEventSection(SECTION_WINDOW,0,0,NULL);
				break;

			case UnmapNotify:
				win_mapped = 0;
				runEventSection(SECTION_WINDOW,0,0,NULL);
				break;

			case ButtonPress:
				/* Disable mouse events when playing */
				mousePressed(
					event.xbutton.button,
					event.xbutton.x,event.xbutton.y);
				break;

			case ButtonRelease:
				/* ignore_mouse_release set in buttons.c */
				if (ignore_mouse_release)
					ignore_mouse_release = 0;
				else
					mouseReleased(event.xbutton.button);
				break;

			case MotionNotify:
				if (evr_state == RECORDER_PLAY) break;

				/* Do something with motion if a mouse
				   button is pressed */
				mouse_x = event.xmotion.x;
				mouse_y = event.xmotion.y;
				if (first_mouse_button) 
				{
					mouseData(
						first_mouse_button,
						mouse_x,mouse_y,1);
				}
				last_motion = now;
				hover_str = NULL;
				break;

			case KeyPress:
				keyPressed(&event);
				break;

			case KeyRelease:
				keyReleased(&event);
				break;

			case ClientMessage:
				quit(QUIT_XKILL);
			
			default:
				break;
			}
		}

		/* Countdowns for escape key being pressed twice and load prog
		   button clicked twice */
		if (escape_cnt) --escape_cnt;
		if (load_prog_cnt) --load_prog_cnt;

		/* So buttons flick when pressed */
		updateClickButtons();

		/* Spectrum analyser */
		if (params.show_analyser) calcDFT();

		/* Go through patch listing */
		if (list_dir) listFiles(NULL);

		/* If mouse hasn't moved for a short while see if its hovering
		   over a button and if it is then set hover value */
		if (now - last_motion > 500000)
		{
			setHoverString(mouse_x,mouse_y);
			last_motion = now;
		}

		updateMessages();

		/* Draw every win_refresh iteration so we don't overload X */
		if (!(cnt % win_refresh))
		{
			draw();
			XdbeSwapBuffers(display,&swapinfo,1);
			XFlush(display);
		}

		/* Running basic is considered part of the sleep time when the
		   GUI is not being updated */
		now = getUsecTime();
		runNonEventSections();

		usec = USEC_LOOP_DELAY - (getUsecTime() - now);

		/* usec could be greater if getUsecTime() has wrapped */
		if (usec <= USEC_LOOP_DELAY) usleep(usec);
	}
}




/*** Set various vars that are affected by window size change ***/
void windowResized()
{
	int i;

	g_y_scale = (double)win_height / WIN_HEIGHT;
	g_x_scale = (double)win_width / WIN_WIDTH;
	g_avg_scale = (g_x_scale + g_y_scale) / 2;
	shm->win_width = (u_short)win_width;
	params.win_height = win_height;

	/* Reset line width because its dependent on g_avg_scale */
	for(i=0;i <= NUM_COLOURS;++i) setLineWidth(i,1);
}




/*** Mainloop if in console mode ***/
void consoleLoop()
{
	struct timeval tv;
	fd_set mask;
	u_int now;
	int usec;
	int cnt;
	int is_note_key;

	/* If we have a patch to load then do it */
	if (disk.op == DISK_LOAD_PATCH) loadFile(1);

	for(cnt=0;;)
	{
		if (evr_state == RECORDER_PLAY) 
			evrPlayEvents(getUsecTime());

		FD_ZERO(&mask);
		FD_SET(STDIN,&mask);

		now = getUsecTime();
		runNonEventSections();

		/* Countdown for escape key being pressed twice */
		if (escape_cnt) --escape_cnt;

		usec = USEC_LOOP_DELAY - (getUsecTime() - now);
		if (usec > USEC_LOOP_DELAY) usec = 0;
		tv.tv_sec = 0;
		tv.tv_usec = usec;

		if (select(FD_SETSIZE,&mask,0,0,&tv) == 1) 
		{
			cnt = 20;
			is_note_key = 0;
			if (!processConsoleData(&is_note_key)) goto ERROR;
		}
		else --cnt;

		/* Have to autorelease since we don't get a key release
		   event from terminals */
		if (is_note_key &&
		    !cnt && !params.hold_note && shm->play) playOff(1);
	}

	ERROR:
	puts("ERROR: Can't read from STDIN");
	exit(1);
}




/*** Catch child process dying ***/
void sigHandler(int sig)
{
	int status;
	int csig;

	if (sig != SIGCHLD) quit(QUIT_SIGNAL);

	waitpid(-1,&status,WNOHANG);

	if (WIFSIGNALED(status))
	{
		/* WIFSIGNALED() seems to return true even if child only
		   suspended due to SIGSTOP so check signal value */
		if ((csig = WTERMSIG(status)) > 30) return;
		
		printf("ERROR: Sound process crashed/exited with signal %d ",WTERMSIG(status));
		if (WCOREDUMP(status))
			puts("(core dumped)");
		else
			putchar('\n');

		resetKeyboard();
		exit(1);
	}
}
