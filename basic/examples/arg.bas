# The -arg parameter value is passed to BASIC as $arg
section init
	if $arg = "" then
		print "No -arg parameter provided"
	else
		print "Arg = ",$arg
	fi
