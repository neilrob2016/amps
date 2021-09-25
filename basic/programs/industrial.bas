#
# Generates a kind of industrial/cyborg type noise using FM Flanger
#
section init
	reset
	set("VOL",150)
	set("SAWTOOTH",1)

	set("SB1","SAWTOOTH")
	set("SO1",1)

	set("CH","1-6-8")

	set("PH","FM FLANGER")
	set("PO",2)
	set("PS",2)

	set("EL",2)
	set("DE",10)
	set("MF",50)

	var ang0
	var ang1


section timer0 0.02
	set("SYS:FILTER",200 * abs(sin(ang0)))
	ang0 = (ang0 + 2) % 360


section timer1 0.02
	set("SO1",10 * sin(ang1))
	ang1 = (ang1 + 5) % 360
