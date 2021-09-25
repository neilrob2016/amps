#
# Tests nested gosubs
#
section init
	var i,sub
	for i = 1 to 10
		print "Gosub ",i
		gosub "sub1" : next i
	print "DONE"
	exit

@sub1
	print "IN SUB 1"
	gosub "sub2"	
	return	

@sub2
	print "IN SUB 2"
	# Pick a random subroutine to jump to
	sub = "sub"+intstr(random(3) + 1)
	print "SUB = ",sub
	gosub sub
	return

@sub3
	print "IN SUB 3"
	return
