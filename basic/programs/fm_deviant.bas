#
# Using values for the deep_fm2 patch, this modifies the FM1 parameter to
# produce an interesting effect.
#
section init
	reset

	# Init synth
	set("SYS:MAIN_OSC","SINE FM")
	set("SYS:KEY_START_NOTE","1C")
	set("SYS:SCALE","C#m")

	set("SB1","SINE FM")
	set("SO1",1)

	set("CH","1-8")

	set("FM1",5)
	set("FM2",10)
	set("FH",3)
	set("PH","ADDITIVE FLANGER")

	set("PS",70)
	set("PL",5)

	set("RT",100)

	set("EL",2)
	set("DE",60)
	
	set("MF",20)

	set("CS",160)
	set("VOL",200)
	
	# Init vars
	var FM1_MIN = 5
	var FM1_MAX = 60
	var fm1_val = FM1_MIN
	var fm1_add = 5
	var reset_on_play = 1


section play
	# Reset each time new note played
	if reset_on_play then
		set("FM1",FM1_MIN)
		fm1_val = FM1_MIN
		fm1_add = 5
	fi


section funckey
	if $function_key = 1 then
		mesg "Reset on note play"
		reset_on_play = 1
	else if $function_key = 2 then
		mesg "No reset on note play"
		reset_on_play = 0
	fiall


section timer 0.1
	set("FM1",fm1_val)
	fm1_val = fm1_val + fm1_add
	if fm1_val = FM1_MAX or fm1_val = FM1_MIN then
		fm1_add = -fm1_add
	fi
