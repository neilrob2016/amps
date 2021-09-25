#
# The keyboard key start note
#
section init
	print "At init = ",$key_start_note

section keystart
	print "Key start note = ",$key_start_note

section event
	print "Event = ",$event_type

section main
	var s = 0
	while 1
		print "set result      = ",set("SYS:KEY_START_NOTE",s)
		print "$key_start_note = ",$key_start_note
		s = s + 1
		sleep 1
	wend
