#
# Electronic yabbering
#
section init
	reset
	set("SYS:MAIN_OSC","SINE FM")
	set("SYS:PLAY",1)
	set("MF",5)


section timer 0.02
	set("FH",random(255) - 127)
	set("FM1",random(255))
	set("FM2",random(255))


section timer1 0.2
	set("SYS:NOTE",random(5))
