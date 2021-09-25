#
# Trance Tremelo 3. Same as 2 except new ring modulator functionality
# used to vary volume.
#
section init
	printmesg ""
	printmesg ""
	printmesg "*** TRANCE TREMELO 3 ***"
	var ang2
	var val
	var do_fill = 1

	reset
	set("SYS:MAIN_OSC","SAWTOOTH")
	set("SYS:SCALE","Cm")
	set("SB1","SAWTOOTH")
	set("SB2","SAWTOOTH")

	set("CH","1-4-6") # Chord
	set("EL",10)      # Echo length
	set("DE",10)      # Decay

	set("RE",2)       # Resonance follow main freq
	set("RL",10)      # Resonance level
	set("RF",5)       # Resonance freq

	set("PH",5)       # Additive phaser
	set("PS",45)
	set("PL",7)

	set("CS",150)

	# Ring modulator
	set("RI","RANGE 1")
	set("RM","SINE")
	set("IF",7)
	set("IL",200)

	set("VOL",200)


#
# Switch filling on/off
#
section funckey
	if $function_key = 1 then
		printmesg "Fill waveform"
		do_fill = 1
	else if $function_key = 2 then
		printmesg "Don't fill waveform"
		do_fill = 0
		set("SYS:FILL",0)
	fiall


# Vary sub oscillator offsets
section main
	var offset 
	var ang1
	while 1
		for ang1 = 0 to 360 step 5
			offset = 10 * abs(sin(ang1))
			set("SO1",offset)
			set("SO2",-offset)
		next ang1
	wend


# Sweep aliasing and filter at the same time
section timer 0.02
	val = 150 * abs(sin(ang2))
	set("AL",20 + val)
	set("SYS:FILTER",val)
	if do_fill then
		if val < 120 then
			set("SYS:FILL",1)
		else
			set("SYS:FILL",0)
		fi
	fi
	ang2 = (ang2 + 1) % 360
