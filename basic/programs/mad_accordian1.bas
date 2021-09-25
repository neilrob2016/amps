section init
	reset
	set("VOL",150)
	set("SQUARE",1)
	set("SB1","SQUARE")
	set("SB2","SAWTOOTH")
	set("CH","1-3-5")
	set("DE",20)
	set("EL",5)

	set("RI","RANGE 1")
	set("RM","SINE")
	set("IF",1)
	set("IL",255)

	var ang0
	var ang1
	var val

section timer0 0.02
	set("IF",abs(sin(ang0) * 5) + 3)
	ang0 = (ang0 + 2) % 360

section timer1 0.1
	val = abs(sin(ang1) * 5)
	set("SO1",val)
	set("SO2",-val) 
	ang1 = (ang1 + 10) % 360
