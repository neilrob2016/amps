section init
	dim i
	for i = 1 to 5
		print "I = ",intstr(i)
		loop i
			print "   loop"
		lend
	next i
	print "DONE"
