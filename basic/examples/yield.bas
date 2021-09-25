# Show effect of setting the yield.
section main
	var y
	for y = 100 to 1 step -1
		print "Yield = ",get("SYS:YIELD")
		yield y
		# Alternative. Returns yield value
		# set("SYS:YIELD",y)
	next y
	exit
