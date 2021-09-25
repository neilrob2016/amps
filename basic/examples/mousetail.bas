# Flashing mouse tail
section main
	var on
	while 1
		set("SYS:MOUSE_TAIL",on)
		on = not on
		sleep 0.05
	wend
