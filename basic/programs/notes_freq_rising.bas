#
# Increase the frequency and disply when we've reached the notes in the
# scale of C (which is has the value zero)
#
section init
	var freq 
	var note
	var prev_note

	reset
	set("FQ","CONTINUOUS")
	set("SYS:PLAY",1)
	set("SINE",1)
	set("VOL",200)


section timer0 0.02
	set("SYS:FREQ",freq)
	note = getnotebyfreq(freq,0)

	if note <> prev_note then
		mesg "Note = ",getnotename(note,0),", freq = ",tointstr(freq)
		prev_note = note
	else if note = $num_notes - 1 then
		mesg "*** DONE ***"
		set("SYS:PLAY",0)
		exit
	fiall
	freq = freq + 1
	
	
