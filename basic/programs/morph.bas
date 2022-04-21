#
# Shows morphing between a square/sawtooth and sine wave on the main oscillator
#
section main
	var add = 1
	var mval = 0
	var cnt = 0

	set("SYS:PLAY","ON")
	set("SYS:MAIN_OSC","SQUARE")
	set("SYS:FREQ",100)
	set("VOL",200)

	while 1
		set("MOM",mval)
		if mval = 255 then
			add = -1
		else if mval = 0 then
			add = 1
			cnt = cnt + 1
			if not (cnt % 4) then
				set("SYS:MAIN_OSC","SQUARE")
			else if not (cnt % 2) then
				set("SYS:MAIN_OSC","SAWTOOTH")
			fiall
		fiall
		mval = mval + add
			
		sleep 0.01
	wend

