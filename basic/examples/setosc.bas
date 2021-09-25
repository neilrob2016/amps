#
# Set oscillators by number or name
#

section init
	# Must set sub oscillators to something (not OFF) or they won't 
	# follow the main one
	set("SB1","SINE")
	set("SB2","SINE")
	set("SO1",1)
	set("SO2",-1)
	set("SUBS FOLLOW MAIN",1)

#
# Set the main oscillator numerically
#
section main
	#exit  # Comment out to run
	var ran
	while 1
		ran = random(7) + 1
		print "Main osc = ",ran
		set("SYS:MAIN_OSC",ran)
		sleep 0.2
	wend

#
# Set the main oscillator using the names of sounds
#
section main1
	exit 
	var snd
@sound_types
	data "sine fm","sine","square","triangle","sawtooth","aah","ooh","noise"
	autorestore "sound_types"

	while 1
		read snd
		set("SYS:MAIN_OSC",snd)
		sleep 0.2
	wend

