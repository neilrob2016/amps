/*** Opens sound devices for Alsa or OpenSound ***/

#include "globals.h"


#if SOUND != NO_SOUND

static void initSound();
#if SOUND==ALSA
static void initALSAdevice(snd_pcm_t *hnd);
#endif

#endif


/*** Create shared memory, reset shared mem, creates notes then spawn child
     process and call child mainloop ***/
void startSoundDaemon()
{
	pid_t pid;

	pcm_freq = PCM_FREQ;
#if SOUND==OSX
	do_sampling = 0;
#elif SOUND != NO_SOUND
	if (do_sound)
	{
		do_sampling = 1;
		initSound();
	}
#endif

	/* Spawn off sound daemon as child process */
	switch((pid = fork()))
	{
	case -1:
		printf("SOUND: fork(): %s\n",strerror(errno));
		exit(1);

	case 0:
		/* Child */
		signal(SIGINT,SIG_IGN);
		signal(SIGQUIT,SIG_IGN);
		signal(SIGTERM,SIG_IGN);
#if SOUND==OSX
		/* The audio framework has to be initialised after the fork */
		initSound();
#endif
		soundLoop();
		exit(0);
	}
	printf("Sound PID    : %u\n\n",pid);

	/* Parent ends up here */
#if SOUND != NO_SOUND
	if (do_sound)
	{
#if SOUND==ALSA
		snd_pcm_close(handle);
#elif SOUND==OPENSOUND
		close(sndfd);
#endif
	}
#endif
}




/*** Called from above and from main.c ***/
void resetSharedMemory()
{
	bzero(shm,sizeof(struct st_sharmem));

	/* Set up non-zero shared memory values */
	shm->win_width = win_width;
	shm->sound = SND_SINE_FM;
	shm->buffer_reset = 1;
	shm->echo_decay = 100;
	shm->echo_stretch = MAX_CHAR;
	shm->arp_delay = 2;
	shm->arp_spacing = 2;
	shm->filter_sweep = MAX_UCHAR;
	shm->freq_mode = FREQ_NOTES;

	/* These have 127 subtracted before use. Set to this for dial angle */
	shm->sub1_offset = MAX_CHAR;
	shm->sub2_offset = MAX_CHAR;

	shm->sub1_vol = MAX_UCHAR;
	shm->sub2_vol = MAX_UCHAR;

	shm->fm_harm_offset = MAX_CHAR;
	shm->fm_offset = MAX_CHAR;
	shm->fm_volume1 = 100;
	shm->fm_volume2 = 100;

	shm->res_freq = 1;
	shm->res_damping = 252; 

	shm->phaser_freq_sep = MAX_CHAR;
	/* goes 0 -> 2, default = 0.5 */
	shm->phaser_low_off_mult = MAX_UCHAR / PHASER_LOW_MULT_RANGE / 2;

	shm->square_width = MAX_UCHAR;
	shm->volume = VOLUME_INIT;
	shm->note_scale = SCALE_C;

	shm->glide_distance = MAX_CHAR;
	shm->glide_velocity = 1;

	shm->compress_start = 230;
	shm->compress_exponent = 240;

	shm->do_reset = 1;

	resetEffectsSeq(0);
}




/*** Create the note frequencies for the scales. Notes each successive octave 
     has a frequency 2* the same note in the previous octave ***/
void createNotes()
{
	int i;
	int j;
	int k;
	int note;
	int mult;

	middle_c = 0;

	/* Go through scales */
	for(i=0;i < NUM_SCALES;++i)
	{
		/* Go through octaves in scale */
		for(j=0;j < NUM_OCTAVES;++j)
		{
			/* Go through notes in octave */
			for(k=0;k < OCTAVE;++k) 
			{
				note = j*OCTAVE+k;
				mult = 1 << j;
				note_freq_scale[i][note] = (int)round(base_octave[i][k] * mult);
/*
				printf("INFO: Scale = %d, octave = %d, note = %d, freq = %d\n",
					i,j,k,note_freq_scale[i][note]);
*/
				if (i == SCALE_C &&
				    note_freq_scale[i][note] == round(FREQ_C * 8))
				{
					middle_c = note;
				}
			}
		}
	}
	if (!middle_c) 
	{
		puts("SOUND: Internal error - middle C not set");
		exit(1);
	}
	note_freq = note_freq_scale[SCALE_C];
}



#if SOUND != NO_SOUND
#if SOUND==ALSA

/*** Set up ALSA sound device ***/
void initSound()
{
	int err;

	/* Open output */
	if ((err = snd_pcm_open(&handle,alsa_device,SND_PCM_STREAM_PLAYBACK,0)) < 0)
	{
		printf("SOUND: Can't open audio: snd_pcm_open() 1: %s\n",
			snd_strerror(err));
		exit(1);
	}

	initALSAdevice(handle);

	/* Open microphone. 0 for final arg means using blocking mode */
	if ((err = snd_pcm_open(
		&mic_handle,ALSA_DEVICE,SND_PCM_STREAM_CAPTURE,0)) < 0)
	{
		printf("SOUND: WARNING: Can't open mic: snd_pcm_open() 2: %s\n",
			snd_strerror(err));
		do_sampling = 0;
	}
	else initALSAdevice(mic_handle);
}




void initALSAdevice(snd_pcm_t *hnd)
{
	snd_pcm_hw_params_t *hw_params;
	int err;

	/* Allocate hardware params structure */
	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_malloc(): %s\n",
			snd_strerror(err));
		exit(1);
	}

	/* Init structure */
	if ((err = snd_pcm_hw_params_any(hnd,hw_params)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_any(): %s\n",snd_strerror(err));
		exit(1);
	}
	
	/* Set interleaved regardless of mono or stereo */
	if ((err = snd_pcm_hw_params_set_access(
		hnd,hw_params,SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_set_access(): %s\n",snd_strerror(err));
		exit(1);
	}

	/* Set number of channels. 1 in this case because we want mono */
	if ((err = snd_pcm_hw_params_set_channels(hnd,hw_params,1)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_set_channels(): %s\n",snd_strerror(err));
		exit(1);
	}

	/* Set word format */
	if ((err = snd_pcm_hw_params_set_format(hnd,hw_params,SND_PCM_FORMAT_S16_LE)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_set_format(): %s\n",snd_strerror(err));
		exit(1);
	}

	/* Set PCM frequency */
	if ((err = snd_pcm_hw_params_set_rate_near(hnd,hw_params,&pcm_freq,0)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params_set_rate_near(): %s\n",snd_strerror(err));
		exit(1);
	}
	if (pcm_freq != PCM_FREQ)
		printf("SOUND: WARNING: Device PCM freq %dHz is not the requested freq %dHz\n",pcm_freq,PCM_FREQ);

	/* Do actual set of parameters on device */
	if ((err = snd_pcm_hw_params(hnd,hw_params)) < 0)
	{
		printf("SOUND: snd_pcm_hw_params(): %s\n",snd_strerror(err));
		exit(1);
	}
	snd_pcm_hw_params_free(hw_params);	

	/* Not sure what this is for */
	if ((err = snd_pcm_prepare(hnd)) < 0)
	{
		printf("SOUND: snd_pcm_prepare(): %s\n",snd_strerror(err));
		exit(1);
	}
}


#elif SOUND==OPENSOUND

/*** Use open sound system /dev devices ***/
void initSound()
{
	int tmp;

	/* Open sound device. If can't open with microphone try write only
	   ie - output only */
	if ((sndfd = open("/dev/dsp",O_RDWR)) == -1 &&
	    (sndfd = open("/dev/audio",O_RDWR)) == -1)
	{
		do_sampling = 0;
		if ((sndfd = open("/dev/dsp",O_WRONLY)) == -1 &&
	    	    (sndfd = open("/dev/audio",O_WRONLY)) == -1)
		{
			printf("SOUND: Can't open /dev/dsp or /dev/audio for I/O: %s\n",strerror(errno));
			if (errno == EBUSY)
				puts("SOUND: Consider using 'aoss' wrapper script");
			exit(1);
		}
	}

	/* Reset it */
	if (ioctl(sndfd,SNDCTL_DSP_RESET) == -1)
	{
		printf("SOUND: ioctl(sndfd,SNDCTL_DSP_RESET) failed: %s\n",
			strerror(errno));
		exit(1);
	}

	/* Set data endian and word size */
	tmp = AFMT_S16_NE;
	if (ioctl(sndfd,SNDCTL_DSP_SETFMT,&tmp) == -1)
	{
		printf("SOUND: ioctl(SNDCTL_DSP_SETFMT) failed: %s\n",
			strerror(errno));
		exit(1);
	}

	/* Set sample rate */
	if (ioctl(sndfd,SNDCTL_DSP_SPEED,&pcm_freq) == -1)
	{
		printf("SOUND: ioctl(SNDCTL_DSP_SPEED) failed: %s\n",
			strerror(errno));
		exit(1);
	}
	if (pcm_freq != PCM_FREQ)
		printf("SOUND: WARNING: Device PCM freq %dHz is not the requested freq %dHz\n",pcm_freq,PCM_FREQ);
}

#elif SOUND==OSX

void initSound()
{
	AudioStreamBasicDescription fmt;
	int i;

	osx_buffer = (AudioQueueBufferRef *)malloc(
		sizeof(AudioQueueBufferRef) * num_osx_buffers);
	assert(osx_buffer);

	/* Set up device format */
	bzero(&fmt,sizeof(AudioStreamBasicDescription));
	fmt.mSampleRate = PCM_FREQ;
	fmt.mFormatID = kAudioFormatLinearPCM;
	fmt.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
	fmt.mFramesPerPacket = 1;
	fmt.mChannelsPerFrame = 1;  /* 2 for stereo */
	fmt.mBytesPerPacket = 2;
	fmt.mBytesPerFrame = 2;
	fmt.mBitsPerChannel = 16;  /* using 16 bit short for each sample */

	/* Create audio queue and set callback */
	if (AudioQueueNewOutput(
		&fmt,osxAudioCallback,NULL,
		CFRunLoopGetCurrent(),kCFRunLoopCommonModes,0,&osx_queue))
	{	
		puts("SOUND: AudioQueueNewOutput() failed");
		exit(1);
	}
	for(i=0;i < num_osx_buffers;++i)
	{
		if (AudioQueueAllocateBuffer(
			osx_queue,SNDBUFF_BYTES,&osx_buffer[i]))
		{
			puts("SOUND: AudioQueueAllocateBuffer() failed");
			exit(1);
		}
		/* We always write the same number of bytes out each time so 
		   just set once */
		osx_buffer[i]->mAudioDataByteSize = SNDBUFF_BYTES;

		/* Number of packets which since we're using shorts means / 2 */
		osx_buffer[i]->mPacketDescriptionCount = SNDBUFF_BYTES / 2;

		bzero(osx_buffer[i]->mAudioData,SNDBUFF_BYTES);

		/* Initial enqueue to start things off else callback won't work
		   for whatever reason */
		AudioQueueEnqueueBuffer(osx_queue,osx_buffer[i],0,NULL);
	}
	osx_buff_num = 0;

	AudioQueueSetParameter(osx_queue,kAudioQueueParam_Volume,1.0);
	/* Doesn't seem to make any difference */
	AudioQueueSetParameter(osx_queue,kAudioStreamPropertyLatency,0);
	AudioQueueStart(osx_queue,NULL);
}

#endif

#endif /* NO_SOUND */
