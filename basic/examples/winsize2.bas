# Demonstrates the window section and the event type associated with it
section window
	if $win_mapped then
		print "Window mapped. Size = ",intstr($win_width),"x",intstr($win_height)
	else
		print "Window unmapped"
	fi

section event
	print "Event type = ",$event_type
