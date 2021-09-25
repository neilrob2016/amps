# Demonstrates obtaining note number by name
section init
@notes
	# C
	data "1F","1G","1A","1B","2C","2D"
	# G Minor
	data "0C#","0D#","3B","6G#","6A#","6B"
	restore "notes"

	var note
	var notename
	var C_MAJOR = 0
	var G_MINOR = 11

	var scale = C_MAJOR
	loop 2
		print "Scale: ",getlongscalename(scale)
		loop 6
			read notename
			print "   Note ",notename," = ",getnotebyname(notename,scale)
		lend
		scale = G_MINOR
	lend
