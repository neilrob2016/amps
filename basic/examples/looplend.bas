section init
	dim i,j
	i = 0
	loop 3
		i = i + 1
		print "Outer count = ",intstr(i)
		j = 0
		loop 5
			j = j + 1
			print "   Inner count = ",intstr(j)
			if i = 2 and j = 3 then break fi
		lend
	lend
	print "DONE"
