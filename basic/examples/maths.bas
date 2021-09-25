#
# Test some of the main maths functions
#
section init
	var i
	var s
	for i = 0 to 360 step 30
		s = sin(i)
		print "SIN(",intstr(i),") = ",s,", ABS = ",abs(s),", SGN = ",sgn(s)
	next i
	print "--------"
	for i = 0 to 360 step 30
		print "COS(",intstr(i),") = ",cos(i)
	next i
	print "--------"
	for i = 1 to 16
		print "SQRT(",intstr(i),") = ",sqrt(i)
	next i
	print "--------"
	i = 1
	var p
	for i = 1 to 10
		p = pow(10,i)
		print "POW(10,",intstr(i),") = ",p,", LOG10 = ",log10(p)
	next i
	print "--------"
	for i = 1 to 8
		p = pow(2,i)
		print "POW(2,",intstr(i),") = ",p,", LOG2 = ";
		if instr($build_options,"NO_LOG2",1) > -1 then
			print "<not implemented>"
		else
			print log2(i)
		fi
	next i
	print "--------"
	for i = 1 to 8
		p = pow($e,i)
		print "POW(E,",intstr(i),") = ",p,", LOG = ",log(p)
	next i

	print "MAX(10,20,3,4) = ",max(10,20,3,4)
	print "MIN(10,20,3,4) = ",min(10,20,3,4)
