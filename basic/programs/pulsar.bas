section init
	var flag
	reset
	set("SAWTOOTH",1)
	set("PS",80)
	set("PL",10)
	set("CH","1-4-6")
	set("DE",50)
	set("SYS:SCALE","C#")

	set("SB1","SAWTOOTH")
	set("SO1",-1)
	set("SB2","SAWTOOTH")
	set("SO2",-1)

	yield 100

section timer 0.1
	if flag then
		set("PH","ADDITIVE PHASER")
		set("VOL",150)
	else
		set("PH","OFF")
		set("VOL",50)
	fi
	flag = not flag
