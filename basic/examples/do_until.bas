section init
	var a,b
	do: print "a = ",a : b = 0
		do: print "    b = ",intstr(b): b = b+1: until b = 5
		a = a + 1
	until a >= 10 : print "*** DONE ***"
