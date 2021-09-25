# Gradually increase the window size. Not much practical use.
section main
	var i
	for i = 10 to 1000 step 50
		set("SYS:WIN_WIDTH",i)
		set("SYS:WIN_HEIGHT",i/2)
		print "Width = ",$win_width,", height = ",$win_height
		sleep 0.2
	next i
