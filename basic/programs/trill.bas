section init
	var cnt
	var add
	var il
	reset
@setup
	cnt = 0
	add = 1
	il = 1
	set("SYS:MAIN_OSC","SAWTOOTH")
	set("SB1","SAWTOOTH")
	set("SB2","SAWTOOTH")
	set("SO1",-1)
	set("SO2",1)
	set("EL",5)
	set("RI","RANGE 1")
	set("RM","SINE")
	set("IL",180)
	set("PH","ADDITIVE PHASER")
	set("PO",4)
	set("PL",3)
	set("PS",100)
	set("IF",2)
	cnt = 0

section timer0 .2
	set("SO1",get("SO1")-add*2)
	set("SO2",get("SO2")+add)
	set("PL",get("PL")+add*10)
	set("IF",il+add*2)
	cnt = cnt + 1
	if cnt = 5 then
		add = -add
		cnt = 0
	fi
	
section button
	if $button = "RST" then
		gosub "setup"
	fi

section play
	il = $note
