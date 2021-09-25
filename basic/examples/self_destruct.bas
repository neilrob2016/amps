#
# Countdown to self destruct
#
section main
	var i

	reset
	set("SYS:MAIN_OSC","SQUARE")
	set("SYS:NOTE","4C")

	sleep 1
	mesg "Hello! I will self terminate in..."
	sleep 1

	for i = 5 to 1 step -1
		mesg intstr(i)," seconds"
		set("SYS:PLAY",1)
		sleep .1
		set("SYS:PLAY",0)
		sleep 0.9
	next i
	mesg "Goodbye cruel world!"
	set("SYS:PLAY",1)
	sleep 1
	set("SYS:PLAY",0)

	set("DEL PROG",1)
