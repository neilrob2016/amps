##############################################################################
#
# Automatic chord changer demo program. This changes the chord depending on 
# the note you're playing. Also after a timeout it starts cycling through all 
# the chords itself. Jan 2015
#
##############################################################################

section init
	var note_chord
	var release_time = $time
	var cycle_chords = 0
	var user_playing = 0
	var n,c
@chords
	# Chords to play for a given note
	data "A","1-4-6", "B","1-3-6", "C","1-3-5", "D","1-4-6"
	data "E","1-6-8", "F","1-3-8", "G","1-3-5"

	reset

	#
	# Load the note to chord name mappings
	#
	restore "chords"
	loop 7
		read n,c
		note_chord[n] = c
	lend
@setup
	# Set main oscillator to sawtooth
	set("SAWTOOTH",1)

	# Set sub osc 1 to sawtooth with 1 hz offset
	set("SB1","SAWTOOTH")
	set("SO1",1)

	# Set sub osc 2 to sawtooth with 1 hz offset
	set("SB2","SAWTOOTH")
	set("SO2",-1)

	# Set decay and echo
	set("DE",20)
	set("EL",5)
	if $section = "BUTTON" then
		return
	fi



#
# If reset button is pressed do set up again
#
section button
	if $button = "RST" then
		gosub "setup"
		cycle_chords = 0
		release_time = $time
	fi



#
# Note played - set the chord. If we're not in notes mode then $note_name will
# be an empty string
#
section play
	user_playing = 1
	if $note_name <> "" then
		# Set chord using chord name
		set("CH",note_chord[substr($note_name,1,1)])
	fi



#
# Note released. Make note of when
#
section release
	user_playing = 0
	release_time = $time



#
# Change chord automatically if no new note played for a short time
#
section main
	var current_chord

	while 1
		if user_playing then
			if cycle_chords then
				cycle_chords = 0
				set("HOLD NOTE","OFF")
			fi

			# Hand back control from Basic to save CPU cycles
			# otherwise we'd waste them spinning in a loop
			yield

			continue
		fi

		# Cycle chords using numerical value
		if cycle_chords then
			current_chord = ((current_chord + 1) % ($num_chords - 1)) + 1
			set("CH",current_chord)
			sleep 0.1
		else if $time - release_time > 2 then 
			if cycle_chords = 0 then
				cycle_chords = 1
				set("HOLD NOTE","ON")
				set("SYS:PLAY",1)
			fi
		else
			mesg "Just waiting..."
			sleep 0.5
		fiall
	wend
