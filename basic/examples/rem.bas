#
# Shows how REM statements can alter execution times
#
section main
	var i
	var prev
	var now
	yield 1
	print "With REM"
	print "--------"
	for i = 0 to 10
                rem Can put comments here
                rem And some more here 
		rem
		gosub "print_time_diff"
        next i
	print
	print "Without REM"
	print "-----------"
	for i = 0 to 10
		gosub "print_time_diff"
        next i
	quit

@print_time_diff
	now = $time
	if i then
               	print now - prev
	fi
	prev = now
	return
