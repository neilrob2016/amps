#
# Switch an echoing sine wave on and off 
#
section main
	reset
	set("SINE",1)
	set("VOL",200)
	set("SYS:NOTE","5C")
	set("EL",1)
	set("ED",200)

	while 1
		set("SYS:PLAY",1)
		mesg("PING!")
		sleep 0.2
		set("SYS:PLAY",0)
		sleep 1.8
	wend
