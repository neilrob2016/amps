#
# Generates appegiations just using sub oscillator note offsets
#
section init
	var ang 
	var val
@setup
	ang = 0

	reset
	set("SYS:MAIN_OSC","TRIANGLE")
	set("SB1","TRIANGLE")
	set("SB2","TRIANGLE")

	set("DE",20)
	set("EL",5)
	
	set("SUB1 NOTES",1)
	set("SUB2 NOTES",1)

	if $section = "BUTTON" then
		return
	fi


section button
	if $button = "RST" then
		gosub "setup"
	fi


section timer 0.05
	val = 5 + sin(ang) * 3
	set("SO1",val)
	set("SO2",-val)
	ang = (ang + 10) % 360
