/*** Various functions to control loading and saving patches ***/

#include "globals.h"

#define PATCH_MAGIC  0xAD55  /* Amps Digital Synth System */
#define EVENTS_MAGIC 0xAD56  /* If we have events saved too */

enum
{
	NOT_FOUND,
	LOAD_FILE,
	LOAD_DIR
};


char matchpath[PATH_MAX+1];

int match(char *path, char *str);
int wildmatch(char *str, char *pat);
int savePatch();
int saveProgram();
void writeSetStrStr(FILE *fp, char *s1, char *s2);
void writeSetStrInt(FILE *fp, char *s, int i);
void writeSetStrDouble(FILE *fp, char *s, double d);


/********************************* LOADING **********************************/

void loadProgramMode()
{
	clearDiskStruct();
	disk.op = DISK_LOAD_PROG;
	runEventSection(SECTION_BUTTON,BUT_LOAD_PROG,1,"");
}




void loadPatchMode()
{
	clearDiskStruct();
	disk.op = DISK_LOAD_PATCH;
	runEventSection(SECTION_BUTTON,BUT_LOAD_PATCH,1,"");
}




/*** Convert to network byte order for saving. In most systems htons/l() and 
     ntohs/l() do exactly the same thing and are interchangable but keep them 
     seperate just in case. */
void hostToNetworkByteOrder(struct st_recorder_event *ev)
{
	ev->time_offset = htonl(ev->time_offset);
	ev->ksym = htonl(ev->ksym); 
	ev->x = htons(ev->x);
	ev->y = htons(ev->y);
}




/*** Convert to host byte order for loading. ***/
void networkToHostByteOrder(struct st_recorder_event *ev)
{
	ev->time_offset = ntohl(ev->time_offset);
	ev->ksym = ntohl(ev->ksym); 
	ev->x = ntohs(ev->x);
	ev->y = ntohs(ev->y);
}




/*** Load a patch or Basic program ***/
int loadFile(int patch)
{
	struct st_recorder_event *ev;
	u_short magic;
	u_int ec;
	u_int e;
	int ret;
	int fd;
	int len;
	int shmlen;
	int funcret;

	if (patch) pauseSoundProcess();

	fd = -1;
	funcret = 0;

	matchpath[0] = 0;

	/* Find path/filename match */
	switch(disk.filename[0])
	{
	case '/':
		ret = match("/",disk.filename+1);
		break;

	case '~':
		if (disk.filename[1] != '/')
		{
			message("ERROR: Invalid filename");
			goto RESTART;
		}
		ret = match(home_dir,disk.filename+2);
		break;

	default:
		/* If we're loading a program and the filename given is a
		   '#' then we want to reload previous program if there was
		   one */
		if (!patch && basic_file && !strcmp(disk.filename,"#.bas"))
		{
			strcpy(matchpath,basic_file);
			ret = LOAD_FILE;
		}
		else ret = match(".",disk.filename);
		break;
	}

	switch(ret)
	{
	case NOT_FOUND:
		message("<No matching file(s)>");
		goto RESTART;

	case LOAD_FILE:
		if (!patch)
		{
			FREE(basic_file);
			basic_file = strdup(matchpath);
			loadProgram(0);
			clearDiskStruct();
			return 1;
		}
		break;

	case LOAD_DIR:
		listFiles(patch ? PATCH_SUFFIX : BASIC_SUFFIX);
		goto RESTART;

	default:
		assert(0);
	}
	
	if ((fd = open(matchpath,O_RDONLY)) == -1)
	{
		message("ERROR: open(): %s",strerror(errno));
		/* Go to restart since haven't done bzero yet so current patch 
		   unchanged */
		goto RESTART;
	}

	/* Get and check magic number first */
	if ((len = read(fd,&magic,sizeof(magic))) < sizeof(magic))
	{
		message("ERROR: Unexpected EOF");
		goto RESTART;
	}

	magic = ntohs(magic);
	if (magic != PATCH_MAGIC && magic != EVENTS_MAGIC)
	{
		message("ERROR: Not an AMPS file - invalid magic number 0x%02X",magic);
		goto RESTART; 
	}

	/* Load patch */
	bzero(&params,sizeof(params));
	shmlen = sizeof(struct st_sharmem) - SNDBUFF_BYTES;
	bzero(shm,shmlen);
	patch_file = basename(matchpath);

	if ((len = read(fd,&params,sizeof(params))) < sizeof(params) ||
	    (len = read(fd,shm,shmlen)) < shmlen)
	{
		/* These can cause a crash values are bad */
		resetEffectsSeq(0);
		goto READ_ERROR;
	}

	/* Load events if its that type of patch file */
	if (magic == EVENTS_MAGIC)
	{
		evrClear(0);

		memcpy(&evr_params,&params,sizeof(params));
		memcpy(&evr_sharmem,shm,sizeof(struct st_sharmem));

		if ((len = read(fd,&ec,sizeof(ec))) < sizeof(ec)) 
			goto READ_ERROR;

		ec = ntohl(ec);
		for(e=0;e < ec;++e)
		{
			if (!(ev = evrCreateNewEvent())) break;
			if ((len = read(fd,ev,sizeof(*ev))) < sizeof(*ev))
			{
				--evr_events_cnt;
				goto READ_ERROR;
			}
			networkToHostByteOrder(ev);
		}
		message("Loaded '%s' with %d events",matchpath,ec);
	}
	else message("Loaded '%s'",matchpath);

	funcret = 1;

	READ_ERROR:
	if (!funcret)
	{
		if (len == -1)
			message("ERROR: read(): %s",strerror(errno));
		else
			message("ERROR: Unexpected EOF");
		patch_file = "<load failed>";
	}

	setTitleBar(NULL);
	normalisePatch(x_mode);

	RESTART:
	if (fd != -1) close(fd);
	restartSoundProcess();
	clearDiskStruct();

	return funcret;
}




/*** matchpath will contain a directory - list all the patch or BASIC files 
     in it ***/
void listFiles(char *suffix)
{
	static int timer = 0;
	static int cnt = 0;
	static char *stat_suffix = NULL;
	struct dirent *ds;
	int len;
	int slen;

	if (!list_dir)
	{
		if (!(list_dir = opendir(matchpath)))
		{
			printf("ERROR: Can't open dir '%s': %s\n",
				matchpath,strerror(errno));
		}
		cnt = 0;
		stat_suffix = suffix;
		return;
	}

	/* Already opened - show one file every given interval (otherwise
	   they'll all be squashed together on screen) */
	if (++timer < 5 * win_refresh) return;
	timer = 0;
	while ((ds = readdir(list_dir))) 
	{
		len = strlen(ds->d_name);
		slen = strlen(stat_suffix);
		if (len > slen && !strcmp(ds->d_name+len-slen,stat_suffix))
		{
			++cnt;
			message(ds->d_name);
			return;
		}
	}

	if (!cnt) message("<No files found>");
	closedir(list_dir);
	list_dir = NULL;
}




/*** Find the first path that matches str. The reason this recurses instead of
     just opening the path directly is so we can match individual parts of
     the path that have wildcards. eg: ~/source/p*s/d*y ***/
int match(char *dirname, char *str)
{
	DIR *dir;
	struct dirent *ds;
	struct stat fs;
	char path[PATH_MAX+1];
	char *ptr;
	char c = 0; /* Init to stop compiler warning */
	int ret;
	
	if (!str) return NOT_FOUND;

	if (!*str)
	{
		strcpy(matchpath,dirname);
		return LOAD_DIR;
	}

	/* Check for current dir listing */
	if (!strcmp(str,"./"))
	{
		strcpy(matchpath,".");
		return LOAD_DIR;
	}

	/* Remove trailing slash */
	if ((ptr = strchr(str,'/')))
	{
		c = *ptr;
		*ptr = 0;
	}
	else ptr = NULL;

	if (!(dir = opendir(dirname)))
	{
		printf("ERROR: Can't open dir '%s': %s\n",dirname,strerror(errno));
		if (ptr) *ptr = c;
		return NOT_FOUND;
	}

	ret = NOT_FOUND;

	while((ds = readdir(dir)))
	{
		if (!strcmp(ds->d_name,".") || 
		    (!strcmp(ds->d_name,"..") && strcmp(str,"..")) ||
		    !wildmatch(ds->d_name,str)) continue;

		if (strcmp(dirname,"/"))
			snprintf(path,PATH_MAX,"%s/%s",dirname,ds->d_name);
		else
			snprintf(path,PATH_MAX,"%s%s",dirname,ds->d_name);

		/* If ptr is set then str is a path, not a filename */
		if (ptr)
		{
			lstat(path,&fs);

			/* If its a directory then descend further unless 
			   str ends in a '/' so we're looking for dir match */
			if ((fs.st_mode & S_IFMT) == S_IFDIR)
			{
				if (!*(ptr+1))
				{
					strncpy(matchpath,path,PATH_MAX);
					ret = LOAD_DIR;
					goto DONE;
				}
				if ((ret = match(path,ptr+1)) != NOT_FOUND)
					goto DONE;
			}
		}
		else
		{
			strncpy(matchpath,path,PATH_MAX);
			ret = LOAD_FILE;
			break;
		}
	}

	DONE:
	closedir(dir);
	if (ptr) *ptr = c;
	return ret;
}




/*** Returns 1 if the string matches the pattern, else 0. Supports wildcard
     patterns containing '*' and '?' ***/
int wildmatch(char *str, char *pat)
{
	char *s,*p,*s2;

	for(s=str,p=pat;*s && *p;++s,++p)
	{
		switch(*p)
		{
		case '?':
			continue;

		case '*':
			if (!*(p+1)) return 1;

			for(s2=s;*s2;++s2)
				if (wildmatch(s2,p+1)) return 1;
			return 0;
		}
		if (*s != *p) return 0;
	}

	return (!*s && !*p);
}


/*********************************** SAVING ********************************/


void savePatchMode()
{
	clearDiskStruct();
	disk.op = DISK_SAVE_PATCH;
	runEventSection(SECTION_BUTTON,BUT_SAVE_PATCH,1,"");
}




void saveProgramMode()
{
	clearDiskStruct();
	disk.op = DISK_SAVE_PROG;
	runEventSection(SECTION_BUTTON,BUT_SAVE_PROG,1,"");
}




/*** Save a patch either as a binary or a BASIC program ***/
int saveFile(int patch)
{
	char tmp_path[PATH_MAX+1];

	pauseSoundProcess();

	if (strchr(disk.filename,'*') || strchr(disk.filename,'?'))
	{
		message("ERROR: Invalid filename");
		restartSoundProcess();
		return 0;
	}

	/* Add in home dir */
	if (disk.filename[0] == '~')
	{
		if (disk.filename[1] != '/')
		{
			message("ERROR: Invalid filename");
			restartSoundProcess();
			return 0;
		}

		snprintf(tmp_path,PATH_MAX,"%s%s",home_dir,disk.filename+1);
		strcpy(disk.filename,tmp_path);
	}

	if (patch) return savePatch();

	return saveProgram();
}




/*** Save the patch as a binary file ***/
int savePatch()
{
	struct st_recorder_event *ev;
	struct st_sharmem *save_shm;
	struct st_params *save_params;
	u_short magic;
	u_int ec;
	u_int e;
	int fd;
	int err;
	int len;
	int shmlen;
	int funcret;

	fd = -1;
	funcret = 0;
	if ((fd = open(disk.filename,O_WRONLY | O_CREAT | O_TRUNC,0600)) == -1)
	{
		message("ERROR: open(): %s",strerror(errno));
		restartSoundProcess();
		return 0;
	}

	/* Different magic number for pure patch or patch + recorder events */
	if (evr_flags.events_in_save && evr_events_cnt)
	{
		magic = htons(EVENTS_MAGIC);
		save_shm = &evr_sharmem;
		save_params = &evr_params;
	}
	else
	{
		magic = htons(PATCH_MAGIC);
		save_shm = shm;
		save_params = &params;
	}

	/* Convert byte orders */
	shm->win_width = htons(win_width);
	shm->freq = htons(shm->freq);
	params.win_height = htons(win_height);

	/* Save patch */
	shmlen = sizeof(struct st_sharmem) - SNDBUFF_BYTES;
	if ((len = write(fd,&magic,sizeof(magic))) < sizeof(magic) ||
	    (len = write(fd,save_params,sizeof(params))) < sizeof(params) ||
	    (len = write(fd,save_shm,shmlen)) < shmlen)
		err = 1;
	else
		err = 0;

	/* Convert back */
	shm->win_width = win_width;
	shm->freq = ntohs(shm->freq);
	if (err) goto WRITE_ERROR;

	/* Save events */
	if (evr_flags.events_in_save && evr_events_cnt)
	{
		ec = htonl(evr_events_cnt);
		if ((len = write(fd,&ec,sizeof(ec))) < sizeof(ec))
			goto WRITE_ERROR;

		for(e=0;e < evr_events_cnt;++e)
		{
			ev = &evr_events[e];

			/* Convert byte order */
			hostToNetworkByteOrder(ev);

			/* Disk write will always return entire length or 
			   error */
			len = write(fd,ev,sizeof(struct st_recorder_event));

			/* Convert back */
			networkToHostByteOrder(ev);

			if (len < sizeof(struct st_recorder_event))
				goto WRITE_ERROR;
		}
		message("Saved '%s' with %d events",disk.filename,evr_events_cnt);
	}
	else message("Saved '%s'",disk.filename);

	funcret = 1;
	patch_file = basename(disk.filename);
	setTitleBar(NULL);

	WRITE_ERROR:
	if (!funcret)
	{
		if (len == -1)
			message("ERROR: write(): %s",strerror(errno));
		else
			message("ERROR: Failed to write all data");
	}

	close(fd);
	restartSoundProcess();

	return funcret;
}




/*** Save the patch (no events) as a BASIC program ***/
int saveProgram()
{
	struct st_button *butt;
	FILE *fp;
	char *name;
	int i;

	/* High level I/O so I can use fprintf() */
	if (!(fp = fopen(disk.filename,"w")))
	{
		message("ERROR: fopen(): %s",strerror(errno));
		restartSoundProcess();
		return 0;
	}
	/* I could save some of these values as numbers but using the actual
	   names is clearer for someone looking at the code. Save SYS:
	   variables first. */
	fprintf(fp,"section init\n	reset\n");
	writeSetStrStr(fp,"SYS:MAIN_OSC",sound_name[shm->sound]);
	writeSetStrStr(fp,"SYS:SCALE",scale_short_name[shm->note_scale]);
	writeSetStrInt(fp,"SYS:KEY_START_NOTE",params.key_start_note);
	writeSetStrInt(fp,"SYS:FILL",params.fill_waveform);
	writeSetStrInt(fp,"SYS:MOUSE_TAIL",draw_tail);
	writeSetStrInt(fp,"SYS:WIN_WIDTH",win_width);
	writeSetStrInt(fp,"SYS:WIN_HEIGHT",win_height);
	writeSetStrStr(fp,"SYS:EFFECTS_SEQ",effects_seq_str);

	/* Save dials. Do these before the buttons since these can alter modes 
	   settings if done after (except FQ and MF) */
	for(i=0;i < NUM_BUTTONS;++i)
	{
		butt = &button[i];
		name = button_name[i];

		/* Just use the angle to get dial values */
		if (butt->type != BTYPE_DIAL) continue;

		switch(i)
		{
		case BUT_SUB1_OFFSET:
		case BUT_SUB2_OFFSET:
		case BUT_GLIDE_DISTANCE:
			/* These buttons go from -127 to 128 */
			writeSetStrInt(
				fp,name,
				(int)butt->angle - DIAL_ANGLE_MIN - MAX_CHAR);
			break;

		case BUT_ANALYSER_RANGE:
			writeSetStrInt(fp,name,getAnalyserRange());
			break;

		case BUT_MAX_FREQ:
			/* Write frequency mode first as it resets max freq */
			writeSetStrStr(
				fp,button_name[BUT_FREQ_MODE],
			freq_mode[shm->freq_mode]);
			writeSetStrInt(fp,name,getMaxFrequency());
			break;

		case BUT_FM_OFFSET:
			writeSetStrDouble(fp,name,getFMOffset());
			break;

		case BUT_PHASER_LOW_OFF_MULT:
			writeSetStrDouble(fp,name,getPhaserLOM());
			break;
			
		case BUT_RES_DAMPING:
			writeSetStrDouble(fp,name,getResonanceDamping());
			break;

		case BUT_COMPRESS_EXP:
			writeSetStrDouble(fp,name,getGainCompressionExponent());
			break;
			
		case BUT_ECHO_STRETCH:
			writeSetStrDouble(fp,name,getEchoStretch());
			break;

		case BUT_RING_FREQ:
			writeSetStrDouble(fp,name,getRingModFreq(shm->ring_freq));
			break;

		default:
			writeSetStrInt(fp,name,(int)butt->angle-DIAL_ANGLE_MIN);
		}
	}

	/* Save buttons */
	for(i=0;i < NUM_BUTTONS;++i)
	{
		butt = &button[i];
		name = button_name[i];

		/* We ignore some button values - eg RST */
		switch(i)
		{
		case BUT_ECHO_INVERT:
			writeSetStrInt(fp,name,shm->echo_invert);
			break;

		case BUT_CHORD:
			writeSetStrStr(fp,name,chord_name[shm->chord]);
			break;

		case BUT_ARP_SEQ:
			writeSetStrStr(fp,name,arp_seq_name[shm->arp_seq]);
			break;

		case BUT_ARP_SPACING_DEC:
		case BUT_ARP_DELAY_DEC:
			// Do nothing - we set the INC instead
			break;

		case BUT_ARP_SPACING_INC:
			writeSetStrInt(fp,name,shm->arp_spacing);
			break;

		case BUT_ARP_DELAY_INC:
			writeSetStrInt(fp,name,shm->arp_delay);
			break;

		case BUT_SUB1_SOUND:
			writeSetStrStr(fp,name,sound_name[shm->sub1_sound]);
			break;

		case BUT_SUB2_SOUND:
			writeSetStrStr(fp,name,sound_name[shm->sub2_sound]);
			break;

		case BUT_FREQ_MODE:
			/* Already done above */
			break;

		case BUT_PHASING_MODE:
			writeSetStrStr(fp,name,phasing_mode[shm->phasing_mode]);
			break;

		case BUT_RES_MODE:
			writeSetStrStr(fp,name,resonance_mode[shm->res_mode]);
			break;

		case BUT_RING_RANGE:
			writeSetStrStr(fp,name,ring_range[shm->ring_range]);
			break;

		case BUT_RING_MODE:
			writeSetStrStr(fp,name,ring_mode[shm->ring_mode]);
			break;

		default:
			break;
		}
	}

	fclose(fp);
	message("Saved '%s'",disk.filename);
	return 0;
}




void writeSetStrStr(FILE *fp, char *s1, char *s2)
{
	fprintf(fp,"	set(\"%s\",\"%s\")\n",s1,s2);
}




void writeSetStrInt(FILE *fp, char *s, int i)
{
	fprintf(fp,"	set(\"%s\",%d)\n",s,i);
}




void writeSetStrDouble(FILE *fp, char *s, double d)
{
	fprintf(fp,"	set(\"%s\",%lf)\n",s,d);
}

/************************************ MISC ***********************************/


/*** Deal with a key entered for the filename ***/
void addFilenameChar(KeySym ksym, char key)
{
	char *valid = " 0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.@+-/*?!~#()[]=%&^:;'";

	switch(ksym)
	{
	case XK_Delete:
	case XK_BackSpace:
		if (disk.pos)
		{
			--disk.pos;
			disk.filename[disk.pos] = 0;
		}
		return;

	case XK_Return:
		if (disk.pos)
		{
			addFileSuffix();

			switch(disk.op)
			{
			case DISK_SAVE_PATCH:
				saveFile(1);
				break;

			case DISK_SAVE_PROG:
				saveFile(0);
				break;

			case DISK_LOAD_PATCH:
				loadFile(1);
				break;

			case DISK_LOAD_PROG:
				loadFile(0);
				break;

			default:
				assert(0);
			}
		}
		clearDiskStruct();
		return;
	}
	
	if (key && strchr(valid,key) && disk.pos <= PATH_MAX) 
		disk.filename[disk.pos++] = key;
}




void addFileSuffix()
{
	char *suffix;

	switch(disk.op)
	{
	case DISK_LOAD_PROG:
	case DISK_SAVE_PROG:
		suffix = BASIC_SUFFIX;
		break;

	default:
		suffix = PATCH_SUFFIX;
	}
	if (disk.filename[disk.pos-1] != '/' &&
	    (disk.pos < 4 || strcmp(disk.filename+disk.pos-4,suffix)))
	{
		strcat(disk.filename,suffix);
	}
}




void clearDiskStruct()
{
	bzero(&disk,sizeof(disk));
}
