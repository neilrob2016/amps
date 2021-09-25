#
# Trance Tremelo. Dec 2014
#
section init
	reset
	set("SYS:MAIN_OSC","SAWTOOTH")
	set("SB1","SAWTOOTH")
	set("SB2","SAWTOOTH")

	set("CH","1-4-6") # Chord
	set("EL",10)      # Echo length
	set("DE",10)      # Decay

	set("RE",2)       # Resonance follow main freq
	set("RL",10)      # Resonance level
	set("RF",5)       # Resonance freq


# Vary the volume rapidly
section main
	var max_vol = 100
	var ang 
	while 1
		for ang = 0 to 360 step 5
			set("VOL",20 + max_vol * abs(sin(ang)))
		next ang
	wend


# Vary sub oscillator offsets
section main1
	var offset 
	var ang1
	while 1
		for ang1 = 0 to 360 step 5
			offset = 10 * abs(sin(ang1))
			set("SO1",offset)
			set("SO2",-offset)
		next ang1
	wend


# Switch between note and frequency offset
section main2
	# exit
	var on = 0
	while 1
		sleep 1
		set("SUB1 NOTES",on)
		set("SUB2 NOTES",on)
		on = not on
	wend

