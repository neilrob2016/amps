SOUND=0

# Linux ALSA
SOUND=1
LIBS=-lasound

# Linux OpenSound
#SOUND=2

# OSX
#SOUND=3
#LIBS=-framework AudioToolbox -framework CoreFoundation

# No sound
#SOUND=4

# Some tired old systems don't have the log2() function. Uncomment this if yours
# is one of them. You'll soon know when the compile fails with an unresolved
# symbol for log2().
#NO_LOG2=-DNO_LOG2

# Should cover all OS's
LIBPATH=-L/usr/X11/include -L/usr/X11R6/lib -L/usr/X11R6/lib64

CC=cc -Wall -g -I/usr/X11/include -DSOUND=$(SOUND) $(NO_LOG2)
OBJS=\
	main.o \
	sound_init.o \
	sound_if.o \
	sound_run.o \
	sound_gen.o \
	sound_misc.o \
	mouse.o \
	keyboard.o \
	buttons.o \
	mesg.o \
	dft.o \
	disk.o \
	draw.o \
	recorder.o \
	misc.o \
	bas_init.o \
	bas_tokeniser.o \
	bas_run.o \
	bas_commands.o \
	bas_variables.o \
	bas_expressions.o \
	bas_functions.o \
	bas_values.o \
	bas_labels.o \
	bas_misc.o


amps: build_date $(OBJS) Makefile
	cc $(OBJS) $(LIBS) $(LIBPATH) -lX11 -lm -lXext -o amps

build_date:
	echo "#define BUILD_DATE \"`date -u +'%F %T %Z'`\"" > build_date.h

main.o: main.c globals.h waveforms.h Makefile
	$(CC) -c main.c 

sound_init.o: sound_init.c globals.h Makefile
	$(CC) -c sound_init.c 

sound_if.o: sound_if.c globals.h Makefile
	$(CC) -c sound_if.c

sound_run.o: sound_run.c globals.h Makefile
	$(CC) -c sound_run.c 

sound_gen.o: sound_gen.c globals.h Makefile
	$(CC) -c sound_gen.c

sound_misc.o: sound_misc.c globals.h Makefile
	$(CC) -c sound_misc.c

mouse.o: mouse.c globals.h Makefile
	$(CC) -c mouse.c

keyboard.o: keyboard.c globals.h Makefile
	$(CC) -c keyboard.c

buttons.o: buttons.c globals.h Makefile
	$(CC) -c buttons.c

mesg.o: mesg.c globals.h Makefile
	$(CC) -c mesg.c

dft.o: dft.c globals.h Makefile
	$(CC) -c dft.c

disk.o: disk.c globals.h Makefile
	$(CC) -c disk.c

draw.o: draw.c globals.h Makefile
	$(CC) -c draw.c

recorder.o: recorder.c globals.h Makefile
	$(CC) -c recorder.c

misc.o: misc.c globals.h Makefile
	$(CC) -c misc.c

bas_init.o: bas_init.c globals.h Makefile
	$(CC) -c bas_init.c

bas_tokeniser.o: bas_tokeniser.c globals.h Makefile
	$(CC) -c bas_tokeniser.c

bas_run.o: bas_run.c globals.h Makefile
	$(CC) -c bas_run.c

bas_commands.o: bas_commands.c globals.h Makefile
	$(CC) -c bas_commands.c

bas_variables.o: bas_variables.c globals.h Makefile
	$(CC) -c bas_variables.c

bas_expressions.o: bas_expressions.c globals.h Makefile
	$(CC) -c bas_expressions.c

bas_functions.o: bas_functions.c globals.h Makefile
	$(CC) -c bas_functions.c

bas_values.o: bas_values.c globals.h Makefile
	$(CC) -c bas_values.c

bas_labels.o: bas_labels.c globals.h Makefile
	$(CC) -c bas_labels.c

bas_misc.o: bas_misc.c globals.h Makefile
	$(CC) -c bas_misc.c

clean:
	rm -f core amps build_date.h *.o 
