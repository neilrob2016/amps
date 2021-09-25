section main
	var i = 5
	print i & 1
	print i & 2
	print i & 4
	print i & 0xF
	print "----"

	print i | 1
	print i | 2
	print i | 4
	print i | 0xF

	print "----"
	print i ^ 1
	print i ^ 2
	print i ^ 4 
	print i ^ 0xF

	for i = 1 to 8
		print "Left shift 1 << ",i," = ",1 << i
		sleep 0.1
	next i
	for i = 1 to 8
		print "Right shift 256 >> ",i," = ",0x100 >> i
		sleep 0.1
	next i
