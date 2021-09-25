section init
	var a,b
	for a = 1 to 5*2 step 10/5
		print "a = ",a
		if a > 5 then continue fi
		for b = 1 to 10
			if b = 2 then print "   skipping b = 2" continue fi
			if b = 5 then
				print "   b end"
				break
			fi
			print "   b = ",b
		next b
		print "NEXT a"
	next a
	print "*** DONE ***"
