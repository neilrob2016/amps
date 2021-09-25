# Uses FM wierd to generate some wierd bell sounds. Requires 1.9.1 min.
# July 2017
section init
	reset
	set("SYS:MAIN_OSC","SINE FM")
	set("SB1","SINE FM")
	set("SO1",1)

	set("FH",-30)
	set("FM1",80)
	set("FM2",147)
	set("FV1",58)
	set("FV2",104)

	set("PH","ADDITIVE PHASER")
	set("PO",3)
	set("PS",60)
	set("PL",5)

	set("EL",2)
	set("DE",75)
	set("MF",20)

	var fw = 0
	var add = 5

section timer 0.1
	set("FW",fw)
	fw = fw + add
	if fw < 0 or fw > 100 then 
		add = -add	
	fi
	
