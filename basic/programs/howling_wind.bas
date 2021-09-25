#
# Howling wind. 
#
# Vary the frequency of some white noise and set up some resonance on it.
#
section main
	var freq 
	var min_freq
	var freq_add
	var ang 
	var inc

	reset
	set("EL",15)

	set("NOISE",1)

	# Resonance
	set("RE","FOLLOW MAIN FREQ")
	set("RL",10)
	set("RF",20)

	# Other
	set("FQ","CONTINUOUS")
	set("VOL",100)

	while 1
		set("SYS:PLAY",1)

		inc = 1 + random(9)
		min_freq = 20 + random(500)
		freq_add = 500 + random(2500)

		for ang = 0 to 180 step inc
			freq = min_freq + sin(ang) * freq_add
			set("SYS:FREQ",freq)
			sleep 0.1
		next ang

		set("SYS:PLAY",0)

		sleep 0.1 + random(3)
	wend
