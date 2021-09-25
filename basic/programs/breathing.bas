#
# Makes a sound like breathing
#
section init
	reset
	set("SYS:MAIN_OSC","NOISE")
	set("SYS:PLAY",1)
	set("SYS:NOTE","2C")
	set("HOLD NOTE",1)
	set("RE",2)
	set("RF",40)
	var ang

section timer 0.1
	set("RL",abs(sin(ang) * 100))
	ang = (ang + 10) % 360
