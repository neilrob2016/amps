#
# Uses echo stretch to drop the note freq on key release
#
section init
	reset
	set("SAWTOOTH",1)
	set("CH","1-4-6")
	set("EL",5)
	set("ED",200)
	set("PH","SUBTRACTIVE DIRTY FLANGER")
	set("PO",10)
	set("PS",20)
	set("PL",140)
	set("VOL",150)
	set("SYS:SCALE","Cm")

section play
	set("EST",1)

section release
	set("EST",1.01)
