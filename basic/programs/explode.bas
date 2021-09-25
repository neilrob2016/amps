section main
	var freq 
	var vol

	reset
	set("NOISE",1)
	set("RT",75)
	set("DE",35)
	set("FQ","CONTINUOUS")

	set("SYS:PLAY",1)

	vol = 250
	for freq = 500 to 100 step -10
		set("SYS:FREQ",freq)
		set("VOL",vol)
		yield
		vol = vol - 5
	next freq	
	
	set("SYS:PLAY",0)
