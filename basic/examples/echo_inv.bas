# Switch echo invert on/off
section main
	var flag

	set("EL",3)
	set("SAWTOOTH",1)
	set("VOL",100)

	while 1
		set("EI",flag)
		flag = not flag
		sleep .1
	wend
