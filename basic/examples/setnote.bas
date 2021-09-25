# Testing setting notes by name or number
section init
	set("SYS:MAIN_OSC","SAWTOOTH")
	set("SYS:SCALE","C#")
	set("SYS:PLAY",1)

section main
	#exit
@notes
	data "0C","0C#","0F","0F#","0A#",
	data "1C","1C#","1D#","1F","1F#","1G#","1A#",
	data "2C","2C#","2F","2F#","2A#",
	data "3C#","3F",
	data "4C#","4F"
	autorestore "notes"
	var note

	while 1
		read note
		print "Note = ",note,", ",intstr(set("SYS:NOTE",note))
		sleep 0.1
	wend

section main1
	exit
	var i
	for i = 0 to $num_notes-1
		print "Note = ",set("SYS:NOTE",i)
		sleep 0.1
	next i
