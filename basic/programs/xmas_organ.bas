#
# A nice Christmassy sounding organ. Dec 2016
#
section init
	var chordmap
	var chord
	var note
	var vibang
	var i

@setup
	# Set up note to chord mappings
	data "C","1-3-5","D","1-3-6","E","1-4-6","F","1-3-8","G","1-4-8"
	data "A","1-3-8","B","1-5-8"
	restore "setup"
	for i = 1 to 7
		read note,chord
		chordmap[note] = chord
	next i
	vibang = 0

	# Set up synth params
	reset
	set("SYS:MAIN_OSC","SINE FM")
	set("SB1","SINE FM")
	set("VOL",50)
	set("SO1",1)
	set("FH",2)
	set("FM1",50)
	set("FM2",10)
	set("VL",200)
	set("EL",3)
	set("DE",50)
	set("RE","FOLLOW MAIN FREQ")
	set("RF",5)

section button
	if $button = "RST" then
		gosub "setup"
	fi

section play
	# Change chord depending on the note played
	set("CH",chordmap[substr($note_name,1,1)])
	set("RL",$note*2)

section timer 0.1
	# Modulate the vibrato frequency
	vibang = (vibang + 10) % 360
	set("VS",2 + 5 * abs(sin(vibang)))

