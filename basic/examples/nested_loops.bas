section init
	var a
	var b
	var c
	WHILE a < 10 : print "a = ",a
		b = 0
		WHILE b < 5
			print "   b = ",intstr(b): b = b+1:
			FOR c = 1 to 3:
				print "      c = ",c
			NEXT c
		WEND
		a = a + 1
	WEND: print "*** DONE ***"
