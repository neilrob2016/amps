# Filter get/set test
section main
	var filt = 0,add = 10

	# Setting this to zero means Basic will never yield control to main
	# synth loop unless forced by a "yield" command
	set("SYS:YIELD",0)

	while 1
		set("SYS:FILTER",filt)

		# Force return of control to main synth loop at this point so
		# timing of setting filter is consistent
		yield 

		print "Filter = ",get("SYS:FILTER")
		filt = filt + add
		if filt >= 255 then
			add = -add
			filt = 255
		else if filt <= 0 then
			add = -add
			filt = 0
		fiall
	wend
