#
# Go through the ring mod frequencies
#
section main
	dim range,freq
	dim max_freq = $num_ring_ranges * 256
	
	reset
	range = get("RI")

	set("RM","SQUARE")
	set("IL",255)
	set("SYS:NOTE","2C")
	set("SYS:PLAY",1)

	for freq = 1 to max_freq step 5
		set("IF",freq)
		if get("RI") > range then
			range = get("RI")
			printmesg "Ring range now = ",intstr(range)
		fi
		sleep 0.1
	next freq
	printmesg "*** DONE ***"
	
