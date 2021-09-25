#
# Laser gun. Not terribly convincing really.
#
section main
	var vol = 255
	var freq = 2000
	var exp = 8
	
	set("NOISE","ON")
	set("FQ","CONTINUOUS")
	set("SB1","SQUARE")
	set("SYS:PLAY","ON")
	yield 200

	while freq > 50
		set("VOL",vol)
		set("SYS:FREQ",freq)

		freq = freq - pow(2,exp)
		if exp > 0.2 then
			exp = exp - 0.2
		fi
		vol = vol - pow(1.5,exp)

		sleep 0.02
	wend
	set("SYS:PLAY","OFF")

