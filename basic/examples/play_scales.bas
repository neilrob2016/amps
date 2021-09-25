#
# Play 3 octaves for each scale
#
section main
	var s,n

	set("SYS:MAIN_OSC","TRIANGLE")
	set("SYS:PLAY","ON")

	for s = 0 to $num_scales - 1
		set("SYS:SCALE",s)
		for n = 7 to 28
			set("SYS:NOTE",n)
			sleep 0.1
		next n
	next s

	set("SYS:PLAY","OFF")
