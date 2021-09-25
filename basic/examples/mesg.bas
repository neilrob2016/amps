#
# Switching system messages on or off 
#
section init
	print "Messages = ",get("SYS:MESG")
	
section main
	var i
	for i = 0 to $num_chords - 1
		set("CH",i)
		if i = 3 or i = 10 then
			mesg "Messages on!"
			set("SYS:MESG","ON")
		else if i = 6 then
			mesg "Messages off!"
			set("SYS:MESG","OFF")
		fiall
		sleep 0.5
	next i
	mesg "DONE!"
